
#include "xy566.h"

#include <epicsThread.h>

/*
 * Commands for manipulationg the AM9513
 * System Timing Controller chip found on
 * the XYCOM 566
 *
 * 566 users should be concerned with
 * 3 of the 5 counter/timers found on
 * the AM9513.
 *
 * #3 - Trigger (Sequence) clock
 * #4 - Sample clock
 * #5 - Event clock
 *
 * Note: The interval of the Sequence clock must
 *       be greater then the Sample clock.
 */

/* The 566 document states that a delay
 * of 1.5us is required between all STC
 * register operations.
 */
#define STCNOOP epicsSleep(1.5e-6);

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

  /* Disable channel */
  *ctrl = 0xC0 | (1<<chan);
  STCNOOP;

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

  /* Load 0 into counter */
  *ctrl = 0x40 | (1<<chan);
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
