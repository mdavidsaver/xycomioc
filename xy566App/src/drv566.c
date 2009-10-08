
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

  card->fail=0;
  card->stop_on_seq_done=0;
  card->clk_div=0; /* stc uninitialized */
  ellInit(&card->seq_ctor);

  if(!!bipol)
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
  WRITE16(card->base+XY566_SEQ, 0);

  WRITE16(card->base+XY566_VEC, vec);

  devEnableInterruptLevelVME(level);
  devConnectInterruptVME(vec, xycom566isr, card);

  card->guard=epicsMutexMustCreate();
  scanIoInit(&card->seq_irq);

  ellAdd(&xy566s,&card->node);
}

static
long
xycom566_init(void)
{
  ELLNODE *node;
  xy566* card;
  

  for(node=ellFirst(&xy566s); node; node=ellNext(node))
  {
    card=node2priv(node);
    epicsUInt16 csr;

    if(!card->fail){
      if(!!finish566seq(card)){
        errlogPrintf("Board #%d failed to generate samping sequence and will not be used\n",card->id);
        card->fail=1;
      }
    }

    if(card->fail)
      errlogPrintf("Board #%d failed to initialize and will not be used\n",card->id);

    else if(!card->clk_div)
      errlogPrintf("Board #%d STC not initialized and will not be used\n",card->id);

    if(card->fail || !card->clk_div){
      /* Reset the board again and
       * turn off the LEDS to indicate failure
       */
      WRITE16(card->base+XY566_CSR, XY566_SWS);
      WRITE16(card->base+XY566_CSR, XY566_CSR_RED);

    }else{
      csr=READ16(card->base+XY566_CSR);
      WRITE16(card->base+XY566_CSR, csr|XY566_CSR_RED|XY566_CSR_GRN);

    }
  }

  return 0;
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

  if(!(csr&XY566_CSR_PND))
    return; /* not ours */


  epicsMutexMustLock(card->guard);

  csr=READ16(card->base+XY566_CSR);

  /* disable sequence controller */
  if( card->stop_on_seq_done )
    csr&=~XY566_CSR_SEQ;

  epicsMutexUnlock(card->guard);
}

/*********************** DRVET ************************/

drvet drvXycom566 = {2, NULL, xycom566_init};
epicsExportAddress(drvet,drvXycom566);


/********************** IOCSH *************************/

/* xycom566setup */
static const iocshArg xycom566setupArg0 = { "Card id #",iocshArgInt};
static const iocshArg xycom566setupArg1 = { "Config (A16) base",iocshArgInt};
static const iocshArg xycom566setupArg2 = { "Data (A24) base",iocshArgInt};
static const iocshArg xycom566setupArg3 = { "IRQ Level",iocshArgInt};
static const iocshArg xycom566setupArg4 = { "IRQ Vector",iocshArgInt};
static const iocshArg xycom566setupArg5 = { "0 - 16 chan, 1 - 32 chan",iocshArgInt};
static const iocshArg * const xycom566setupArgs[6] =
{
    &xycom566setupArg0,&xycom566setupArg1,&xycom566setupArg2,
    &xycom566setupArg3,&xycom566setupArg4,&xycom566setupArg5
};
static const iocshFuncDef xycom566setupFuncDef =
    {"xycom566setup",6,xycom566setupArgs};
static void xycom566setupCallFunc(const iocshArgBuf *args)
{
    xycom566setup(args[0].ival,args[1].ival,args[2].ival,
                  args[3].ival,args[4].ival,args[5].ival);
}

static
void drv566Register(void)
{
  iocshRegister(&xycom566setupFuncDef,xycom566setupCallFunc);
}
epicsExportRegistrar(drv566Register);