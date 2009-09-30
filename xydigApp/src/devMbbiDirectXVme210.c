/* devMbbiDirectXVme210.c,v 1.1 2003/08/27 15:21:39 mrk Exp */

/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/* devMbbiDirectXVme210.c - Device Support Routines	*/
/* XYcom 32 bit Multibit binary input		*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Matthew Needes
 *      Date:            10-08-93
 *
 * Modification Log:
 * -----------------
 *   (modification log of devMbbiXVme210 applies)
 *  .01 10-08-93   mcn   added support for diredt mbbi record
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "mbbiDirectRecord.h"
#include <epicsExport.h>

#include "drvXy210.h"

/* Create the dset for devMbbiDirectXVme210 */
static long init_record();
static long read_mbbi();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_mbbi;
}devMbbiDirectXVme210={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	read_mbbi};
epicsExportAddress(dset,devMbbiDirectXVme210);

static long init_record(struct mbbiDirectRecord *pmbbi)
{

    /* mbbi.inp must be an VME_IO */
    switch (pmbbi->inp.type) {
    case (VME_IO) :
	pmbbi->shft = pmbbi->inp.value.vmeio.signal;
	pmbbi->mask <<= pmbbi->shft;
	break;
    default :
	recGblRecordError(S_db_badField,(void *)pmbbi,
		"devMbbiDirectXVme210 (init_record) Illegal INP field");
	return(S_db_badField);
    }
    return(0);
}

static long read_mbbi(struct mbbiDirectRecord *pmbbi)
{
	struct vmeio	*pvmeio;
	int		status;
	epicsUInt32	value;

	
	pvmeio = (struct vmeio *)&(pmbbi->inp.value);
	status = xy210_driver(pvmeio->card,pmbbi->mask,&value);
	if(status==0) {
		pmbbi->rval = value;
	} else {
                recGblSetSevr(pmbbi,READ_ALARM,INVALID_ALARM);
	}
	return(status);
}
