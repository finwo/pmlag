#ifndef __PMLAG_UTIL_RT_H__
#define __PMLAG_UTIL_RT_H__

#include "config.h"

struct pmlag_rt_entry {
  unsigned char *mac;            // mac address of the remote entity
  uint16_t bcidx;                // broadcast index last seen from the mac
  pmlag_iface_llist *interfaces; // list of pointers to interfaces
};

struct btree * rt_init(void *udata);

#endif // __PMLAG_UTIL_RT_H__
