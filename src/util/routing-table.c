#include <linux/if_ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "finwo/mindex.h"
#include "routing-table.h"

static int rt_compare(const void *a, const void *b, void *udata) {
  struct pmlag_rt_entry *ta = (struct pmlag_rt_entry *)a;
  struct pmlag_rt_entry *tb = (struct pmlag_rt_entry *)b;

  // Attempt without memcmp (faster)
  if (a == b) return 0;

  /* printf("\nCMP\n  A = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n  B = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", */
  /*   ta->mac[0],ta->mac[1],ta->mac[2],ta->mac[3],ta->mac[4],ta->mac[5], */
  /*   tb->mac[0],tb->mac[1],tb->mac[2],tb->mac[3],tb->mac[4],tb->mac[5] */
  /* ); */

  return memcmp(ta->mac, tb->mac, ETH_ALEN);
}

static void rt_purge(const void *item, void *udata) {
  struct pmlag_rt_entry *rt_entry = (struct pmlag_rt_entry *)item;
  pmlag_iface_llist *iface_entry;

/* #ifdef DEBUG */
/*   printf("PURGE %.2x:%.2x:%.2x:%.2x:%.2x:%.2x -- %p\n", */
/*     rt_entry->mac[0], */
/*     rt_entry->mac[1], */
/*     rt_entry->mac[2], */
/*     rt_entry->mac[3], */
/*     rt_entry->mac[4], */
/*     rt_entry->mac[5], */
/*     item */
/*   ); */
/* #endif */

  // Free all iface entries in the rt_entry
  while(rt_entry->interfaces) {
    iface_entry          = rt_entry->interfaces;
    rt_entry->interfaces = iface_entry->next;
    free(iface_entry);
  }

  // Free remainder
  free(rt_entry->mac);
  free(rt_entry);
}

struct mindex_t * rt_init(void *udata) {
  return mindex_init(rt_compare, rt_purge, udata);
}

int rt_upsert(
  struct mindex_t *rt,
  pthread_mutex_t *mtx,
  struct pmlag_iface *iface,
  unsigned char *mac,
  int16_t bcidx
) {
  int isnew  = 0;
  struct pmlag_rt_entry *rt_entry;

/* #ifdef DEBUG */
/*   printf("rt_upsert\n"); */
/* #endif */

  // Lock the routing table
  pthread_mutex_lock(mtx);
/* #ifdef DEBUG */
/*   printf("  acquired lock\n"); */
/* #endif */

  // Attempt to fetch the rt entry
  rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac }));

/* #ifdef DEBUG */
/*   printf("  Found %p -- %d\n", rt_entry, rt_entry ? rt_entry->bcidx : 0); */
/* #endif */

  // None given, build new one
  if (!rt_entry) {
    rt_entry             = calloc(1, sizeof(struct pmlag_rt_entry));
    rt_entry->mac        = malloc(ETH_ALEN);
    rt_entry->bcidx      = 0;
    rt_entry->interfaces = NULL;
    memcpy(rt_entry->mac, mac, ETH_ALEN);
    isnew = 1;
  }

  // Bail if
  if (
      (!bcidx && rt_entry->bcidx) ||  // We receive a regular packet on bcidx-tracked entry
      (rt_entry->bcidx && ((bcidx - rt_entry->bcidx) < 0)) // Or the received bcidx is lower than known (old packet)
  ) {
/* #ifdef DEBUG */
/*     printf("  Bail, %d, %d\n\n", bcidx, rt_entry->bcidx); */
/* #endif */
    pthread_mutex_unlock(mtx);
    return 0;
  }

  // Clear list of known interfaces if
  pmlag_iface_llist *iface_entry;
  if (
    (bcidx && (rt_entry->bcidx != bcidx)) || // We got a NEW broadcast index
    (!bcidx && (rt_entry->bcidx == 0))       // Or we're updating a non-pmlag remote
  ) {
    // Free list 1-by-1
    while(rt_entry->interfaces) {
      iface_entry          = rt_entry->interfaces;
      rt_entry->interfaces = iface_entry->next;
      free(iface_entry);
    }
  }

  // Update the rt_entry's broadcast index
  rt_entry->bcidx = bcidx;

  // Add our iface to the entry's interface list
  iface_entry          = malloc(sizeof(pmlag_iface_llist));
  iface_entry->next    = rt_entry->interfaces;
  iface_entry->data    = iface;
  iface_entry->l       = rt_entry->interfaces ? ((pmlag_iface_llist *)iface_entry->next)->l + 1 : 1;
  rt_entry->interfaces = iface_entry;

  // Ensure the entry is in the rt
  if (isnew) {
    mindex_set(rt, rt_entry);
  }

/* #ifdef DEBUG */
/*   printf("  Iface count: %d\n", rt_entry->interfaces->l); */
/*   printf("  Routing table is now %ld entries\n", mindex_length(rt)); */
/* #endif */
  if (mindex_length(rt) > RT_MAX_ENTRIES) {
    rt_entry = mindex_rand(rt);
    if (rt_entry) {
      mindex_delete(rt, rt_entry);
    }
  }

  pthread_mutex_unlock(mtx);
  return 0;
}

struct pmlag_iface * rt_find(
  struct mindex_t *rt,
  pthread_mutex_t *mtx,
  unsigned char *mac
) {
  // Lock the routing table
  pthread_mutex_lock(mtx);
  struct pmlag_rt_entry *rt_entry;
  int llist_len = 0;

  // Attempt to fetch the rt entry
  rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac }));
  if (!rt_entry) {
    pthread_mutex_unlock(mtx);
    return NULL;
  }

  // Get the list length
  pmlag_iface_llist *iface_entry = rt_entry->interfaces;
  while(iface_entry) {
    llist_len++;
    iface_entry = iface_entry->next;
  }

  // Select an interface at random
  int sel = rand() % llist_len;
  iface_entry = rt_entry->interfaces;
  while(sel--) {
    iface_entry = iface_entry->next;
  }

  // Unlock the routing table again
  pthread_mutex_unlock(mtx);
  return iface_entry->data;
}
