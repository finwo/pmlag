#ifndef __PMLAG_CONFIG_H__
#define __PMLAG_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PMLAG_MODE_NONE          0
#define PMLAG_MODE_ACTIVE_BACKUP 1
#define PMLAG_MODE_BROADCAST     2
#define PMLAG_MODE_BALANCED_RR   3

struct pmlag_interface {
  void *next;
  char *name;
  int weight;
};

struct pmlag_bond {
  void *next;
  char *name;
  int   mode;
  struct pmlag_interface *interfaces;
};

struct pmlag_configuration {
  struct pmlag_bond *bonds;
};

struct pmlag_configuration * config_load(const char * filename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PMLAG_CONFIG_H__
