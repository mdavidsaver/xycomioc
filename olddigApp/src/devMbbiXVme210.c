/* devMbbiXVme210.c,v 1.1 2003/08/27 15:21:39 mrk Exp */

/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/* devMbbiXVme210.c - Device Support Routines	*/
/* XYcom 32 bit Multibit binary input		*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 *
 * Modification Log:
 * -----------------
 * .01  11-11-91        jba     Moved set of alarm stat and sevr to macros
 * .02	03-13-92	jba	ANSI C changes
 * .03	02-08-94	mrk	Prevent error message storms
 *      ...
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "errlog.h"
#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "mbbiRecord.h"
#include <epicsExport.h>

#include "drvXy210.h"

/* Create the dset for devAiMbbiXVme210 */
static long init_record();
static long read_mbbi();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_mbbi;
}devMbbiXVme210={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	read_mbbi};
epicsExportAddress(dset,devMbbiXVme210);

static long init_record(struct mbbiRecord *pmbbi)
{

    /* mbbi.inp must be an VME_IO */
    switch (pmbbi->inp.type) {
    case (VME_IO) :
	pmbbi->shft = pmbbi->inp.value.vmeio.signal;
	pmbbi->mask <<= pmbbi->shft;
	break;
    default :
	recGblRecordError(S_db_badField,(void *)pmbbi,
		"devMbbiXVme210 (init_record) Illegal INP field");
	return(S_db_badField);
    }
    return(0);
}

static long read_mbbi(struct mbbiRecord *pmbbi)
{
	struct vmeio	*pvmeio;
	int		status;
	epicsUInt32	value;

	
	pvmeio = (struct vmeio *)&(pmbbi->inp.value);
	status = xy210_driver(pvmeio->card,pmbbi->mask,&value);
	if(status==0) {
		pmbbi->rval = value;
		return(0);
	} else {
                if(recGblSetSevr(pmbbi,READ_ALARM,INVALID_ALARM) && errVerbose
		&& (pmbbi->stat!=READ_ALARM || pmbbi->sevr!=INVALID_ALARM))
			recGblRecordError(-1,(void *)pmbbi,"xy210_driver Error");
		return(2);
	}
	return(status);
}
