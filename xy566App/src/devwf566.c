
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

#include <waveformRecord.h>

#include "xy566.h"

#ifndef MIN
# define MIN(A,B) ((A)<(B) ? (A) : (B))
#endif

static
long init_record(waveformRecord* prec)
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
  
  if(prec->ftvl!=DBF_DOUBLE){
    err=S_db_badDbrtype;
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
long read_chan(waveformRecord* prec)
{
  xy566 *card=prec->dpvt;
  short chan=prec->inp.value.vmeio.signal;
  double *fptr=prec->bptr;
  size_t i;

  if(chan < 0 || chan > card->nchan)
    return -1;

  epicsMutexMustLock(card->guard);

  if(card->dlen[chan]<1){
    epicsMutexUnlock(card->guard);
    return -1;
  }

  prec->nord=MIN(prec->nelm, card->dlen[chan]);

  for(i=0; i<prec->nord; i++)
    fptr[i]=card->data[chan][i];

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
} devWf566 =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record,
  (DEVSUPFUN)get_ioint_info,
  (DEVSUPFUN)read_chan
};
epicsExportAddress(dset,devWf566);

