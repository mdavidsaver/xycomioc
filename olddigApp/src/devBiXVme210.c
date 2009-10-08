/* devBiXVme210.c,v 1.1 2003/08/27 15:21:39 mrk Exp */

/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/* devBiXVme210.c - Device Support Routines for XYcom 32 bit binary input*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 *
 *
 * Modification Log:
 * -----------------
 * .01  11-11-91        jba     Moved set of alarm stat and sevr to macros
 * .02	03-13-92	jba	ANSI C changes
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
#include "biRecord.h"
#include <epicsExport.h>

#include "drvXy210.h"

/* Create the dset for devBiXVme210 */
static long init_record();
static long read_bi();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_bi;
}devBiXVme210={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	read_bi};
epicsExportAddress(dset,devBiXVme210);

static long init_record(struct biRecord *pbi)
{
    struct vmeio *pvmeio;

    /* bi.inp must be an VME_IO */
    switch (pbi->inp.type) {
    case (VME_IO) :
	pvmeio = (struct vmeio *)&(pbi->inp.value);
	pbi->mask=1;
	pbi->mask <<= pvmeio->signal;
	break;
    default :
	recGblRecordError(S_db_badField,(void *)pbi,
		"devBiXVme210 (init_record) Illegal INP field");
	return(S_db_badField);
    }
    return(0);
}

static long read_bi(struct biRecord *pbi)
{
	struct vmeio *pvmeio;
	int	    status;
	epicsUInt32 value;

	
	pvmeio = (struct vmeio *)&(pbi->inp.value);
	status = xy210_driver(pvmeio->card,pbi->mask,&value);
	if(status==0) {
		pbi->rval = value;
		return(0);
	} else {
                if(recGblSetSevr(pbi,READ_ALARM,INVALID_ALARM) && errVerbose
		&& (pbi->stat!=READ_ALARM || pbi->sevr!=INVALID_ALARM))
			recGblRecordError(-1,(void *)pbi,"xy210_driver Error");
		return(2);
	}
	return(status);
}
