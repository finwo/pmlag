#include <linux/if_ether.h>
/* #include <stdio.h> */
/* #include <stdlib.h> */
#include <string.h>

#include "finwo/mindex.h"

#include "config.h"
#include "routing-table.h"

static int rt_compare(const void *a, const void *b, void *udata) {
  struct pmlag_rt_entry *ta = (struct pmlag_rt_entry *)a;
  struct pmlag_rt_entry *tb = (struct pmlag_rt_entry *)b;
  if (a == b) return 0;
  return memcmp(ta->mac, tb->mac, ETH_ALEN);
}

static void rt_purge(void *item, void *udata) {
  struct pmlag_rt_entry *rt_entry = (struct pmlag_rt_entry *)item;
  if (!rt_entry) return;
  if (rt_entry->iface) free(rt_entry->iface);
  if (rt_entry->mac) free(rt_entry->mac);
  free(rt_entry);
}

struct mindex_t * rt_init(void *udata) {
  return mindex_init(rt_compare, rt_purge, udata);
}

int rt_upsert(
  struct mindex_t    *rt,
  struct pmlag_iface *iface,
  unsigned char      *mac,
  int16_t             bcidx
) {
  struct pmlag_rt_entry *rt_entry;
  int i, isnew = 0;
  int iface_found = 0;

  // Attempt to fetch the rt entry
  rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac }));

  // None given, build new one
  if (!rt_entry) {
    rt_entry              = calloc(1, sizeof(struct pmlag_rt_entry));
    rt_entry->mac         = malloc(ETH_ALEN);
    rt_entry->bcidx       = bcidx;
    rt_entry->iface       = calloc(iface->bond->iface_count, sizeof(struct pmlag_iface *));
    rt_entry->iface_count = 0;
    memcpy(rt_entry->mac, mac, ETH_ALEN);
    isnew = 1;
  }

  // Clear interface list if bcidx is different
  if (rt_entry->bcidx != bcidx) {
    rt_entry->iface_count = 0;
    rt_entry->bcidx = bcidx;
  }

  // Register the interface on the rt_entry
  for( i = 0 ; i < rt_entry->iface_count ; i++ ) {
    if (rt_entry->iface[i] != iface) continue;
    iface_found = 1;
    break;
  }
  if (!iface_found) {
    rt_entry->iface[rt_entry->iface_count] = iface;
    rt_entry->iface_count++;
  }

  // The actual upsert part
  if (isnew) {
    mindex_set(rt, rt_entry);
    // Keep rt size in check
    if (rt->length > PMLAG_RT_MAX) {
      mindex_delete(rt, mindex_rand(rt));
    }
  }

  return 0;
}

struct pmlag_iface * rt_find(
  struct mindex_t *rt,
  pthread_mutex_t *mtx,
  unsigned char *mac
) {
  struct pmlag_rt_entry *rt_entry;

  // Attempt to fetch the rt entry
  rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac }));
  if (!rt_entry) return NULL;

  // Unlock the routing table again
  struct pmlag_iface *iface = rt_entry->interfaces[rand() % rt_entry->iface_cnt];
  return iface;
}
