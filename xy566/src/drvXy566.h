/*
 * Header file for drvXy566.c
 */
#include	<dbScan.h>

#define XY566SE         0       /* & Xycom 12-bit Single Ended Scanned*/
#define XY566DI         1       /* &% Xycom 12-bit Differential Scanned */
#define XY566DIL        2       /* &% Xycom 12-bit Differential Latched */
#define NUM_TYPES       3

#define XY566WF         0       /* & Xycom 566 as a waveform */

typedef void(*pcb_fn)(void*,int,volatile unsigned char*);

int ai_xy566_getioscanpvt(unsigned short card, IOSCANPVT *scanpvt);
int ai_xy566_driver(
    short	   card,
    short	   chan,
    unsigned int   type,
    unsigned short *prval
);
int xy566_driver(
    unsigned short slot,
    pcb_fn         pcbroutine,
    void           *parg
);
