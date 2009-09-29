/* devMbboDirectXVme220.c */
/* base/src/dev devMbboDirectXVme220.c,v 1.1 2003/08/27 15:21:39 mrk Exp */

/***********************************************************************
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory, and the Regents of the University of
* California, as Operator of Los Alamos National Laboratory.
* xycom is distributed subject to a Software License Agreement
* found in file LICENSE that is included with this distribution.
***********************************************************************/
/* devMbboDirectXVme220.c - Device Support Routines	*/
/* XYcom 32 bit binary output			*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Matthew Needes
 *      Date:            10-08-93
 *
 * Modification Log:
 * -----------------
 *   (modification log for devMbboXVme220 applies)
 *  .01  10-08-93  mcn     (created)  device support for MbboDirect records
 */


#include <vxWorks.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "module_types.h"
#include "mbboDirectRecord.h"
#include <epicsExport.h>

#include "drvXy220.h"


/* Create the dset for devMbboXVme220 */
static long init_record();
static long write_mbbo();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_mbbo;
}devMbboDirectXVme220={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	write_mbbo};
epicsExportAddress(dset,devMbboDirectXVme220);

static long init_record(struct mbboDirectRecord *pmbbo)
{
    unsigned long value;
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
		"devMbboDirectXVme220 (init_record) Illegal OUT field");
    }
    return(status);
}

static long write_mbbo(struct mbboDirectRecord *pmbbo)
{
	struct vmeio *pvmeio;
	int	    status;
	unsigned long value;

	
	pvmeio = &(pmbbo->out.value.vmeio);
	status = xy220_driver(pvmeio->card,&pmbbo->rval,pmbbo->mask);
	if(status==0) {
		status = xy220_read(pvmeio->card,pmbbo->mask,&value);
		if(status==0) pmbbo->rbv = value;
                else recGblSetSevr(pmbbo,READ_ALARM,INVALID_ALARM);
	} else {
                recGblSetSevr(pmbbo,WRITE_ALARM,INVALID_ALARM);
	}
	return(status);
}
