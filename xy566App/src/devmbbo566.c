
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

#include <mbboRecord.h>

#include "xy566.h"

static
long init_record(mbboRecord* prec)
{
  xy566 *card=get566(prec->out.value.vmeio.card);

  if(!card){
    errMessage(errlogFatal,"card# not associated with a device");
    return S_dev_noDevice;
  }

  return 0;
}

static
long write_gain(mbboRecord* prec)
{
  xy566 *card=prec->dpvt;
  short chan=prec->out.value.vmeio.signal;

  if(chan < 0 || chan > card->nchan)
    return 1;

  WRITE16(card->base+XY566_GAIN(chan), prec->rval);

  return 0;
}

typedef struct {
  long    number;
  DEVSUPFUN report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN read_write;
} io_dset;

static io_dset devMbbo566Gain =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record,
  NULL,
  (DEVSUPFUN)write_gain
};
epicsExportAddress(dset,devMbbo566Gain);

