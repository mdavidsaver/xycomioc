
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbDefs.h>
#include <dbScan.h>
#include <epicsMutex.h>
#include <epicsThread.h>
#include <devLib.h>
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
  xy566 *card=malloc(sizeof(xy566));
  volatile epicsUInt8  **cb;
  volatile epicsUInt16 **db;

  card->fail=0;
  card->stop_on_seq_done=0;
  card->clk_div=0; /* stc uninitialized */

  if(!!bipol)
    card->nchan=32;
  else
    card->nchan=16;

  if(!card){
    errlogPrintf("Out of memory!\n");
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
  card->ivec=vec;

  card->base_addr=cbase;
  card->data_base=dbase;

  cb=&card->base;
  db=&card->data_base;

  if(devBusToLocalAddr(atVMEA16, card->base_addr, (volatile void **)cb)){
    errlogPrintf("Failed to map A16 %lx for card %x\n", (unsigned long)card->base_addr,id);
    free(card);
    return;
  }

  if(devBusToLocalAddr(atVMEA24, card->data_base, (volatile void **)db)){
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

  csr=card->base+XY566_CSR;

  *(card->base+XY566_CSR)=XY566_SWS; /* Reset */
  *(card->base+XY566_CSR)=XY566_CSR_GRN; /* turn on green and red to indicate init start */

  *(card->base+XY566_RAM)=0;
  *(card->base+XY566_SEQ)=0;

  *(card->base+XY566_VEC)=vec;

  devEnableInterruptLevelVME(level);
  devConnectInterruptVME(vec, xycom566isr, card);

  card->guard=epicsMutexMustCreate();
  scanIoInit(&card->trig_irq);
  scanIoInit(&card->seq_irq);
  scanIoInit(&card->evt_irq);

  ellAdd(&xy566s,card->node);
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
  epicsUInt16 csr=*(card->base+XY566_CSR);

  if(!(csr&XY566_CSR_PND))
    return; /* not ours */


  epicsMutexMustLock(&card->guard);

  csr=*(card->base+XY566_CSR);

  /* disable sequence controller */
  if( card->stop_on_seq_done )
    csr&=~XY566_CSR_SEQ;

  epicsMutexUnlock(&card->guard);
}

