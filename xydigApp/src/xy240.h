
#ifndef XY240_H
#define XY240_H

/* ID string is every other byte starting at */
#define U8_XY240_ID	0x0B
#  define	XY240_ID_CNT	6
#define U8_XY240_IRQ_INP	0x80
#define U8_XY240_CSR	0x81
#  define X240_CSR_RED 0x01 /* 0=on 1=off */
#  define X240_CSR_GRN 0x02 /* 0=of 1=on */
#  define X240_CSR_PND 0x04 /* IRQ pending */
#  define X240_CSR_ENA 0x08 /* enable IRQ */
#  define X240_CSR_RST 0x10 /* Reset */
#define U8_XY240_IRQ_PEND	0x82
#define U8_XY240_IRQ_MASK	0x83
#define U8_XY240_IRQ_CLR	0x84
#define U8_XY240_IRQ_VEC	0x85
#define U8_XY240_FLAG_OUT	0x86
#define U8_XY240_DIR	0x87
#define U8_XY240_PORT_0	0x88
/* 0 <= N <= 7 */
#define U8_XY240_PORT(N) ( U8_XY240_PORT_0 + (N) )
#  define XY240_NPORTS 8

#endif /* XY240_H */
