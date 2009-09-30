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


#include <vxWorks.h>
#include <stdlib.h>
#include <stdio.h>
#include <vxLib.h>
#include <sysLib.h>
#include <vme.h>
#include <module_types.h>
#include <drvSup.h>
#include <epicsExport.h>

#include "drvXy210.h"

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

#define MAX_XY_BI_CARDS	(bi_num_cards[XY210]) 

/* Xycom 210 binary input memory structure */
/* Note: the high and low order words are switched from the io card */
struct bi_xy210{
        char    begin_pad[0xc0];        /* nothing until 0xc0 */
        unsigned short  low_value;      /* low order 16 bits value */
        unsigned short  high_value;     /* high order 16 bits value */
        char    end_pad[0x400-0xc0-4];  /* pad until next card */
};


/* pointers to the binary input cards */
struct bi_xy210 **pbi_xy210s;      /* Xycom 210s */

/* test word for forcing bi_driver */
int	bi_test;

static char *xy210_addr;


/*
 * BI_DRIVER_INIT
 *
 * intialization for the binary input cards
 */
long xy210_driver_init()
{
	int bimode;
        int status;
        register short  i;
        struct bi_xy210  *pbi_xy210;

	pbi_xy210s = (struct bi_xy210 **)
		calloc(MAX_XY_BI_CARDS, sizeof(*pbi_xy210s));
	if(!pbi_xy210s){
		return ERROR;
	}

        /* initialize the Xycom 210 binary input cards */
        /* base address of the xycom 210 binary input cards */
        if ((status = sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO,(char *)(long)bi_addrs[XY210],&xy210_addr)) != OK){ 
           printf("Addressing error in xy210 driver\n");
           return ERROR;
        }
        pbi_xy210 = (struct bi_xy210 *)xy210_addr;  
        /* determine which cards are present */
        for (i = 0; i <bi_num_cards[XY210]; i++,pbi_xy210++){
          if (vxMemProbe((char *)pbi_xy210,READ,sizeof(short),(char *)&bimode) == OK){
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
long xy210_driver(unsigned short card, unsigned long mask, unsigned long *prval)
{
	register unsigned int	work;

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

long xy210_io_report(int level)
 { 
   int			card;
   unsigned int 	value;
   
   for (card = 0; card < bi_num_cards[XY210]; card++){
	 if (pbi_xy210s[card]){
           value = (pbi_xy210s[card]->high_value << 16)    /* high */
                   + pbi_xy210s[card]->low_value;               /* low */
           printf("BI: XY210:      card %d value=0x%8.8x\n",card,value);
	}
   }
   return (0);
}
