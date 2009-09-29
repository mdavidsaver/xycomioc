/*
 * Header file for drvXy240.c
 */

long xy240_init();
long xy240_getioscanpvt(short card, IOSCANPVT *scanpvt);
long xy240_bi_driver(short card, unsigned long mask, unsigned long *prval);
long xy240_bo_read(short card, unsigned long mask, unsigned long *prval);
long xy240_bo_driver(short card, unsigned long val, unsigned long mask);
long xy240_io_report(int level);
