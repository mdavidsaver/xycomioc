
#include "xy566.h"

#include <stdlib.h>
#include <string.h>

#include <epicsExport.h>
#include <iocsh.h>
#include <errlog.h>

/*
 * XYCOM 566 sampling sequence construction
 *
 * When assembling the sequence each channel
 * is assigned three properties.
 *
 * number of samples
 *
 * order - Channels are ordered from lowest to highest
 *         several channels can have the same order #.
 *         In this case they are interleaved (1,2,3,1,2,3,...)
 *
 * priority - Used to determine the order of interleaved
 *            channels.
 *
 * The combination of order and priority must be unique.
 *
 * Interleaved channels must have the same number of samples.
 */

/*
 * A list of seqent is stored in the seq_ctor member
 * of the xy566 structure.
 *
 * It is sorted in accending order by channel, order, priority
 */
typedef struct {
  ELLNODE node; /* must be first */

  size_t channel;
  size_t nsamples;

  size_t order;
  size_t priority;
} seqent;

#define node2seqent(n) ( (seqent*)(n) )

#define entFirst(list) node2seqent(ellFirst(list))
#define entNext(ent) node2seqent(ellNext(&(ent)->node))


/* Special bits in sequence ram */
#define SEQ_IRQ (1<<5)
#define SEQ_STP (1<<6)
#define SEQ_END (1<<7)

void
seq566set(int id, int ch, int nsamp, int ord, int prio)
{
  xy566 *card=get566(id);
  ELLNODE *node, *prev;
  seqent *ent;

  if(card->fail) return;

  if(!card){
    errlogPrintf("Invalid ID\n");
    card->fail=1;
    return;
  }

  if(ch<0 || ch>card->nchan){
    errlogPrintf("Invalid channel (0->%d)\n",card->nchan-1);
    card->fail=1;
    return;
  }

  if(nsamp<1 || nsamp>256){
    /* This is just a first pass to catch the
     * obviously impossible.
     * The actual max number of samples will depend
     * on what other channels are used
     */
    errlogPrintf("Invalid number of samples\n");
    card->fail=1;
    return;
  }

  if(ord<0 || ord >= card->nchan){
    errlogPrintf("Invalid order (1->%d)\n",card->nchan);
    card->fail=1;
    return;
  }

  if(prio<1 || prio >= card->nchan){
    errlogPrintf("Invalid priority (1->%d)\n",card->nchan);
    card->fail=1;
    return;
  }

  /* find location for insertion which maintains
   * sorted order
   */
  for(node=ellFirst(&card->seq_ctor), prev=NULL;
      node;
      prev=node, node=ellNext(node)
  ){
    ent=node2seqent(node);
    if(ent->channel==ch){
      errlogPrintf("Channel %d already used\n",ch);
      card->fail=1;
      return;
    }else if(ent->channel > ch)
      break;
    else if(ent->channel >= ch && ent->order > ord)
      break;
    else if(ent->channel >= ch && ent->order == ord && ent->priority > prio)
      break;
    else if(ent->channel >= ch && ent->order == ord && ent->priority == prio){
      errlogPrintf("Order %d and prio %d are already used by channel %d\n",
        ord,prio,ent->channel);
      card->fail=1;
      return;
    }
  }

  /* node and/or prev may be NULL if this
   * entry is to be placed at the start
   * of the list
   */

  ent=malloc(sizeof(seqent));
  if(!ent){
    errlogPrintf("Out of memory\n");
    card->fail=1;
    return;
  }

  ent->channel=ch;
  ent->nsamples=nsamp;
  ent->order=ord;
  ent->priority=prio;

  /* Inserts between prev and node */
  ellInsert(&card->seq_ctor, prev, &ent->node);
}

int finish566seq(xy566* card)
{
  seqent *bfirst, *blast;
  size_t cord, cprio, csize;
  size_t nchan; /* number of channels with order 'cord' */
  size_t seqpos=0; /* first unused position in the sequence */
  size_t i,j;

  memset(&card->seq,0,sizeof(card->seq));

  if(!ellFirst(&card->seq_ctor)){
    errlogPrintf("Card %d has no channel definitions\n",card->id);
    return 1;
  }

  if(dbg566>1){
    errlogPrintf("Sequence Source for card %d\n",card->id);
    for(bfirst=entFirst(&card->seq_ctor);
        bfirst;
        bfirst=entNext(bfirst)
    ){
      errlogPrintf("Ch %u (%u) %u:%u\n",
        bfirst->channel,bfirst->nsamples,
        bfirst->order,bfirst->priority
      );
    }
    bfirst=NULL;
  }

  /* The sequence is constructed block by block
   * where each block is all the channels with
   * the same order.
   */
  do{
    bfirst=entFirst(&card->seq_ctor);
    cord=bfirst->order;
    cprio=bfirst->priority;
    csize=bfirst->nsamples;

    /* find the entry after the last entry for this order */
    for(blast=entNext(bfirst), nchan=1;
        blast;
        blast=entNext(blast), nchan++
    ){
      if(blast->order > cord)
        break;

      /* sanity check sorting and number of samples */
      else if(blast->order < cord){
        errlogPrintf("Card %d list not sorted (order)!?!\n",card->id);
        return 1;
      }else if(blast->priority <= cprio){
        errlogPrintf("Card %d list not sorted (priority)!?!\n",card->id);
        return 1;
      }else if(blast->nsamples != csize){
        errlogPrintf("Card %d channel %d in order %d has as "
            "incorrect number of samples, %d not %d\n",
            card->id,blast->channel,blast->order,blast->nsamples,csize);
        return 1;
      }
    }

    /* this order # will require nchan*csize entries in the sequence list */

    if( seqpos + nchan*csize >= sizeof(card->seq) ){
      errlogPrintf("Card %d does not have enough sequence ram to "
        "sample all requested channels\n",card->id);
      return 1;
    }

    for(i=0; bfirst!=blast; bfirst=entNext(bfirst), i++)
    {
      for(j=0; j<csize; j++){
        card->seq[seqpos + i + j*nchan]=bfirst->channel;
      }
    }
    seqpos+=nchan*csize;

    /* blast is the frist entry for the next order */
    bfirst=blast;
  }while(bfirst);

  if(seqpos==0){
    errlogPrintf("Card %d defines no samples?!?\n",card->id);
    return 1;
  }

  card->seq[seqpos-1]|=SEQ_END|SEQ_IRQ;

  if(dbg566>0){
    errlogPrintf("Sequence for card %d\n",card->id);
    for(i=0;i<sizeof(card->seq);i++){
      if(i%16==0)
        errlogPrintf("%02x: ",i);
      errlogPrintf("%02x",card->seq[i]);
      if(i%16==15)
        errlogPrintf("\n");
      else if(i%4==3)
        errlogPrintf(" ");
    }
  }

  /* Now clean up things which don't need to persist when running */

  bfirst=entFirst(&card->seq_ctor);
  while(bfirst){
    blast=entNext(bfirst);

    ellDelete(&card->seq_ctor, &bfirst->node);
    free(bfirst);

    bfirst=blast;
  }

  return 0;
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
void seq566Register(void)
{
  iocshRegister(&seq566setFuncDef,seq566setCallFunc);
}
epicsExportRegistrar(seq566Register);

