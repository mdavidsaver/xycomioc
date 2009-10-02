/* drvXy220.c,v 1.1 2003/08/27 15:21:40 mrk Exp */
/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/*
 * subroutines that are used to interface to the binary output cards
 *
 * 	Author:      Bob Dalesio
 * 	Date:        5-26-88
 *
 *
 * Modification Log:
 * -----------------
 * .01	10-31-91	bg	broke xy220 driver out of bo_driver.c 
 *				broke xy220 code out of io_report and
 *                              created xy220_io_report(). 
 *                              Added sysBusToLocalAdrs.
 * .02	02-03-92	bg	Gave xy220_io_report the ability to 
 *				read raw values from card if level > 1.
 * .03	08-10-92	joh	merged potions of bo_driver.h
 * .04	08-25-92	mrk	made masks a macro
 */

static struct {char **ps; char *s;} RCSID = {&RCSID.s,
    "drvXy220.c,v 1.1 2003/08/27 15:21:40 mrk Exp"};

/*
 * Code Portions:
 *
 * bo_drv_init	Finds and initializes all binary output cards present
 * bo_driver	Interfaces to the binary output cards present
 */

#include <stdlib.h>
#include <stdio.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <devLib.h>
#include <iocsh.h>
#include <errlog.h>

#include "drvXy220.h"

static long xy220_driver_init(void);
static long xy220_io_report(int level);

struct {
        long    number;
        DRVSUPFUN       report;
        DRVSUPFUN       init;
} drvXy220={
        2,
        xy220_io_report,
        xy220_driver_init};
epicsExportAddress(drvet,drvXy220);


#define XY_LED          0x3     /* set the Xycom status LEDs to 0 */
#define XY_SOFTRESET    0x10    /* reset the IO card */

/* Xycom 220 binary output memory structure */
struct bo_xy220{
        epicsUInt8    begin_pad[0x80];        /* nothing until 0x80 */
        epicsUInt16   csr;                    /* control status register */
        epicsUInt16   low_value;       /* low order 16 bits value */
        epicsUInt16   high_value;      /* high order 16 bits value */
        epicsUInt8    end_pad[0x400-0x80-6];  /* pad until next card */
};

static epicsUInt32 first_base_addr=0;

/* pointers to the binary output cards */
volatile struct bo_xy220	**pbo_xy220s;	/* Xycom 220s */
static epicsUInt32 card_count=0;


/*
 * XY220_DRIVER_INIT
 *
 * intialization for the xy220 binary output cards
 */
static
long xy220_driver_init(void)
{
	int bomode;
        int status;
	short	i;
	struct bo_xy220	*pbo_xy220;

	if(card_count==0){
		errMessage(errlogMajor,"Number of 220 cards not set.  Call xy220setup() before iocInit()");
		return 1;
	}

	pbo_xy220s = (volatile struct bo_xy220 **)
		calloc(card_count, sizeof(*pbo_xy220s));
	if(!pbo_xy220s){
		return 1;
	}

	/* initialize the Xycom 220 binary output cards */
	/* base address of the xycom 220 binary output cards */
	status = devBusToLocalAddr(atVMEA16, first_base_addr,
		(volatile void**)&pbo_xy220);
        if (status != 0){
           printf("%s: xy220 A16 base map failure\n", __FILE__);
           return 1;
        }

	/* determine which cards are present */
	for (i = 0; i < card_count; i++,pbo_xy220++){
	    if (devReadProbe(sizeof(short), pbo_xy220, &bomode) ==0){
		pbo_xy220s[i] = pbo_xy220;
		pbo_xy220s[i]->csr = XY_SOFTRESET;
		pbo_xy220s[i]->csr = XY_LED;
	    }else{
		pbo_xy220s[i] = 0;
	    }
	}
	return(0);
}

/*
 * XY220_DRIVER
 *
 * interface to the xy220 binary outputs
 */

long xy220_driver(unsigned short card, epicsUInt32 *val, epicsUInt32 mask)
{
	epicsUInt32 work;

	/* verify card exists */
	if (!pbo_xy220s[card])
		return (-1);

	/* use structure to handle high and low short swap */
	/* get current output */
	work = (pbo_xy220s[card]->high_value << 16)
		 + pbo_xy220s[card]->low_value;

	/* alter specified bits */
	work = (work & ~mask) | ((*val) & mask);

	/* write new output */
	pbo_xy220s[card]->high_value = (unsigned short)(work >> 16);
	pbo_xy220s[card]->low_value = (unsigned short)work;

return (0);
}

/*
 * xy220_read
 * 
 * read the binary output
 */
long xy220_read(unsigned short card, epicsUInt32 mask, epicsUInt32 *pval)
{

	/* verify card exists */
	if (!pbo_xy220s[card])
		return (-1);
	/* readback */
	*pval = (pbo_xy220s[card]->high_value << 16) + pbo_xy220s[card]->low_value;  

	*pval &= mask; 

        return(0);

}

#define masks(K) ((1<<K))
/*
 *   XY220_IO_REPORT 
 *
 *
*/

static
long xy220_io_report(int level)
{
   int		card;
   epicsUInt32	value;

   for (card = 0; card < card_count; card++){
	if (pbo_xy220s[card]){
	   value = (pbo_xy220s[card]->high_value << 16)    /* high */
		+ pbo_xy220s[card]->low_value;               /* low */
           printf("BO: XY220:      card %d value=0x%8.8x\n",card,value);
        }
   }  
   return (0);
 }

void xy220setup(int cards, int start_addr)
{
  card_count=cards;
  first_base_addr=start_addr;
}

/* xy220setup */
static const iocshArg xy220setupArg0 = { "Number of cards",iocshArgInt};
static const iocshArg xy220setupArg1 = { "VME base address of first card",iocshArgInt};
static const iocshArg * const xy220setupArgs[2] =
{
    &xy220setupArg0,&xy220setupArg1
};
static const iocshFuncDef xy220setupFuncDef =
    {"xy220setup",2,xy220setupArgs};
static void xy220setupCallFunc(const iocshArgBuf *args)
{
    xy220setup(args[0].ival,args[1].ival);
}

static
void xy220Register(void)
{
  iocshRegister(&xy220setupFuncDef,xy220setupCallFunc);
}
epicsExportRegistrar(xy220Register);
