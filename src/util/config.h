#ifndef __PMLAG_UTIL_CONFIG_H__
#define __PMLAG_UTIL_CONFIG_H__

#include <stdint.h>

#include "finwo/mindex.h"

struct pmlag_configuration_bond {
  char *name;
  char **interface;
  uint16_t interface_count;
  uint16_t tick_rate;
};

struct pmlag_configuration {
  struct mindex_t *bonds;
};

struct pmlag_configuration * config_load(char * filepath, struct pmlag_configuration *config);

#endif // __PMLAG_UTIL_CONFIG_H__
