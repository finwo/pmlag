#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "benhoyt/inih.h"
#include "config.h"

static int config_load_handler(
  void *user,
  const char *section,
  const char *name,
  const char *value
) {
  struct pmlag_configuration* config = (struct pmlag_configuration *) user;

  // Get bond being configured
  struct pmlag_bond *bond = config->bonds;
  while(bond) {
    if (!strcmp(bond->name, section)) {
      break;
    }
    bond = bond->next;
  }

  // Create bond if not found
  if (!bond) {
    bond          = calloc(1, sizeof(struct pmlag_bond));
    bond->next    = config->bonds;
    bond->name    = strdup(section);
    config->bonds = bond;
  }

  // Get interface being configured
  struct pmlag_iface *iface = bond->interfaces;

  if (0) {
    // Intentionally empty
  } else if (!strcmp(name, "mode")) {
    // Set the mode according to found value
    if (0) {
      // Intentionally empty
    } else if (!strcmp(value, "active-backup")) {
      bond->mode = PMLAG_MODE_ACTIVE_BACKUP;
    } else if (!strcmp(value, "balanced-rr")) {
      bond->mode = PMLAG_MODE_BALANCED_RR;
    } else if (!strcmp(value, "broadcast")) {
      bond->mode = PMLAG_MODE_BROADCAST;
    } else {
      return 0;
    }
  } else if (!strcmp(name, "interface")) {

    // Select the right interface
    while(iface) {
      if (!strcmp(iface->name, value)) {
        break;
      }
      iface = iface->next;
    }

    // Create iface if not found
    if (!iface) {
      iface            = calloc(1, sizeof(struct pmlag_iface));
      iface->next      = bond->interfaces;
      iface->name      = strdup(value);
      /* iface->weight    = 10; */
      iface->bond      = bond;
      bond->interfaces = iface;
    }

  /* } else if (!strcmp(name, "weight")) { */
  /*   if (!iface) { */
  /*     return 0; */
  /*   } */
  /*   iface->weight = atoi(value); */
  } else {
    // Unknown key
    return 0;
  }

  return 1;
}

struct pmlag_configuration * config_load(const char * filename) {
  // Load config, entry-by-entry
  struct pmlag_configuration *config = calloc(1, sizeof(struct pmlag_configuration));
  if (ini_parse(filename, config_load_handler, config) < 0) {
    fprintf(stderr, "Can not load %s\n", filename);
    return NULL;
  }
  return config;
}
