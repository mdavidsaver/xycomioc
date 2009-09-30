/* devMbboXVme220.c */
/* base/src/dev devMbboXVme220.c,v 1.1 2003/08/27 15:21:40 mrk Exp */

/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/* devMbboXVme220.c - Device Support Routines	*/
/* XYcom 32 bit binary output			*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 *
 * Modification Log:
 * -----------------
 * .01  11-11-91        jba     Moved set of alarm stat and sevr to macros
 * .02	03-13-92	jba	ANSI C changes
 * .03	02-08-94	mrk	Issue Hardware Errors BUT prevent Error Message Storms
 *      ...
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
#include "mbboRecord.h"
#include <epicsExport.h>

#include "drvXy220.h"


/* Create the dset for devAiMbboXVme220 */
static long init_record();
static long write_mbbo();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_mbbo;
}devMbboXVme220={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	write_mbbo};
epicsExportAddress(dset,devMbboXVme220);

static long init_record(struct mbboRecord *pmbbo)
{
    epicsUInt32 value;
    struct vmeio *pvmeio;
    int		status=0;

    /* mbbo.out must be an VME_IO */
    switch (pmbbo->out.type) {
    case (VME_IO) :
	pvmeio = &(pmbbo->out.value.vmeio);
	pmbbo->shft = pvmeio->signal;
	pmbbo->mask <<= pmbbo->shft;
	status = xy220_read(pvmeio->card,pmbbo->mask,&value);
	if(status==0) pmbbo->rbv = pmbbo->rval = value;
	else status = 2;
	break;
    default :
	status = S_db_badField;
	recGblRecordError(status,(void *)pmbbo,
		"devMbboXVme220 (init_record) Illegal OUT field");
    }
    return(status);
}

static long write_mbbo(struct mbboRecord *pmbbo)
{
	struct vmeio *pvmeio;
	int	    status;
	epicsUInt32 value;

	
	pvmeio = &(pmbbo->out.value.vmeio);
	status = xy220_driver(pvmeio->card,&pmbbo->rval,pmbbo->mask);
	if(status==0) {
		status = xy220_read(pvmeio->card,pmbbo->mask,&value);
		if(status==0) pmbbo->rbv = value;
                else recGblSetSevr(pmbbo,READ_ALARM,INVALID_ALARM);
	} else {
                if(recGblSetSevr(pmbbo,WRITE_ALARM,INVALID_ALARM) && errVerbose
		&& (pmbbo->stat!=WRITE_ALARM || pmbbo->sevr!=INVALID_ALARM))
			recGblRecordError(-1,(void *)pmbbo,"xy220_driver Error");
	}
	return(0);
}
