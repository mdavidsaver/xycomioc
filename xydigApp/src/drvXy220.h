#ifndef DRVXY220_H
#define DRVXY220_H 1

/*
 * Header file for drvXy220.c
 */

#include <epicsTypes.h>

long xy220_driver(unsigned short card, epicsUInt32 *val, epicsUInt32 mask);
long xy220_read(unsigned short card, epicsUInt32 mask, epicsUInt32 *pval);

#endif /* DRVXY220_H */
