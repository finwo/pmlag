#include <linux/if_ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "tidwall/btree.h"
#include "routing-table.h"

static int compare_rt_entries(const void *a, const void *b, void *udata) {
  struct pmlag_rt_entry *ta = (struct pmlag_rt_entry*)a;
  struct pmlag_rt_entry *tb = (struct pmlag_rt_entry*)b;
  return memcmp(ta->mac, tb->mac, ETH_ALEN);
}

struct btree * rt_init(void *udata) {
  return btree_new(sizeof(void*), 0, compare_rt_entries, udata);
}

int rt_upsert(
  struct btree *rt,
  pthread_mutex_t *mtx,
  struct pmlag_iface *iface,
  unsigned char *mac,
  int16_t bcidx
) {
  int isnew  = 0;
  struct pmlag_rt_entry *rt_entry;

  printf("\nUpserting RT\n");

  // Lock the routing table
  pthread_mutex_lock(mtx);

  // Attempt to fetch the rt entry
  rt_entry = btree_get(rt, &(struct pmlag_rt_entry){ .mac = mac });

  // None given, build new one
  if (!rt_entry) {
    rt_entry = calloc(1, sizeof(struct pmlag_rt_entry));
    rt_entry->mac = malloc(ETH_ALEN);
    memcpy(rt_entry->mac, mac, ETH_ALEN);
    isnew = 1;
  }

  // Bail if
  if (
      (!bcidx && rt_entry->bcidx) ||  // We receive a regular packet on bcidx-tracked entry
      ((bcidx - rt_entry->bcidx) < 0) // Or the received bcidx is lower than known (old packet)
  ) {
    printf("  Bail\n\n");
    pthread_mutex_unlock(&(iface->bond->mtx_rt));
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
  rt_entry->interfaces = iface_entry;

  // Save rt entry in the routing table if it's new
  // A pre-existing one is already stored (btree holds pointers, no need to re-save)
  if (isnew) {
    printf("  New MAC\n");
    btree_set(rt, rt_entry);
  }

  printf("  RT is now %ld\n\n", btree_count(rt));

  pthread_mutex_unlock(mtx);
  return 0;
}
