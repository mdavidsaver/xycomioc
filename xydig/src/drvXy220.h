/*
 * Header file for drvXy220.c
 */

long xy220_driver_init(void);
long xy220_driver(unsigned short card, unsigned long *val, unsigned long mask);
long xy220_read(unsigned short card, unsigned long mask, unsigned long *pval);
long xy220_io_report(int level);
