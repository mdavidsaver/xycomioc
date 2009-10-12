
#include "xy566.h"

#include <iocsh.h>
#include <epicsExport.h>

/* xycom566setup */
static const iocshArg xycom566setupArg0 = { "Card id #",iocshArgInt};
static const iocshArg xycom566setupArg1 = { "Config (A16) base",iocshArgInt};
static const iocshArg xycom566setupArg2 = { "Data (A24) base",iocshArgInt};
static const iocshArg xycom566setupArg3 = { "IRQ Level",iocshArgInt};
static const iocshArg xycom566setupArg4 = { "IRQ Vector",iocshArgInt};
static const iocshArg xycom566setupArg5 = { "0 - 16 chan, 1 - 32 chan",iocshArgInt};
static const iocshArg * const xycom566setupArgs[6] =
{
    &xycom566setupArg0,&xycom566setupArg1,&xycom566setupArg2,
    &xycom566setupArg3,&xycom566setupArg4,&xycom566setupArg5
};
static const iocshFuncDef xycom566setupFuncDef =
    {"xycom566setup",6,xycom566setupArgs};
static void xycom566setupCallFunc(const iocshArgBuf *args)
{
    xycom566setup(args[0].ival,args[1].ival,args[2].ival,
                  args[3].ival,args[4].ival,args[5].ival);
}

/* xycom566finish */
static const iocshArg * const xycom566finishArgs[0] =
{
};
static const iocshFuncDef xycom566finishFuncDef =
    {"xycom566finish",0,xycom566finishArgs};
static void xycom566finishCallFunc(const iocshArgBuf *args)
{
    xycom566finish();
}

/* stc566simple */
static const iocshArg stc566simpleArg0 = { "Card id #",iocshArgInt};
static const iocshArg stc566simpleArg1 = { "STC clock divider (1->16)",iocshArgInt};
static const iocshArg stc566simpleArg2 = { "Sample period in multiples of 0.25us * div (1->0xffff)",iocshArgInt};
static const iocshArg * const stc566simpleArgs[3] =
{
    &stc566simpleArg0,&stc566simpleArg1,&stc566simpleArg2
};
static const iocshFuncDef stc566simpleFuncDef =
    {"stc566simple",3,stc566simpleArgs};
static void stc566simpleCallFunc(const iocshArgBuf *args)
{
    stc566simple(args[0].ival,args[1].ival,args[2].ival);
}

/* seq566set */
static const iocshArg seq566setArg0 = { "Card id #",iocshArgInt};
static const iocshArg seq566setArg1 = { "Channel number (0->16 or 32)",iocshArgInt};
static const iocshArg seq566setArg2 = { "Number of samples (>0)",iocshArgInt};
static const iocshArg seq566setArg3 = { "Order #",iocshArgInt};
static const iocshArg seq566setArg4 = { "Priority",iocshArgInt};
static const iocshArg * const seq566setArgs[5] =
{
    &seq566setArg0,&seq566setArg1,&seq566setArg2,&seq566setArg3,&seq566setArg4
};
static const iocshFuncDef seq566setFuncDef =
    {"seq566set",5,seq566setArgs};
static void seq566setCallFunc(const iocshArgBuf *args)
{
    seq566set(args[0].ival,args[1].ival,args[2].ival,
              args[3].ival,args[4].ival);
}

static
void register566(void)
{
  iocshRegister(&xycom566setupFuncDef,xycom566setupCallFunc);
  iocshRegister(&xycom566finishFuncDef,xycom566finishCallFunc);
  iocshRegister(&stc566simpleFuncDef,stc566simpleCallFunc);
  iocshRegister(&seq566setFuncDef,seq566setCallFunc);
}
epicsExportRegistrar(register566);
