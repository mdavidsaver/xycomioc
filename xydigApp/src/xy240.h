
#ifndef XY240_H
#define XY240_H

/* ID string is every other byte starting at */
#define XY240_ID	0x0B
#  define	XY240_ID_CNT	6
#define XY240_IRQ_INP	0x80
#define XY240_CSR	0x81
#define XY240_IRQ_PEND	0x82
#define XY240_IRQ_MASK	0x83
#define XY240_IRQ_CLR	0x84
#define XY240_IRQ_VEC	0x85
#define XY240_FLAG_OUT	0x86
#define XY240_DIR	0x87
#define XY240_PORT_0	0x88
/* 0 <= N <= 7 */
#define XY240_PORT(N) ( XY240_PORT_0 + (N) )
#  define XY240_NPORTS 8

#endif /* XY240_H */
