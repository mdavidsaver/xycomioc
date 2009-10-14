
#ifndef XY220_H
#define XY220_H

/* ID string is every other byte starting at */
#define  U8_XY220_ID	0x0B
#  define	XY220_ID_CNT	6
#define U16_XY220_CSR	0x80
#  define X220_CSR_RED 0x01 /* 0=on 1=off */
#  define X220_CSR_GRN 0x02 /* 0=of 1=on */
#  define X220_CSR_RST 0x10 /* Reset */
#define  U8_XY220_PORT_0	0x81
/* 0 <= N <= 4 */
#define  U8_XY220_PORT(N) ( U8_XY220_PORT_0 + (N) )
#  define XY220_NPORTS 4

#endif /* XY220_H */
