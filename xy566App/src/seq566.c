
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
 *
 * Each channel can have only one entry
 */

/*
 * A list of seqent is stored in the seq_ctor member
 * of the xy566 structure.
 *
 * It is sorted in accending order by order, priority
 */
typedef struct {
  ELLNODE node; /* must be first */

  size_t channel;
  size_t nsamples;

  size_t order;
  size_t priority;
} seqent;

#define node2seqent(n) ( (seqent*)(n) )

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

  /* Search to see that the rules are followed */
  for(node=ellFirst(&card->seq_ctor), prev=NULL;
      node;
      prev=node, node=ellNext(node)
  ){
    ent=node2seqent(node);
    if(ent->channel==ch){
      errlogPrintf("Channel %d already used\n",ch);
      card->fail=1;
      return;
    }else if(ent->order == ord && ent->priority == prio){
      errlogPrintf("Order %d and prio %d are already used by channel %d\n",
        ord,prio,ent->channel);
      card->fail=1;
      return;
    }else if(ent->order == ord && ent->nsamples != nsamp){
      errlogPrintf("Order %d must have size %u\n",
        ord,ent->nsamples);
      card->fail=1;
      return;
    }
  }

  /* find location for insertion which maintains
   * sorted order
   */
  for(node=ellFirst(&card->seq_ctor), prev=NULL;
      node;
      prev=node, node=ellNext(node)
  ){
    ent=node2seqent(node);
    if(ent->order > ord)
      break;
    else if(ent->order == ord && ent->priority > prio)
      break;
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
  ELLNODE *nfirst, *nlast;
  seqent *bfirst, *blast;
  size_t cord, cprio, csize;
  size_t nchan; /* number of channels with order 'cord' */
  size_t seqpos=0; /* first unused position in the sequence */
  size_t i,j;

  memset(&card->seq,0,sizeof(card->seq));

  if(dbg566>1){
    errlogPrintf("Sequence Source for card %d\n",card->id);
    for(nfirst=ellFirst(&card->seq_ctor);
        nfirst;
        nfirst=ellNext(nfirst)
    ){
      bfirst=node2seqent(nfirst);
      errlogPrintf("Ch %u (%u) %u:%u\n",
        bfirst->channel,bfirst->nsamples,
        bfirst->order,bfirst->priority
      );
    }
    bfirst=NULL;
  }

  nfirst=ellFirst(&card->seq_ctor);
  if(!nfirst){
    errlogPrintf("Card %d has no channel definitions\n",card->id);
    return 1;
  }

  /* The sequence is constructed block by block
   * where each block is all the channels with
   * the same order.
   */
  do{
    bfirst=node2seqent(nfirst);
    cord=bfirst->order;
    cprio=bfirst->priority;
    csize=bfirst->nsamples;

    /* find the entry after the last entry for this order */
    for(nlast=ellNext(nfirst), nchan=1;
        nlast;
        nlast=ellNext(nlast), nchan++
    ){
      blast=node2seqent(nlast);

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

    /* at this point nlast can be NULL and blast can be invalid */

    /* this order # will require nchan*csize entries in the sequence list */

    if( seqpos + nchan*csize >= sizeof(card->seq) ){
      errlogPrintf("Card %d does not have enough sequence ram to "
        "sample all requested channels\n",card->id);
      errlogPrintf("cur %u nchan %u csize %u lim %u\n",
        seqpos,nchan,csize,sizeof(card->seq));
      return 1;
    }

    for(i=0; nfirst!=nlast; nfirst=ellNext(nfirst), i++)
    {
      bfirst=node2seqent(nfirst);
      for(j=0; j<csize; j++){
        card->seq[seqpos + i + j*nchan]=bfirst->channel;
      }
    }
    seqpos+=nchan*csize;

    /* nlast is the frist entry for the next order */
    nfirst=nlast;
  }while(nfirst);

  if(seqpos==0){
    errlogPrintf("Card %d defines no samples?!?\n",card->id);
    return 1;
  }

  card->seq[seqpos-1]|=SEQ_END|SEQ_IRQ;

  /* allocate final data buffers */
  for(nfirst=ellFirst(&card->seq_ctor), i=0;
      nfirst;
      nfirst=ellNext(nfirst), i++
  ){
    bfirst=node2seqent(nfirst);
    card->dlen[i]=bfirst->nsamples;
    card->data[i]=calloc(bfirst->nsamples,sizeof(epicsUInt16));
    if(!card->data[i]){
      errlogPrintf("Out of memory!!!\n");
      return 1;
    }
  }

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
      if(card->seq[i]&SEQ_END){
        errlogPrintf("\n");
        break;
      }
    }
  }

  /* Now clean up things which don't need to persist when running */

  nfirst=ellFirst(&card->seq_ctor);
  while(nfirst){
    bfirst=node2seqent(nfirst);
    nlast=ellNext(nfirst);

    ellDelete(&card->seq_ctor, &bfirst->node);
    free(bfirst);

    nfirst=nlast;
  }

  return 0;
}

