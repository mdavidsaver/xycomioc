/* drvXy240.c,v 1.1 2003/08/27 15:21:40 mrk Exp */
/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/*
 *	routines used to test and interface with Xycom240
 *	digital i/o module
 *
 * 	Author:      Bob Dalesio
 * 	Date:        11/20/91
 * Modification Log:
 * -----------------
 * .01	06-25-92	bg	Added driver to code.  Added xy240_io_report
 *				to it. Added copyright disclaimer.
 * .02	08-10-92	joh	merged xy240_driver.h into this source
 * .03	08-11-92	joh	fixed use of XY240 where XY240_BI or XY240_BO
 *				should have been used
 * .04  08-11-92	joh	now allows for runtime reconfiguration of
 *				the addr map
 * .05  08-25-92        mrk     added DSET; made masks a macro
 * .06  08-26-92        mrk     support epics I/O event scan
 * .07	08-26-92	joh 	task params from task params header
 * .08	08-26-92	joh 	removed STDIO task option	
 * .09	08-26-92	joh 	increased stack size for V5
 * .10	08-26-92	joh 	increased stack size for V5
 * .11	08-27-92	joh	fixed no status return from bo driver
 * .12	09-03-92	joh	fixed wrong index used when testing for card
 *				present 
 * .13	09-03-92	joh	fixed structural problems in the io
 *				report routines which caused messages to
 *				be printed even when no xy240's are present 
 * .14	09-17-92	joh	io report now tabs over detailed info
 * .15	09-18-92	joh	documentation
 * .16	08-02-93	mrk	Added call to taskwdInsert
 * .17	08-04-93	mgb	Removed V5/V4 and EPICS_V2 conditionals
 */

#include <stdlib.h>
#include <stdio.h>

#include "drvSup.h"
#include "dbDefs.h"
#include "dbScan.h"
#include "taskwd.h"
#include <epicsThread.h>
#include <devLib.h>

#include "dbAccess.h"
#include <epicsExport.h>
#include "drvXy240.h"

#define masks(K) ((1<<K))

/*xy240 memory structure*/
struct dio_xy240{
        char            begin_pad[0x80];        /*go to interface block*/
        unsigned short  csr;                    /*control status register*/
        unsigned short  isr;                    /*interrupt service routine*/
        unsigned short  iclr_vec; 
              /*interrupt clear/vector*/
        unsigned short  flg_dir;                /*flag&port direction*/
        unsigned short  port0_1;                /*port0&1 16 bits value*/
        unsigned short  port2_3;                /*por2&3 16 bits value*/
        unsigned short  port4_5;                /*port4&5 16 bits value*/
        unsigned short  port6_7;                /*port6&7 16 bits value*/
        char            end_pad[0x400-0x80-16]; /*pad til next card*/
};

/*create dio control structure record*/
struct dio_rec
        {
        volatile struct dio_xy240 *dptr;                 /*pointer to registers*/
        short num;                              /*device number*/
        short mode;                             /*operating mode*/
        unsigned short sport0_1;                /*saved inputs*/
        unsigned short sport2_3;                /*saved inputs*/
        IOSCANPVT ioscanpvt;
        /*short dio_vec;*/                      /*interrupt vector*/
        /*unsigned int intr_num;*/              /*interrupt count*/
        };


static unsigned int first_base_addr=0;

static struct dio_rec *dio;	/*define array of control structures*/
static unsigned int card_count=0;

static long xy240_init(void);
static long xy240_io_report(int level);

struct {
        long    number;
        DRVSUPFUN       report;
        DRVSUPFUN       init;
} drvXy240={
        2,
        xy240_io_report,
        xy240_init};
epicsExportAddress(drvet,drvXy240);


/*
 *
 *dio_scan
 *
 *task to check for change of state
 *
 */
static
void dio_scan(void *ignore)
{	
	int i;
	int	first_scan,first_scan_complete;

 for(;;){
    if(interruptAccept) break;
    epicsThreadSleep(0.01); /* 0.01 is arbitrary */
 }
 first_scan_complete = FALSE;
 first_scan = TRUE;
 for (;;) 
 {
   for (i = 0; i < card_count; i++)
    {
    if (dio[i].dptr)
     if (((dio[i].dptr->port0_1) ^ (dio[i].sport0_1)) || 
		((dio[i].dptr->port2_3) ^ (dio[i].sport2_3))
		|| first_scan)
      {
	 /* printf("io_scanner_wakeup for card no %d\n",i); */
          scanIoRequest(dio[i].ioscanpvt);
	  dio[i].sport0_1 = dio[i].dptr->port0_1;
	  dio[i].sport2_3 = dio[i].dptr->port2_3;	  
	  }
    }
    if (first_scan){
	first_scan = 0;
	first_scan_complete = 1;
    }
    epicsThreadSleep(0.01);
  }
}

static epicsThreadId scanner;

/*DIO DRIVER INIT
 *
 *initialize xy240 dig i/o card
 */
