#ifndef DRVXY240_H
#define DRVXY240_H 1

/*
 * Header file for drvXy240.c
 */

long xy240_getioscanpvt(short card, IOSCANPVT *scanpvt);
long xy240_bi_driver(short card, unsigned long mask, unsigned long *prval);
long xy240_bo_read(short card, unsigned long mask, unsigned long *prval);
long xy240_bo_driver(short card, unsigned long val, unsigned long mask);

#endif /* DRVXY240_H */
