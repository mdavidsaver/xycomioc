
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

#include "xy240.h"

/* per device struct */
typedef struct {
  ELLNODE node; /* must be first */
  short id;
  epicsUInt32 base_addr;
  volatile epicsUInt8* base;
  epicsMutexId guard;
} xy240;

static ELLLIST xy240s={{NULL,NULL},0};

#define node2priv(n) ( (xy240*)(n) )

static
long init_rec(dbCommon* prec, DBLINK* lnk)
{
  xy240 *priv;
  ELLNODE *node;

  for(node=ellFirst(&xy240s); !!node; node=ellNext(node)){
    priv=node2priv(node);
    if(priv->id==lnk->value.vmeio.card)
      break;
    else
      priv=NULL;
  }

  if(!priv){
    errMessage(errlogFatal,"card# not asociated with a device");
    return S_dev_noDevice;
  }

  prec->dpvt=priv;

  return 0;
}

/********** Inits *********/

static
long init_record_input(biRecord* prec)
{
  return init_rec((dbCommon*)prec,&prec->inp);
}

static
long init_record_output(boRecord* prec)
{
  return init_rec((dbCommon*)prec,&prec->out);
}

static
long init_record_portdir(boRecord* prec)
{
  return init_rec((dbCommon*)prec,&prec->out);
}

/************ read/write ***************/

static
long read_input(biRecord* prec)
{
  xy240 *priv=prec->dpvt;
  short line=prec->inp.value.vmeio.signal;
  int port=line/8;
  int bit=line%8;
  epicsUInt8 mask=1<<bit;
  epicsUInt8 val;

  if(line<0 || port>XY240_NPORTS){
    errMessage(errlogMajor,"I/O bit number out of range");
    return 1;
  }

  epicsMutexMustLock(priv->guard);

  val=*(priv->base+XY240_PORT(port));

  epicsMutexUnlock(priv->guard);

  val&=mask;

  prec->rval=val;

  return 0;
}

static
long write_output(boRecord* prec)
{
  xy240 *priv=prec->dpvt;
  short line=prec->out.value.vmeio.signal;
  int port=line/8;
  int bit=line%8;
  epicsUInt8 mask=1<<bit;
  epicsUInt8 val;

  if(line<0 || port>XY240_NPORTS){
    errMessage(errlogMajor,"I/O bit number out of range");
    return 1;
  }

  epicsMutexMustLock(priv->guard);

  val=*(priv->base+XY240_PORT(port));

  if(prec->rval)
    val|=mask;
  else
    val&=~mask;

  *(priv->base+XY240_PORT(port))=val;

  epicsMutexUnlock(priv->guard);

  return 0;
}

static
long write_portdir(boRecord* prec)
{
  xy240 *priv=prec->dpvt;
  short line=prec->out.value.vmeio.signal;
  int port=line/8;
  epicsUInt8 mask=1<<port;
  epicsUInt8 val;

  if(line<0 || port>XY240_NPORTS){
    errMessage(errlogMajor,"I/O bit number out of range");
    return 1;
  }

  epicsMutexMustLock(priv->guard);

  val=*(priv->base+XY240_DIR);

  if(prec->rval)
    val|=mask;
  else
    val&=~mask;

  *(priv->base+XY240_DIR)=val;

  epicsMutexUnlock(priv->guard);

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

static io_dset devBi240Input =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record_input,
  NULL,
  (DEVSUPFUN)read_input
};
epicsExportAddress(dset,devBi240Input);

static io_dset devBo240Output =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record_output,
  NULL,
  (DEVSUPFUN)write_output
};
epicsExportAddress(dset,devBo240Output);

static io_dset devBo240PortDir =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record_portdir,
  NULL,
  (DEVSUPFUN)write_portdir
};
epicsExportAddress(dset,devBo240PortDir);

void
xycom240setup(int id,int base)
{
  xy240 *card=malloc(sizeof(xy240));
  epicsUInt8 junk;
  volatile epicsUInt8 **ba;

  if(!card){
    errlogPrintf("Allocation failed\n");
    return;
  }

  card->id=id;
  card->base_addr=base;
  ba=&card->base;

  if(devBusToLocalAddr(atVMEA16, card->base_addr, (volatile void **)ba)){
    errlogPrintf("Failed to map %lx for card %x\n",(unsigned long)card->base,id);
    free(card);
    return;
  }

  if(devReadProbe(1, card->base+XY240_ID, &junk)){
    errlogPrintf("Failed to read %lx for card %x\n",(unsigned long)(card->base+XY240_ID),id);
    free(card);
    return;
  }

  *(card->base+XY240_CSR)=3;

  ellAdd(&xy240s,&card->node);
  return;
}

/* xycom240setup */
static const iocshArg xycom240setupArg0 = { "Card id number",iocshArgInt};
static const iocshArg xycom240setupArg1 = { "VME base address of card",iocshArgInt};
static const iocshArg * const xycom240setupArgs[2] =
{
    &xycom240setupArg0,&xycom240setupArg1
};
static const iocshFuncDef xycom240setupFuncDef =
    {"xycom240setup",2,xycom240setupArgs};
static void xycom240setupCallFunc(const iocshArgBuf *args)
{
    xycom240setup(args[0].ival,args[1].ival);
}

static
void dev240reg(void)
{
  iocshRegister(&xycom240setupFuncDef,xycom240setupCallFunc);
}
epicsExportRegistrar(dev240reg);
