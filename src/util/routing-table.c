#include <linux/if_ether.h>
#include <string.h>

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
