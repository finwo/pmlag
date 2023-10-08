#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "benhoyt/inih.h"

#include "config.h"
#include "socket.h"

static int config_load_handler(
  void *user,
  const char *section,
  const char *name,
  const char *value
) {
  struct pmlag_bond  *bond  = NULL;
  struct pmlag_iface *iface = NULL;
  int bond_idx = 0;
  int iface_idx = 0;
  struct pmlag_configuration* config = (struct pmlag_configuration *) user;
  printf("[%s].%s: %s\n", section, name, value);

  // Ensure we have a bond list
  if (!config->bond) config->bond = calloc(1, sizeof(void*));

  // Find the bond being configured
  for( bond_idx = 0 ; bond_idx < config->bond_count ; bond_idx ++ ) {
    if (!strcmp(config->bond[bond_idx]->name, section)) {
      bond = config->bond[bond_idx];
      break;
    }
  }

  // Create bond if not found
  if (!bond) {
    config->bond[config->bond_count++] = bond = calloc(1, sizeof(struct pmlag_bond));
    config->bond                       = realloc(config->bond, (config->bond_count + 1) * sizeof(void*));
    config->bond[config->bond_count  ] = NULL;
    bond->name                         = strdup(section);
  }

  if (0) {
    // Intentionally empty

  } else if (!strcmp(name, "interface")) {

    // Ensure we have an iface list
    if (!bond->iface) bond->iface = calloc(1, sizeof(void*));

    // Find the interface you're referencing
    for( iface_idx = 0 ; iface_idx < bond->iface_count ; iface_idx ++ ) {
      if (!strcmp(config->bond[bond_idx]->iface[iface_idx]->name, value)) {
        iface = config->bond[bond_idx]->iface[iface_idx];
        break;
      }
    }

    // Create bond if not found
    if (!iface) {
      bond->iface[bond->iface_count++] = iface = calloc(1, sizeof(struct pmlag_iface));
      bond->iface                      = realloc(bond->iface, (bond->iface_count + 1) * sizeof(void*));
      bond->iface[bond->iface_count  ] = NULL;
      iface->name                      = strdup(value);
      iface->bond                      = bond;
    }

  } else if (!strcmp(name, "hwaddr")) {

    // Find the interface you're referencing
    for( iface_idx = 0 ; iface_idx < bond->iface_count ; iface_idx ++ ) {
      if (!strcmp(config->bond[bond_idx]->iface[iface_idx]->name, value)) {
        iface = config->bond[bond_idx]->iface[iface_idx];
        break;
      }
    }

    if (bond->hwaddr) {
      printf("Already had MAC: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
        bond->hwaddr[0],
        bond->hwaddr[1],
        bond->hwaddr[2],
        bond->hwaddr[3],
        bond->hwaddr[4],
        bond->hwaddr[5]
      );
    }

    if (iface) {
      // Got interface by that name = use it's hwaddr
      if (bond->hwaddr) free(bond->hwaddr);
      bond->hwaddr = iface_mac(iface->name);
    } else if (!strcmp(value, "random")) {
      // "random" = null, a.k.a. let the kernel generate a random mac
      if (bond->hwaddr) free(bond->hwaddr);
      bond->hwaddr = NULL;
    } else {
      // xx:xx:xx:xx:xx:xx
      if (bond->hwaddr) free(bond->hwaddr);
      bond->hwaddr = (unsigned char *)malloc(6); // ETH_ALEN should be 6, but this is safer
      sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &(bond->hwaddr[0]),
        &(bond->hwaddr[1]),
        &(bond->hwaddr[2]),
        &(bond->hwaddr[3]),
        &(bond->hwaddr[4]),
        &(bond->hwaddr[5])
      );
    }
  }

  return 1;
}

struct pmlag_configuration * config_load(char * filepath, struct pmlag_configuration *config) {
  // Load config, entry-by-entry
  if (!config) config = calloc(1, sizeof(struct pmlag_configuration));
  if (ini_parse(filepath, config_load_handler, config) < 0) {
    fprintf(stderr, "Can not load %s\n", filepath);
    return NULL;
  }
  return config;
}

