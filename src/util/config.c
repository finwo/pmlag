#include <linux/if_ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "benhoyt/inih.h"
#include "config.h"
#include "socket.h"

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
  pmlag_iface_llist *iface_entry = bond->interfaces;

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
    while(iface_entry) {
      if (!strcmp(iface_entry->data->name, value)) {
        break;
      }
      iface_entry = iface_entry->next;
    }

    // Create iface if not found
    if (!iface_entry) {
      // Create the list entry
      iface_entry = calloc(1, sizeof(pmlag_iface_llist));
      iface_entry->next = bond->interfaces;
      bond->interfaces  = iface_entry;
      // Create the iface
      iface_entry->data       = calloc(1, sizeof(struct pmlag_iface));
      iface_entry->data->name = strdup(value);
      iface_entry->data->bond = bond;
    }

  } else if (!strcmp(name, "hwaddr")) {

    // Find iface with name == value
    iface_entry = bond->interfaces;
    while(iface_entry) {
      if (!strcmp(iface_entry->data->name, value)) break;
      iface_entry = iface_entry->next;
    }

    if (iface_entry) {
      // Got interface by that name = use it's hwaddr
      bond->hwaddr = iface_mac(iface_entry->data->name);
    } else if (!strcmp(value, "random")) {
      // "random" = null, a.k.a. let the kernel generate a random mac
      if (bond->hwaddr) free(bond->hwaddr);
      bond->hwaddr = NULL;
    } else {
      // xx:xx:xx:xx:xx:xx
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
