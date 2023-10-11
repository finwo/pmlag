#ifndef __PMLAG_UTIL_CONFIG_H__
#define __PMLAG_UTIL_CONFIG_H__

#include <stdint.h>

#define PMLAG_ENTITY_TYPE        int
#define PMLAG_ENTITY_TYPE_IFACE  1
#define PMLAG_ENTITY_TYPE_BOND   2

#define PMLAG_STATE            int
#define PMLAG_STATE_TRIGGERED  1

struct pmlag_iface {
  PMLAG_ENTITY_TYPE   type;
  char               *name;
  int                 ifidx;
  struct pmlag_bond  *bond;
  int                 sockfd;
  struct epoll_event *epev;
};

struct pmlag_bond {
  PMLAG_ENTITY_TYPE    type;
  char                *name;
  struct epoll_event  *epev;
  PMLAG_STATE          state;
  int                  bc_timer;
  uint16_t             bc_id;
  struct pmlag_iface **iface;
  int                  iface_count;
  int                  sockfd;
  unsigned char       *hwaddr; // hwaddr to use for the interface (null = random)
  struct mindex_t     *rt;
};

struct pmlag_configuration {
  struct pmlag_bond **bond;
  int                 bond_count;
};

struct pmlag_configuration * config_load(char * filepath, struct pmlag_configuration *config);

#endif // __PMLAG_UTIL_CONFIG_H__
