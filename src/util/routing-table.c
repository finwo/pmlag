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

/* int rt_upsert( */
/*   struct mindex_t *rt, */
/*   pthread_mutex_t *mtx, */
/*   struct pmlag_iface *iface, */
/*   unsigned char *mac, */
/*   int16_t bcidx */
/* ) { */
/*   int isnew  = 0; */
/*   struct pmlag_rt_entry *rt_entry; */

/*   // Lock the routing table */
/*   pthread_mutex_lock(mtx); */

/*   // Attempt to fetch the rt entry */
/*   rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac })); */

/*   // None given, build new one */
/*   if (!rt_entry) { */
/*     rt_entry             = calloc(1, sizeof(struct pmlag_rt_entry)); */
/*     rt_entry->mac        = malloc(ETH_ALEN); */
/*     rt_entry->bcidx      = 0; */
/*     rt_entry->interfaces = calloc(iface->bond->iface_cnt, sizeof(struct pmlag_iface *)); */
/*     memcpy(rt_entry->mac, mac, ETH_ALEN); */
/*     isnew = 1; */
/*   } */

/*   // Bail if */
/*   if ( */
/*       (!bcidx && rt_entry->bcidx) ||  // We receive a regular packet on bcidx-tracked entry */
/*       (rt_entry->bcidx && ((bcidx - rt_entry->bcidx) < 0)) // Or the received bcidx is lower than known (old packet) */
/*   ) { */
/*     if (isnew) { */
/*       rt_purge(rt_entry, NULL); */
/*     } */
/*     pthread_mutex_unlock(mtx); */
/*     return 0; */
/*   } */

/*   // Clear list of known interfaces if */
/*   if ( */
/*     (bcidx && (rt_entry->bcidx != bcidx)) || // We got a NEW broadcast index */
/*     (!bcidx && (rt_entry->bcidx == 0))       // Or we're updating a non-pmlag remote */
/*   ) { */
/*     rt_entry->iface_cnt = 0; */
/*   } */

/*   // Update the rt_entry's broadcast index */
/*   rt_entry->bcidx = bcidx; */

/*   // Bail if rt_entry->iface_cnt+1 lg bond->iface_cnt */
/*   if ((rt_entry->iface_cnt + 1) > iface->bond->iface_cnt) { */
/*     if (isnew) { */
/*       rt_purge(rt_entry, NULL); */
/*     } */
/*     pthread_mutex_unlock(mtx); */
/*     return 0; */
/*   } */

/*   // Add our iface to the entry's interface list */
/*   rt_entry->interfaces[rt_entry->iface_cnt] = iface; */
/*   rt_entry->iface_cnt++; */

/*   // Ensure the entry is in the rt */
/*   if (isnew) { */
/*     mindex_set(rt, rt_entry); */
/*   } */

/*   if (mindex_length(rt) > RT_MAX_ENTRIES) { */
/*     rt_entry = mindex_rand(rt); */
/*     if (rt_entry) { */
/*       mindex_delete(rt, rt_entry); */
/*     } */
/*   } */

/*   pthread_mutex_unlock(mtx); */
/*   return 0; */
/* } */

/* struct pmlag_iface * rt_find( */
/*   struct mindex_t *rt, */
/*   pthread_mutex_t *mtx, */
/*   unsigned char *mac */
/* ) { */
/*   struct pmlag_rt_entry *rt_entry; */

/*   // Attempt to fetch the rt entry */
/*   pthread_mutex_lock(mtx); */
/*   rt_entry = mindex_get(rt, &((struct pmlag_rt_entry){ .mac = mac })); */
/*   if (!rt_entry) { */
/*     pthread_mutex_unlock(mtx); */
/*     return NULL; */
/*   } */

/*   // Unlock the routing table again */
/*   struct pmlag_iface *iface = rt_entry->interfaces[rand() % rt_entry->iface_cnt]; */
/*   pthread_mutex_unlock(mtx); */
/*   return iface; */
/* } */
