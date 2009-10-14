
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

#include <aiRecord.h>

#include "xy566.h"

static
long init_record(aiRecord* prec)
{
  long err;
  xy566 *card=get566(prec->inp.value.vmeio.card);

  if(!card){
    errMessage(errlogFatal,"card# not associated with a device");
    err=S_dev_noDevice;
    goto fail;
  }
  if(!!card->fail){
    err=1;
    goto fail;
  }

  prec->dpvt=card;

  return 0;

fail:
  prec->pact=TRUE;
  return err;
}

static
long get_ioint_info(int dir,dbCommon* prec,IOSCANPVT* io)
{
  xy566 *card=prec->dpvt;

  *io=card->seq_irq;

  return 0;
}

static
long read_chan(aiRecord* prec)
{
  xy566 *card=prec->dpvt;
  short chan=prec->inp.value.vmeio.signal;

  if(chan < 0 || chan > card->nchan){
    return 1;
  }

  epicsMutexMustLock(card->guard);

  if(card->dlen[chan]<1){
    epicsMutexUnlock(card->guard);
    return 1;
  }

  prec->rval=card->data[chan][0];

  epicsMutexUnlock(card->guard);

  return 0;
}

struct {
  long    number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read;
  DEVSUPFUN special_linconv;
} devAi566 =
{
  6,
  NULL,
  NULL,
  (DEVSUPFUN)init_record,
  (DEVSUPFUN)get_ioint_info,
  (DEVSUPFUN)read_chan,
  NULL
};
epicsExportAddress(dset,devAi566);

