#ifndef DRVXY220_H
#define DRVXY220_H 1

/*
 * Header file for drvXy220.c
 */

long xy220_driver(unsigned short card, unsigned long *val, unsigned long mask);
long xy220_read(unsigned short card, unsigned long mask, unsigned long *pval);

#endif /* DRVXY220_H */