long xy240_init()
{
	short 			junk;
	register short 		i;
	struct dio_xy240	*pdio_xy240;
	int			status;
	int			at_least_one_present = FALSE;

	/*
	 * allow for runtime reconfiguration of the
	 * addr map
	 */
	dio = (struct dio_rec *) calloc(card_count, sizeof(*dio));
	if(!dio){
		return 1;
	}

	status = devBusToLocalAddr(atVMEA16, first_base_addr,
		(volatile void**)&pdio_xy240);
        if (status != 0){
           	printf("%s: Unable to map the XY240 A16 base addr\n", __FILE__);
           	return 1;
        }

        for (i = 0; i < card_count; i++, pdio_xy240++){

		if (devReadProbe(sizeof(short), pdio_xy240, &junk) ==0){
	   		dio[i].dptr = 0;
    			continue;
		} 

		/*
		 * 	register initialization
		 */
		pdio_xy240->csr = 0x3;
		pdio_xy240->iclr_vec = 0x00;	/*clear interrupt input register latch*/
		pdio_xy240->flg_dir = 0xf0;	/*ports 0-3,input;ports 4-7,output*/
		dio[i].sport2_3 = pdio_xy240->port2_3;	/*read and save high values*/
                dio[i].dptr = pdio_xy240;
		at_least_one_present = TRUE;
                scanIoInit(&dio[i].ioscanpvt);
	}
				
 	if (at_least_one_present) 
  	{

		scanner = epicsThreadCreate("240scan",
			epicsThreadPriorityMedium,
			epicsThreadGetStackSize(epicsThreadStackMedium),
			dio_scan,
			NULL
		);
   		if (!scanner){
  			printf("Unable to create XY240 scan task\n");
			return 0;
		}
  	}

	return 0;

} 	

long xy240_getioscanpvt(short card, IOSCANPVT *scanpvt)
{
        if ((card >= card_count) || (!dio[card].dptr)) return(0);
        *scanpvt = dio[card].ioscanpvt;
        return(0);
}


/*
 * XY240_BI_DRIVER
 *
 *interface to binary inputs
 */

long xy240_bi_driver(short card, unsigned long mask, unsigned long *prval)
{
	register unsigned int	work;

	if ((card >= card_count) || (!dio[card].dptr)) 
	 return -1;
	work = (dio[card].dptr->port0_1 << 16)
	 + dio[card].dptr->port2_3;
	*prval = work & mask;

	return(0);
}

/*
 *
 *XY240_BO_READ
 *
 *interface to binary outputs
 */

long xy240_bo_read(short card, unsigned long mask, unsigned long *prval)
{
	unsigned long	work;
 
	if ((card >= card_count) || (!dio[card].dptr)){ 
		 return -1;
        }
              
	/* printf("%d\n",dio[card].num); */
	work = (dio[card].dptr->port4_5 << 16)
	 + dio[card].dptr->port6_7;
                 		
	*prval = work &= mask;

        return(0);
 }

/* XY240_DRIVER
 *
 *interface to binary outputs
 */

long xy240_bo_driver(short card, unsigned long val, unsigned long mask)
{
	unsigned long	work;

 	if ((card >= card_count) || (!dio[card].dptr)) 
		 return 1;

	/* use structure to handle high and low short swap */
	/* get current output */

	work = (dio[card].dptr->port4_5 << 16)
			 + dio[card].dptr->port6_7;

	work = (work & ~mask) | (val & mask);

	dio[card].dptr->port4_5 = (unsigned short)(work >> 16);
	dio[card].dptr->port6_7 = (unsigned short)work;

	return 0;
 }


/*dio_out
 *
 *test routine for xy240 output 
 */
int xy240_dio_out(short card,short port,short val)
{

 if ((card > card_count-1)) 		/*test to see if card# is allowable*/
  {
  printf("card # out of range\n");
  return -1;
  }
 else if (!dio[card].dptr)				/*see if card exists*/
  {
  printf("can't find card %d\n", card);
  return -2;
  }
 else if ((port >7) || (port <4))			/*make sure they're output ports*/
  {
  printf("use ports 4-7\n");
  return -3;
  }
 else if (port == 4)
  {
  dio[card].dptr->port4_5 = val<< 8;
  return -4;
  }
 else if (port == 5)
  {
  dio[card].dptr->port4_5 = val;
  return -5;
  }
 else if (port == 6)
  {
  dio[card].dptr->port6_7 = val<< 8;
  return -6;
  }
else if (port == 7)
  {
  dio[card].dptr->port6_7 = val;
  return -7;
  }
else{
  printf("use ports 4-7\n");
  return -8;
  }
}
 
/*XY240_WRITE
 *
 *command line interface to test bo driver
 *
 */
int xy240_write(card,val)
        short card;
        unsigned int val;
 {
  return xy240_bo_driver(card,val,0xffffffff);
 }
 

                          
void xy240_bi_io_report(int card)
{
        short int num_chans,j;
        unsigned long val;

        num_chans = 32;

        if(!dio[card].dptr){
		return;
	}

	printf("\tXY240 BINARY IN CHANNELS:\n");
	for(	j=0; j < num_chans; j++){

		xy240_bi_driver(card,masks(j),&val);
		printf("\tChan %d = %lx\t ",j,val);

		if(j%4==3)
			printf("\n");
	}
}


void xy240_bo_io_report(int card)
{
        short int num_chans,j;
        unsigned long val;

        num_chans = 32;

	if(!dio[card].dptr){
		return;
	}

	printf("\tXY240 BINARY OUT CHANNELS:\n");

	for(	j=0; j < num_chans; j++){

		xy240_bo_read(card,masks(j),&val);
		printf("\tChan %d = %lx\t ",j,val);

		if(j%4==3)
			printf("\n");
	}
}


long xy240_io_report(int level)
{
  	int card;

        for (card = 0; card < card_count; card++){
        
                if(dio[card].dptr){
                   printf("B*: XY240:\tcard %d\n",card);
                   if (level >= 1){
			xy240_bi_io_report(card);
			xy240_bo_io_report(card);
                   }
                }
        }
        return(0); 
}

