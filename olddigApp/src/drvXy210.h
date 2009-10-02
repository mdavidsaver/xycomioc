#ifndef DRVXY210_H
#define DRVXY210_H 1

/*
 * Header file for drvXy210.c
 */

#include <epicsTypes.h>

long xy210_driver(unsigned short card, epicsUInt32 mask, epicsUInt32 *prval);

#endif /* DRVXY210_H */
