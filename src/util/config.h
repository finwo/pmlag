#ifndef __PMLAG_CONFIG_H__
#define __PMLAG_CONFIG_H__

#include <pthread.h>
#include <stdint.h>

#include "finwo/mindex.h"

#include "linked-list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PMLAG_MODE_NONE          0
#define PMLAG_MODE_ACTIVE_BACKUP 1
#define PMLAG_MODE_BROADCAST     2
#define PMLAG_MODE_BALANCED_RR   3

#define RCVBUFSIZ 65536

#ifndef RT_MAX_ENTRIES
#define RT_MAX_ENTRIES 64
#endif

typedef LLIST(int l; struct pmlag_iface) pmlag_iface_llist;

struct pmlag_iface {
  char *name;              // name of the interface this object represents
  /* int   weight;            // weight of this interface within the bond */
  int   sockfd;            // file descriptor for the iface raw socket
  int   ifidx;             // index of the interface within the socket
  pthread_t tid;           // thread id where the iface listener recides in
  struct pmlag_bond *bond; // reference to the bond this iface belongs to
};

struct pmlag_bond {
  void *next;                    // linked-list next reference
  char *name;                    // name of the bond interface
  int   mode;                    // which mode to run pmlag in for this bond
  int   sockfd;                  // file descriptor for the bond socket interface
  pthread_t tid_bond;            // thread id where the bond interface listener recides in
  pthread_t tid_announce;        // thread id that'll output announces for the bond
  pthread_mutex_t mtx_rt;        // lock for the routing table of the bond
  struct mindex_t *rt;           // pointer to the routing table
  pmlag_iface_llist *interfaces; // linked-list of interfaces contained in the bond
};

struct pmlag_configuration {
  struct pmlag_bond *bonds;
};

struct pmlag_configuration * config_load(const char * filename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PMLAG_CONFIG_H__
