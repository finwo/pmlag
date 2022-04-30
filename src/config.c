#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "ini.h"
#include "main.h"

static int config_load_handler(
  void *user,
  const char *section,
  const char *name,
  const char *value
) {
  pmlag_configuration* config = (pmlag_configuration *) user;

  // Get interface being configured
  pmlag_interface *iface = config->interfaces;
  while(iface) {
    if (!strcmp(iface->name, section)) {
      break;
    }
    iface = iface->next;
  }

  // Create interface if not found
  if (!iface) {
    iface              = calloc(1, sizeof(pmlag_interface));
    iface->next        = config->interfaces;
    iface->name        = strdup(section);
    config->interfaces = iface;
  }


  // Set found value
  if (!strcmp("master", name)) {
    iface->master = strdup(value);
  } else if (!strcmp("weight", name)) {
    iface->weight = atoi(value);
  } else if (!strcmp("broadcast", name)) {
    if (!strcmp("flood", value)) {
      iface->broadcast = BROADCAST_FLOOD;
    } else if (!strcmp("balanced", value)) {
      iface->broadcast = BROADCAST_BALANCED;
    } else {
      fprintf(stderr, "Unknown broadcast: %s\n", value);
      return 0;
    }
  } else if (!strcmp("mode", name)) {
    if (!strcmp("slave", value)) {
      iface->mode = MODE_SLAVE;
    } else if (!strcmp("active-backup", value)) {
      iface->mode = MODE_ACTIVE_BACKUP;
    } else if (!strcmp("broadcast", value)) {
      iface->mode = MODE_BROADCAST;
    } else if (!strcmp("balance-rr", value)) {
      iface->mode = MODE_BALANCE_RR;
    } else {
      fprintf(stderr, "Unknown mode: %s\n", value);
      return 0;
    }
  } else if (!strcmp("mac", name)) {
    // TODO: parse mac address
    iface->mac = strdup(value);
  } else {
    return 0;
  }

  return 1;
}

void config_free(pmlag_configuration *config) {
  if (!config) return;
  pmlag_interface *iface_next;
  pmlag_interface *iface = config->interfaces;
  while(iface) {
    iface_next = iface->next;
    if (iface_next == config->interfaces) break;
    if (iface->mac   ) free(iface->mac);
    if (iface->master) free(iface->master);
    if (iface->name  ) free(iface->name);
    free(iface);
    iface = iface_next;
  }
  free(config);
}

pmlag_configuration * config_load(const char * filename) {
  // Load config, entry-by-entry
  pmlag_configuration *config = calloc(1, sizeof(pmlag_configuration));
  if (ini_parse(filename, config_load_handler, config) < 0) {
    fprintf(stderr, "Can not load %s\n", filename);
    return NULL;
  }
  // Loop interface list
  pmlag_interface *iface = config->interfaces;
  while (iface && iface->next) iface = iface->next;
  if (iface) iface->next = config->interfaces;
  // Return produce
  return config;
}
