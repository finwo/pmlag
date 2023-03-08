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

  printf("\nCMP\n  A = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n  B = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
    ta->mac[0],ta->mac[1],ta->mac[2],ta->mac[3],ta->mac[4],ta->mac[5],
    tb->mac[0],tb->mac[1],tb->mac[2],tb->mac[3],tb->mac[4],tb->mac[5]
  );

  return memcmp(ta->mac, tb->mac, ETH_ALEN);
}

static void rt_purge(const void *item, void *udata) {
  // Empty for now
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
  /* int isnew  = 0; */
  struct pmlag_rt_entry *rt_entry;

  printf("\nUpserting RT, %d\n", bcidx);

  // Lock the routing table
  pthread_mutex_lock(mtx);

  // Attempt to fetch the rt entry
  rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac }));

  /* int16_t obcidx = 0; */
  if (rt_entry) {
  /*   obcidx = rt_entry->bcidx; */
    printf("  Found, %d\n", rt_entry->bcidx);
  }

  // None given, build new one
  if (!rt_entry) {
    rt_entry             = calloc(1, sizeof(struct pmlag_rt_entry));
    rt_entry->mac        = malloc(ETH_ALEN);
    /* rt_entry->bcidx      = 0; */
    rt_entry->interfaces = NULL;
    memcpy(rt_entry->mac, mac, ETH_ALEN);
    /* isnew = 1; */
  }

  /* printf("  Old bcidx, %d\n", rt_entry->bcidx); */
  /* if ((!isnew) && (obcidx != rt_entry->bcidx)) { */
  /*   printf("  BORKED\n"); */
  /*   exit(1); */
  /* } */

  /* // Bail if */
  /* if ( */
  /*     (!bcidx && rt_entry->bcidx) ||  // We receive a regular packet on bcidx-tracked entry */
  /*     ((bcidx - rt_entry->bcidx) < 0) // Or the received bcidx is lower than known (old packet) */
  /* ) { */
  /*   printf("  Bail, %d, %d\n\n", bcidx, rt_entry->bcidx); */
  /*   pthread_mutex_unlock(mtx); */
  /*   return 0; */
  /* } */

  // Clear list of known interfaces if
  pmlag_iface_llist *iface_entry;
  /* if ( */
  /*   (bcidx && (rt_entry->bcidx != bcidx)) || // We got a NEW broadcast index */
  /*   (!bcidx && (rt_entry->bcidx == 0))       // Or we're updating a non-pmlag remote */
  /* ) { */
    // Free list 1-by-1
    while(rt_entry->interfaces) {
      iface_entry          = rt_entry->interfaces;
      rt_entry->interfaces = iface_entry->next;
      free(iface_entry);
    }
  /* } */

  /* // Update the rt_entry's broadcast index */
  /* rt_entry->bcidx = bcidx; */

  /* printf("  How 'bout now, %d\n", rt_entry->bcidx); */

  // Add our iface to the entry's interface list
  iface_entry          = malloc(sizeof(pmlag_iface_llist));
  iface_entry->next    = rt_entry->interfaces;
  iface_entry->data    = iface;
  rt_entry->interfaces = iface_entry;

  // Ensure the entry is in the rt
  mindex_set(rt, rt_entry);

  /* printf("  or now, %d\n", rt_entry->bcidx); */
  printf("  RT is now %ld\n\n", mindex_length(rt));

  pthread_mutex_unlock(mtx);
  return 0;
}
