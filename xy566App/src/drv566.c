
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbDefs.h>
#include <dbScan.h>
#include <epicsThread.h>
#include <devLib.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <errlog.h>
#include <ellLib.h>

#include <epicsExport.h>

#include "xy566.h"

int dbg566=0;
epicsExportAddress(int,dbg566);

static ELLLIST xy566s={{NULL,NULL},0};

static
void xycom566isr(void *);

void
xycom566setup(
      int id,
      int cbase,
      int dbase,
      int level,
      int vec,
      int bipol
){
  xy566 *card;
  volatile epicsUInt8  **cb;
  volatile epicsUInt16 **db;
  epicsUInt16 junk;

  if(cbase<0 || cbase>0xffff){
    errlogPrintf("config (A16) out of range\n");
    return;
  }
  if(dbase<0 || dbase>0xffffff){
    errlogPrintf("data (A24) out of range\n");
    return;
  }

  if(level<0 || level>7){
    errlogPrintf("Level out of range (0->7)\n");
    return;
  }
  if(vec<0 || vec>0xff){
    errlogPrintf("Vector out of range (0->255)\n");
    return;
  }

  card=get566(id);
  if(!!card){
    errlogPrintf("ID %d already exists\n",id);
    return;
  }

  card=malloc(sizeof(xy566));
  if(!card){
    errlogPrintf("Out of memory!\n");
    return;
  }

  memset(&card->dlen,0,sizeof(card->dlen));

  card->id=id;
  card->fail=0;
  card->clk_div=0; /* stc uninitialized */
  ellInit(&card->seq_ctor);

  if(!bipol)
    card->nchan=32;
  else
    card->nchan=16;
  card->ivec=vec;

  card->base_addr=cbase;
  card->data_addr=dbase;

  cb=&card->base;
  db=&card->data_base;

  if(devBusToLocalAddr(atVMEA16, card->base_addr, (volatile void **)cb)){
    errlogPrintf("Failed to map A16 %lx for card %x\n", (unsigned long)card->base_addr,id);
    free(card);
    return;
  }

  if(devBusToLocalAddr(atVMEA24, card->data_addr, (volatile void **)db)){
    errlogPrintf("Failed to map A24 %lx for card %x\n", (unsigned long)card->data_base,id);
    free(card);
    return;
  }

  if(devReadProbe(2, card->base+XY566_CSR, &junk)){
    errlogPrintf("Failed to read A16 %lx for card %x\n", (unsigned long)(card->base+XY566_CSR),id);
    free(card);
    return;
  }

  if(devReadProbe(2, card->data_base, &junk)){
    errlogPrintf("Failed to read A24 %lx for card %x\n", (unsigned long)(card->base+XY566_CSR),id);
    free(card);
    return;
  }

  WRITE16(card->base+XY566_CSR, XY566_SWS); /* Reset */
  /* turn on green and red to indicate init start */
  WRITE16(card->base+XY566_CSR, XY566_CSR_GRN);

  WRITE16(card->base+XY566_RAM, 0);
  WRITE8(card->base+XY566_SEQ, 0);

  card->guard=epicsMutexMustCreate();
  scanIoInit(&card->seq_irq);

  WRITE8(card->base+XY566_VEC, vec);

  devEnableInterruptLevelVME(level);
  assert(devConnectInterruptVME(vec, xycom566isr, card)==0);

  /* Configure card
   * Mode: continuous sequence (default)
   * enable sequence interrupt only
   * 16 bit conversion (default)
   * sequence controller disable (will be enabled during drvsup init)
   */
  WRITE16(card->base+XY566_CSR,
    XY566_CSR_GRN);

  ellAdd(&xy566s,&card->node);
}

void
xycom566finish(void)
{
  ELLNODE *node;
  xy566* card;
  epicsUInt16 csr;
  

  for(node=ellFirst(&xy566s); node; node=ellNext(node))
  {
    card=node2priv(node);

    if(!card->fail){
      if(!!finish566seq(card)){
        errlogPrintf("Board #%d failed to generate samping sequence and will not be used\n",card->id);
        card->fail=1;
      }
    }
    
    if(!card->clk_div){
      errlogPrintf("Board #%d STC not initialized and will not be used\n",card->id);
      card->fail=1;
    }

    if(!!card->fail){
      errlogPrintf("Board #%d failed to initialize and will not be used\n",card->id);

      /* Reset the board again and
       * turn off the LEDS to indicate failure
       */
      WRITE16(card->base+XY566_CSR, XY566_SWS);
      WRITE16(card->base+XY566_CSR, XY566_CSR_RED);
      return;

    }
    
    WRITE16(card->base+XY566_RAM, 0);
    WRITE8(card->base+XY566_SEQ, 0);

    csr=READ16(card->base+XY566_CSR);
    csr|=XY566_CSR_SEQ|XY566_CSR_INA|XY566_CSR_SIE;
    csr|=XY566_CSR_RED|XY566_CSR_GRN;
    WRITE16(card->base+XY566_CSR, csr);
  }

  return;
}

xy566* get566(short id)
{
  ELLNODE *node;
  xy566* card;

  for(node=ellFirst(&xy566s); node; node=ellNext(node))
  {
    card=node2priv(node);
    if(card->id==id)
      return card;
  }

  return NULL;
}

static
void xycom566isr(void *arg)
{
  xy566 *card=arg;
  epicsUInt16 csr=READ16(card->base+XY566_CSR);
  epicsUInt16 datacnt[32];
  epicsUInt16 dcnt;
  size_t i, ch;

  if(!(csr&XY566_CSR_PND))
    return; /* not ours */

  epicsMutexMustLock(card->guard);

  csr=READ16(card->base+XY566_CSR);

  /* disable sequence controller */
  csr&=~XY566_CSR_SEQ;

  /* and acknowledge interrupts */
  WRITE16(card->base+XY566_CSR,
    csr|XY566_CSR_SIP|XY566_CSR_TIP|XY566_CSR_EIP);

  if(csr&XY566_CSR_SIP){

    memset(datacnt,0,sizeof(datacnt));

    /* number of samples taken */
    dcnt=READ16(card->base+XY566_RAM);

    if(dcnt>256){
      /* Somehow the sequence was restart resetting the pointer
       * or changed by an outside program
       */
      dcnt=256;
      errlogPrintf("Data longer then expected\n");
    }

    for(i=0;i<dcnt;i++){
      ch=card->seq[i]&0x1f;

      card->data[ch][datacnt[ch]]=READ16(card->data_base+2*i);
      datacnt[ch]++;

      if( card->seq[i]&SEQ_END )
        break;
    }
  } /* if(csr&XY566_CSR_SIP) */

  csr=READ16(card->base+XY566_CSR);

  /* enable sequence controller */
  csr|=XY566_CSR_SEQ;
  WRITE16(card->base+XY566_CSR, csr);

  epicsMutexUnlock(card->guard);
}

/*********************** DRVET ************************/

drvet drvXycom566 = {2, NULL, NULL};
epicsExportAddress(drvet,drvXycom566);
