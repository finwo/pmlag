#ifndef __PMLAG_UTIL_CONFIG_H__
#define __PMLAG_UTIL_CONFIG_H__

#define PMLAG_ENTITY_TYPE       int
#define PMLAG_ENTITY_TYPE_IFACE 1
#define PMLAG_ENTITY_TYPE_BOND  2

struct pmlag_iface {
  PMLAG_ENTITY_TYPE type;
  char *name;
  struct pmlag_bond *bond;
  int sockfd;
  struct epoll_event *epev;
  /* /1* int   weight;        // weight of this interface within the bond *1/ */
  /* int   sockfd;            // file descriptor for the iface raw socket */
  /* int   ifidx;             // index of the interface within the socket */
  /* pthread_t tid;           // thread id where the iface listener recides in */
  /* struct pmlag_bond *bond; // reference to the bond this iface belongs to */
};

struct pmlag_bond {
  PMLAG_ENTITY_TYPE type;
  char *name;
  struct pmlag_iface **iface;
  int                  iface_count;
  unsigned char *hwaddr;         // hwaddr to use for the interface (null = random)

/*   int   mode;                    // which mode to run pmlag in for this bond */
/*   int   sockfd;                  // file descriptor for the bond socket interface */
/*   pthread_t tid_bond;            // thread id where the bond interface listener recides in */
/*   pthread_t tid_announce;        // thread id that'll output announces for the bond */
/*   pthread_mutex_t mtx_rt;        // lock for the routing table of the bond */
/*   struct mindex_t *rt;           // pointer to the routing table */
/*   pmlag_iface_llist *interfaces; // linked-list of interfaces contained in the bond */
/*   int iface_cnt;                 // track the number of interfaces in this bond */
};

struct pmlag_configuration {
  struct pmlag_bond **bond;
  int                 bond_count;
};

struct pmlag_configuration * config_load(char * filepath, struct pmlag_configuration *config);

#endif // __PMLAG_UTIL_CONFIG_H__
