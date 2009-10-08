
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <devSup.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <epicsMutex.h>
#include <devLib.h>
#include <iocsh.h>
#include <errlog.h>
#include <ellLib.h>

#include <dbAccess.h>
#include <epicsExport.h>

#include <biRecord.h>
#include <boRecord.h>

#include "xy566.h"

static
long init_rec(dbCommon* prec, DBLINK* lnk)
{
  xy566 *card=get566(lnk->value.vmeio.card);

  if(!card){
    errMessage(errlogFatal,"card# not associated with a device");
    return S_dev_noDevice;
  }

  return 0;
}

/********** Inits *********/

static
long init_record_status(biRecord* prec)
{
  return init_rec((dbCommon*)prec,&prec->inp);
}

static
long init_record_control(boRecord* prec)
{
  return init_rec((dbCommon*)prec,&prec->out);
}

/************ read/write ***************/

static
long read_status(biRecord* prec)
{
  xy566 *card=prec->dpvt;
  short func=prec->inp.value.vmeio.signal;
  epicsUInt16 csr;

  if(func==0){

     /* read running status */

     epicsMutexMustLock(card->guard);

     csr=READ16(card->base+XY566_CSR);

     prec->rval= csr&XY566_CSR_SRT;

     epicsMutexUnlock(card->guard);

     return 0;
  }else{
    errMessage(errlogFatal,"Invalid function given to record");
    return 1;
  }
}

static
long write_control(boRecord* prec)
{
  xy566 *card=prec->dpvt;
  short func=prec->out.value.vmeio.signal;

  if(func==0){

    /* Start/Stop aquisition */

     epicsMutexMustLock(card->guard);

     WRITE16(card->base+XY566_SWS, 1);

     epicsMutexUnlock(card->guard);

     return 0;
  }else{
    errMessage(errlogFatal,"Invalid function given to record");
    return 1;
  }
}


typedef struct {
  long    number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_write;
} io_dset;

static io_dset devBi566Status =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record_status,
  NULL,
  (DEVSUPFUN)read_status
};
epicsExportAddress(dset,devBi566Status);

static io_dset devBo566Control =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record_control,
  NULL,
  (DEVSUPFUN)write_control
};
epicsExportAddress(dset,devBo566Control);

