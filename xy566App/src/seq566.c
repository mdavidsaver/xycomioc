
#include "xy566.h"

#include <stdlib.h>
#include <string.h>

#include <epicsExport.h>
#include <iocsh.h>
#include <errlog.h>

#include <regaccess.h>

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
  
  size_t stop;
} seqent;

#define node2seqent(n) ( (seqent*)(n) )

void
seq566set(int id, int ch, int nsamp, int ord, int prio, int stop)
{
  xy566 *card=get566(id);
  ELLNODE *node, *prev;
  seqent *ent;

  if(card->fail) return;

  if(!card){
    printf("Invalid ID\n");
    card->fail=1;
    return;
  }

  if(ch<0 || ch>card->nchan){
    printf("Invalid channel (0->%d)\n",card->nchan-1);
    card->fail=1;
    return;
  }

  if(nsamp<1 || nsamp>256){
    /* This is just a first pass to catch the
     * obviously impossible.
     * The actual max number of samples will depend
     * on what other channels are used
     */
    printf("Invalid number of samples\n");
    card->fail=1;
    return;
  }

  if(stop<0 || stop>nsamp){
    printf("Stop sample must be between 0 and %u\n",nsamp);
    card->fail=1;
    return;
  }

  if(ord<0 || ord >= card->nchan){
    printf("Invalid order (1->%d)\n",card->nchan);
    card->fail=1;
    return;
  }

  if(prio<1 || prio >= card->nchan){
    printf("Invalid priority (1->%d)\n",card->nchan);
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
      printf("Channel %d already used\n",ch);
      card->fail=1;
      return;
    }else if(ent->order == ord && ent->priority == prio){
      printf("Order %d and prio %d are already used by channel %u\n",
        ord,prio,(unsigned)ent->channel);
      card->fail=1;
      return;
    }else if(ent->order == ord && ent->nsamples != nsamp){
      printf("Order %d must have size %u\n",
        ord,(unsigned)ent->nsamples);
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
    printf("Out of memory\n");
    card->fail=1;
    return;
  }

  ent->channel=ch;
  ent->nsamples=nsamp;
  ent->order=ord;
  ent->priority=prio;
  ent->stop=stop;

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
    printf("Sequence Source for card %d\n",card->id);
    for(nfirst=ellFirst(&card->seq_ctor);
        nfirst;
        nfirst=ellNext(nfirst)
    ){
      bfirst=node2seqent(nfirst);
      printf("Ch %u (%u) %u:%u S%u\n",
        (unsigned)bfirst->channel,(unsigned)bfirst->nsamples,
        (unsigned)bfirst->order,(unsigned)bfirst->priority,
        (unsigned)bfirst->stop
      );
    }
    bfirst=NULL;
  }

  nfirst=ellFirst(&card->seq_ctor);
  if(!nfirst){
    printf("Card %d has no channel definitions\n",card->id);
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
        printf("Card %d list not sorted (order)!?!\n",card->id);
        return 1;
      }else if(blast->priority <= cprio){
        printf("Card %d list not sorted (priority)!?!\n",card->id);
        return 1;
      }else if(blast->nsamples != csize){
        printf("Card %d channel %d in order %d has as "
            "incorrect number of samples, %d not %d\n",
            card->id,(unsigned)blast->channel,(unsigned)blast->order,(unsigned)blast->nsamples,(unsigned)csize);
        return 1;
      }
    }

    /* at this point nlast can be NULL and blast can be invalid */

    /* this order # will require nchan*csize entries in the sequence list */

    if( seqpos + nchan*csize >= sizeof(card->seq) ){
      printf("Card %d does not have enough sequence ram to "
        "sample all requested channels\n",card->id);
      printf("cur %u nchan %u csize %u lim %u\n",
        (unsigned)seqpos,(unsigned)nchan,(unsigned)csize,(unsigned)sizeof(card->seq));
      return 1;
    }

    for(i=0; nfirst!=nlast; nfirst=ellNext(nfirst), i++)
    {
      bfirst=node2seqent(nfirst);
      for(j=0; j<csize; j++){
        card->seq[seqpos + i + j*nchan]=bfirst->channel;

        if(!!bfirst->stop && j==(bfirst->stop-1)){
          card->seq[seqpos + i + j*nchan]|=SEQ_STP;
        }
      }
    }
    seqpos+=nchan*csize;

    /* nlast is the frist entry for the next order */
    nfirst=nlast;
  }while(nfirst);

  if(seqpos==0){
    printf("Card %d defines no samples?!?\n",card->id);
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
      printf("Out of memory!!!\n");
      return 1;
    }
  }

  if(dbg566>0){
    printf("Sequence for card %d\n",card->id);
    for(i=0;i<sizeof(card->seq);i++){
      if(i%16==0)
        printf("%02x: ",(unsigned)i);
      printf("%02x",card->seq[i]);
      if(i%16==15)
        printf("\n");
      else if(i%4==3)
        printf(" ");
      if(card->seq[i]&SEQ_END){
        printf("\n");
        break;
      }
    }
  }
  
  /* Actually write to the card */
  
  for(i=0; i<sizeof(card->seq); i++){
    WRITE8(card->base, XY566_SEQR(i), card->seq[i]);
    if(card->seq[i]&SEQ_END)
      break;
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

