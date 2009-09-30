/* drvXy210.c,v 1.1 2003/08/27 15:21:40 mrk Exp */
/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/*
 * Driver support for the Xycom XVME-210 digital input board
 *
 * 	Author:      Bob Dalesio
 * 	Date:        6-13-88
 *
 * Modification Log:
 * -----------------
 * .01	02-09-89	lrd	moved I/O addresses to module_types.h
 * .02	11-20-89	joh 	added call to at5vxi driver
 * .03  09-11-91	 bg	added bi_io_report
 * .04  03-09-92	 bg	added levels to xy210_io_report. Gave 
 *                              xy210_io_report the ability to read raw
 *                              values from card if level > 1  
 * .05	08-10-92	joh	merged include file
 * .06	08-25-92	mrk	made masks a macro
 * .07	08-25-92	mrk	replaced bi_driver by xy210_driver
 * .08	09-15-93	mrk	Made report shorter
 */

/*
 * Code Portions:
 *
 * bi_driver_init  Finds and initializes all binary input cards present
 * bi_driver       Interfaces to the binary input cards present
 */


#include <stdlib.h>
#include <stdio.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <devLib.h>
#include <iocsh.h>
#include <errlog.h>

#include "drvXy210.h"

static long xy210_driver_init(void);
static long xy210_io_report(int level);

struct {
        long    number;
        DRVSUPFUN       report;
        DRVSUPFUN       init;
} drvXy210={
        2,
        xy210_io_report,
        xy210_driver_init
};
epicsExportAddress(drvet,drvXy210);


static struct {char **sc; char *s;} RCSID = {&RCSID.s, 
    "drvXy210.c,v 1.1 2003/08/27 15:21:40 mrk Exp"};

/* Xycom 210 binary input memory structure */
/* Note: the high and low order words are switched from the io card */
struct bi_xy210{
        epicsUInt8   begin_pad[0xc0];        /* nothing until 0xc0 */
        epicsUInt16  low_value;      /* low order 16 bits value */
        epicsUInt16  high_value;     /* high order 16 bits value */
        epicsUInt8   end_pad[0x400-0xc0-4];  /* pad until next card */
};

static unsigned int first_base_addr=0;

/* pointers to the binary input cards */
static volatile struct bi_xy210 **pbi_xy210s;      /* Xycom 210s */
static unsigned int card_count=0;

/*
 * BI_DRIVER_INIT
 *
 * intialization for the binary input cards
 */
static
long xy210_driver_init(void)
{
	int bimode;
        int status;
        short  i;
        volatile struct bi_xy210  *pbi_xy210;

	if(card_count==0){
		errMessage(errlogMajor,"Number of 210 cards not set.  Call xy210setup() before iocInit()");
		return 1;
	}

	pbi_xy210s = (volatile struct bi_xy210 **)
		calloc(card_count, sizeof(*pbi_xy210s));
	if(!pbi_xy210s){
		return 1;
	}

        /* initialize the Xycom 210 binary input cards */
        /* base address of the xycom 210 binary input cards */
	status = devBusToLocalAddr(atVMEA16, first_base_addr,
		(volatile void**)&pbi_xy210);
        if (status != 0){ 
           printf("Addressing error in xy210 driver\n");
           return 1;
        }

        /* determine which cards are present */
        for (i = 0; i <card_count; i++,pbi_xy210++){
          if (devReadProbe(sizeof(short), pbi_xy210, &bimode) ==0){
                pbi_xy210s[i] = pbi_xy210;
          } else {
                pbi_xy210s[i] = 0;
            }
        }
        return (0);
}

/*
 * XY210_DRIVER
 *
 * interface to the xy210 binary inputs
 */
long xy210_driver(unsigned short card, epicsUInt32 mask, epicsUInt32 *prval)
{
	epicsUInt32	work;

                /* verify card exists */
                if (!pbi_xy210s[card]){
                        return (-1);
                }

                /* read */
                work = (pbi_xy210s[card]->high_value << 16)    /* high */
                   + pbi_xy210s[card]->low_value;               /* low */
                
		/* apply mask */

		*prval = work & mask;
                       
	return (0);
}

static
long xy210_io_report(int level)
 { 
   int			card;
   epicsUInt32	 	value;
   
   for (card = 0; card < card_count; card++){
	 if (pbi_xy210s[card]){
           value = (pbi_xy210s[card]->high_value << 16)    /* high */
                   + pbi_xy210s[card]->low_value;               /* low */
           printf("BI: XY210:      card %d value=0x%8.8x\n",card,value);
	}
   }
   return (0);
}

static
void xy210setup(int cards, int start_addr)
{
  card_count=cards;
  first_base_addr=start_addr;
}

/* xy210setup */
static const iocshArg xy210setupArg0 = { "Number of cards",iocshArgInt};
static const iocshArg xy210setupArg1 = { "VME base address of first card",iocshArgInt};
static const iocshArg * const xy210setupArgs[2] =
{
    &xy210setupArg0,&xy210setupArg1
};
static const iocshFuncDef xy210setupFuncDef =
    {"xy210setup",2,xy210setupArgs};
static void xy210setupCallFunc(const iocshArgBuf *args)
{
    xy210setup(args[0].ival,args[1].ival);
}

static
void xy210Register(void)
{
  iocshRegister(&xy210setupFuncDef,xy210setupCallFunc);
}
epicsExportRegistrar(xy210Register);
