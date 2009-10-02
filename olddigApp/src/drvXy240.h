#ifndef DRVXY240_H
#define DRVXY240_H 1

/*
 * Header file for drvXy240.c
 */

long xy240_getioscanpvt(short card, IOSCANPVT *scanpvt);
long xy240_bi_driver(short card, epicsUInt32 mask, epicsUInt32 *prval);
long xy240_bo_read(short card, epicsUInt32 mask, epicsUInt32 *prval);
long xy240_bo_driver(short card, epicsUInt32 val, epicsUInt32 mask);

#endif /* DRVXY240_H */
