#ifndef __PMLAG_UTIL_RT_H__
#define __PMLAG_UTIL_RT_H__

#include "config.h"

#include "finwo/mindex.h"

struct pmlag_rt_entry {
  unsigned char *mac;            // mac address of the remote entity
  int16_t bcidx;                 // broadcast index last seen from the mac
  pmlag_iface_llist *interfaces; // list of pointers to interfaces
};

struct mindex_t * rt_init(void *udata);

int rt_upsert(
  struct mindex_t *rt,
  pthread_mutex_t *mtx,
  struct pmlag_iface *iface,
  unsigned char *mac,
  int16_t bcidx
);

#endif // __PMLAG_UTIL_RT_H__
