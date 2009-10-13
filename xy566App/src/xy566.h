
#ifndef XY566_H_INC
#define XY566_H_INC 1

#include <epicsTypes.h>
#include <ellLib.h>
#include <dbScan.h>
#include <epicsMutex.h>
#include <callback.h>

#define XY566_CSR 0x80
#  define XY566_CSR_RED 0x0001
#  define XY566_CSR_GRN 0x0002
#  define XY566_CSR_PND 0x0004 /* IRQ pending */
#  define XY566_CSR_INA 0x0008 /* IRQ enable */
#  define XY566_CSR_RST 0x0010 /* soft reset */
#  define XY566_CSR_MDE 0x0020 /* seqential/random mode */
#  define XY566_CSR_IMG 0x0040 /* image/contiguous mode */
#  define XY566_CSR_SRT 0x0080 /* Start */
#  define XY566_CSR_SEQ 0x0100 /* seq. controller enable */
#  define XY566_CSR_12B 0x0200 /* 8 bit/12 bit */
#  define XY566_CSR_TIE 0x0400 /* trig. irq enable */
#  define XY566_CSR_TIP 0x0800 /* trig. irq pending */
#  define XY566_CSR_SIE 0x1000 /* seq. irq enable */
#  define XY566_CSR_SIP 0x2000 /* seq. irq pending */
#  define XY566_CSR_EIE 0x4000 /* evt. irq enable */
#  define XY566_CSR_EIP 0x8000 /* evt. irq pending */
#define XY566_SWS 0x82 /* software start */
#define XY566_VEC 0x83 /* VME IRQ vector */
#define XY566_RAM 0x84 /* data ram A24 pointer */
#define XY566_SEQ 0x87 /* sequence ram pointer */
#define XY566_STD 0xC0 /* STC Data port */
#define XY566_STC 0xC3 /* STC Control port */
#define XY566_GRB 0x101 /* Gain RAM base */
#define XY566_GAIN(N) ( XY566_GRB + 2*(N) )
#define XY566_SRB 0x201 /* Seqence ram base */
#define XY566_SEQR(N) ( XY566_SRB + 2*(N) )

#define XY566_DOFF(N) ( (N) ) /* Data ram offset of N sample */


/* Special bits in sequence ram */
#define SEQ_IRQ (1<<5)
#define SEQ_STP (1<<6)
#define SEQ_END (1<<7)

/* per device struct */
typedef struct {
  ELLNODE node; /* must be first */
  short id;

  /* Set by a config function to indicate failure
   * to prevent device support from using a
   * half configured device.
   */
  int fail:1;

  /* divider for counter clock.
   * All times are a multiple of .25us times this value.
   */
  unsigned int clk_div;

  unsigned char ivec;

  epicsUInt32 base_addr;
  volatile epicsUInt8* base;

  epicsUInt32 data_addr;
  volatile epicsUInt16* data_base;

  epicsMutexId guard;

  unsigned int nchan; /* either 16 or 32 */

  CALLBACK cb_irq;
  IOSCANPVT seq_irq;

  /* Holds the sampling sequence last set to
   * the card.  Needed when interpreting
   * downloaded data.
   */
  epicsUInt8 seq[256];

  /* Array and length of channel data */
  epicsUInt16 *data[32];
  epicsUInt16 dlen[32];

  /* Used by sequence constructor
   * during initialization only.
   */
  ELLLIST seq_ctor;
} xy566;

#define node2priv(n) ( (xy566*)(n) )

xy566* get566(short id);

int finish566seq(xy566*);

#define WRITE16(addr, val) ( *(volatile epicsUInt16*)(addr) = (val) )
#define WRITE8(addr, val) ( *(volatile epicsUInt8*)(addr) = (val) )

#define READ16(addr) ( *(volatile epicsUInt16*)(addr) )
#define READ8(addr) ( *(volatile epicsUInt8*)(addr) )

/* IOCSH stuff */

extern int dbg566;

void
xycom566setup(
      int id,
      int cbase,
      int dbase,
      int level,
      int vec,
      int bipol
);

void
xycom566finish(void);

void
stc566simple(int id, int div, int period);

void
seq566set(int id, int ch, int nsamp, int ord, int prio);

#endif /* XY566_H_INC */
