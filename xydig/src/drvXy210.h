/*
 * Header file for drvXy210.c
 */

long xy210_driver_init();
long xy210_driver(unsigned short card, unsigned long mask, unsigned long *prval);
long xy210_io_report(int level);
