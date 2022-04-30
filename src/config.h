#ifndef __PMLAG_CONFIG_H__
#define __PMLAG_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>

typedef struct {
  // Linked list
  void *next;      // Ref to the next interface
  // Config
  char *name;      // Name of the interface
  char *master;    // Name of the controlling interface
  char *mac;       // Mac address to set on the interface
  int broadcast;   // Broadcast mode
  int mode;        // Balancing mode
  int weight;      // Weight of the interface
  // Activity
  int socket;      // Socket file descriptor
  int remainder;   // Round robin remainder
  sem_t sem_queue; // Egress queue
} pmlag_interface;

typedef struct {
  pmlag_interface *interfaces;
} pmlag_configuration;

void config_free(pmlag_configuration *config);
pmlag_configuration * config_load(const char * filename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PMLAG_CONFIG_H__
