
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

/* The 566 document states that a delay
 * of 1.5us is required between all STC
 * register operations.
 */
#define STCNOOP epicsSleep(1.5e-6);

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
void
stcreset(xy566 *card, int div)
{
  volatile epicsUInt8 *ctrl=card->base+XY566_STC;
  volatile epicsUInt16 *data=card->base+XY566_STD;
  epicsUInt16 mmreg=0;

  *ctrl=0xff; /* Reset */
  STCNOOP;
  *ctrl=0x5f; /* Reload all counters from load registers (0 after reset) */
  STCNOOP;
  *ctrl=0xef; /* set 16 bit mode */
  STCNOOP;

  /* Configure the master mode register
   * See page 1-9 of AMD AM9513 manual
   */

  /*
   * 0x8000 - 0 - binary count mode, 1 - BCD
   * 0x4000 - 0 - data pointer auto increment
   * 0x2000 - 1 - 16 bit bus
   * 0x1000 - 0 - Fout enable
   */
  mmreg|=0x2000;

  div&=0xf;
  /* 0x0F00 - Clock source divider
   *  0 = 16
   *  1 = 1
   *  ...
   *  f = 15
   */
  mmreg|=div<<8;

  /* 0x00F00 - Clock source select
   * 0 - F1
   * 1 -> 5   - SRC 1->5
   * 6 -> 10  - GATE 1->5
   * 11 -> 15 - F 1->5
   * Note: 0 and 11 are equivalent
   */

  mmreg|=0x0000 /* F1 (external clock no division) */

  /* Disable compare and Time of Day */
  mmreg|=0x0000;

  *ctrl=0x17; /* Load MM pointer */
  STCNOOP;

  *data=mmreg;

  card->clk_div=1;
  return 0;
}

/* Disable channel
 * chan - 1->5
 */
static
int
stcdisable(xy566 *card, int chan)
{
  volatile epicsUInt8 *ctrl=card->base+XY566_STC;
  volatile epicsUInt16 *data=card->base+XY566_STD;

  /* Move pointer to channel mode */
  *ctrl = chan;
  STCNOOP;

  /* Source:  F1
     Gate: None
     Count once, down
     Toggle output on TC
     Reload from Load register
   */
  *data = 0x0b02;
  STCNOOP;

  /* pointer auto moves to Load reg */
  *data = 0;
  STCNOOP;

  /* Set TC Toggle output (high)
   * when mode includes 'Toggle on TC'
   * this sets the initial state
   */
  *ctrl = 0xe8 | (chan&0x7);

  /* Since the counter is not armed it will stay high */
}

/*
 * Configure the System Timing Controller on the 566
 * in a simple mode.
 * Only the sample clock is enabled with a period of
 * N/f.  Where f=4MHz/div.
 *
 * This is useful when using the software trigger register
 */
void
stc566simple(int id, int div int period)
{
  xy566 *card=get566(id);
  volatile epicsUInt8 *ctrl;
  volatile epicsUInt16 *data;

  if(!card){
    errlogPrintf("Invalid ID\n");
    return;
  }

  if(div<1 || div>16){
    errlogPrintf("STC divider out of range (1->16)\n");
    return 1;
  }

  epicsMutexMustLock(&card->guard);

  ctrl=card->base+XY566_STC;
  data=card->base+XY566_STD;

  stcreset(card,div);
  STCNOOP;

  stcdisable(card,2); /* disable sequence trigger clock */
  STCNOOP;

  stcdisable(card,5); /* disable event clock */
  STCNOOP;

  *ctrl = 4; /* ptr to sample clock mode reg */
  STCNOOP;

  /* 0xe000 - 8 - Use gate 4 (open when sequence controller runs)
   * 0x1000 - 1 - falling edge
   * 0x0F00 - 5 - Source: SRC 5 (externally connected to Fout)
   * 0x0080 - 1 - Reset when gate opens
   * 0x0040 - 0 - Reload from Load
   * 0x0020 - 1 - auto reload on TC
   * 0x0010 - 0 - binary count
   * 0x0008 - 0 - count down
   * 0x0007 - 5 - active low TC pulse
   */
  *data = 0x95A5;
  STCNOOP;

  /* pointer auto moves to Load reg */
  *data = period&0xFfff;
  STCNOOP;

  /* Set TC Toggle output (high)
   * when mode includes 'Toggle on TC'
   * this sets the initial state
   */
  *ctrl = 0xec;
  STCNOOP;

  /* Arm counter 4 */
  *ctrl = 0x68;
  STCNOOP;

  epicsMutexUnlock(&card->guard);
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

