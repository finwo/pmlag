/* #ifndef __PMLAG_UTIL_RT_H__ */
/* #define __PMLAG_UTIL_RT_H__ */

/* #include "config.h" */

/* #include "finwo/mindex.h" */

/* struct pmlag_rt_entry { */
/*   unsigned char *mac;              // mac address of the remote entity */
/*   int16_t bcidx;                   // broadcast index last seen from the mac */
/*   int16_t iface_cnt;               // amount of interfaces this mac is available on */
/*   struct pmlag_iface **interfaces; // list of pointers to interfaces */
/* }; */

/* struct mindex_t * rt_init(void *udata); */

/* int rt_upsert( */
/*   struct mindex_t *rt, */
/*   pthread_mutex_t *mtx, */
/*   struct pmlag_iface *iface, */
/*   unsigned char *mac, */
/*   int16_t bcidx */
/* ); */

/* struct pmlag_iface * rt_find( */
/*   struct mindex_t *rt, */
/*   pthread_mutex_t *mtx, */
/*   unsigned char *mac */
/* ); */

/* #endif // __PMLAG_UTIL_RT_H__ */
