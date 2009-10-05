
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

#include <boRecord.h>

#include "xy220.h"

/* per device struct */
typedef struct {
  ELLNODE node; /* must be first */
  short id;
  epicsUInt32 base_addr;
  volatile epicsUInt8* base;
  epicsMutexId guard;
} xy220;

static ELLLIST xy220s={{NULL,NULL},0};

#define node2priv(n) ( (xy220*)(n) )

int dbg220=0;
epicsExportAddress(int,dbg220);

static
long init_rec(dbCommon* prec, DBLINK* lnk)
{
  xy220 *priv;
  ELLNODE *node;

  for(node=ellFirst(&xy220s); !!node; node=ellNext(node)){
    priv=node2priv(node);
    if(priv->id==lnk->value.vmeio.card)
      break;
    else
      priv=NULL;
  }

  if(!priv){
    errMessage(errlogFatal,"card# not associated with a device");
    return S_dev_noDevice;
  }

  prec->dpvt=priv;

  return 0;
}

/********** Inits *********/

static
long init_record_output(boRecord* prec)
{
  return init_rec((dbCommon*)prec,&prec->out);
}

/************ write ************/

static
long write_output(boRecord* prec)
{
  xy220 *priv=prec->dpvt;
  short line=prec->out.value.vmeio.signal;
  int port=line/8;
  int bit=line%8;
  epicsUInt8 mask=1<<bit;
  epicsUInt8 val;

  if(line<0 || port>XY220_NPORTS){
    errMessage(errlogMajor,"I/O bit number out of range");
    return 1;
  }

  epicsMutexMustLock(priv->guard);

  val=*(priv->base+XY220_PORT(port));

  if(dbg220>0)
    errlogPrintf("%lx read %02x ",(unsigned long)prec,val);

  if(prec->rval)
    val|=mask;
  else
    val&=~mask;

  if(dbg220>0)
    errlogPrintf("write %02x\n",val);

  *(priv->base+XY220_PORT(port))=val;

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

static io_dset devBo220Output =
{
  5,
  NULL,
  NULL,
  (DEVSUPFUN)init_record_output,
  NULL,
  (DEVSUPFUN)write_output
};
epicsExportAddress(dset,devBo220Output);

void
xycom220setup(int id,int base)
{
  xy220 *card=malloc(sizeof(xy220));
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

  if(devReadProbe(1, card->base+XY220_ID, &junk)){
    errlogPrintf("Failed to read %lx for card %x\n",(unsigned long)(card->base+XY220_ID),id);
    free(card);
    return;
  }

  *(card->base+XY220_CSR)=X220_CSR_RED|X220_CSR_GRN;

  card->guard=epicsMutexMustCreate();

  if(dbg220>0)
    errlogPrintf("%d mapped %lx as %lx\n",id,(unsigned long)card->base,(unsigned long)card->base);

  ellAdd(&xy220s,&card->node);
  return;
}

/* xycom220setup */
static const iocshArg xycom220setupArg0 = { "Card id number",iocshArgInt};
static const iocshArg xycom220setupArg1 = { "VME base address of card",iocshArgInt};
static const iocshArg * const xycom220setupArgs[2] =
{
    &xycom220setupArg0,&xycom220setupArg1
};
static const iocshFuncDef xycom220setupFuncDef =
    {"xycom220setup",2,xycom220setupArgs};
static void xycom220setupCallFunc(const iocshArgBuf *args)
{
    xycom220setup(args[0].ival,args[1].ival);
}

static
void dev220reg(void)
{
  iocshRegister(&xycom220setupFuncDef,xycom220setupCallFunc);
}
epicsExportRegistrar(dev220reg);
