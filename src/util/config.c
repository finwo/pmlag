#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "benhoyt/inih.h"
#include "finwo/mindex.h"

int mindex_bond_cmp(const void *a, const void *b, void *udata) {
  (void)udata;
  const struct pmlag_configuration_bond *_a = a;
  const struct pmlag_configuration_bond *_b = b;
  return strcmp(_a->name, _b->name);
}

void mindex_bond_purge(void *item, void *udata) {
  struct pmlag_configuration_bond *bond = item;
  (void)udata;
  if (!item) return;
  if (bond->name) free(bond->name);
}

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static int config_load_handler(
  void *user,
  const char *section,
  const char *name,
  const char *value
) {
  // struct pmlag_bond  *bond  = NULL;
  // struct pmlag_iface *iface = NULL;
  // int bond_index = 0;
  // int iface_index = 0;
  struct pmlag_configuration* config = (struct pmlag_configuration *) user;

  // Detect invalid entries
  int i, l;
  for( i = 0, l = strlen(name) ; i < l ; i++ ) {
    if (name[i] == 0x5F                   ) continue; // '_'
    if (name[i] >= 0x30 && name[i] <= 0x39) continue; // 0-9
    if (name[i] >= 0x41 && name[i] <= 0x5A) continue; // A-Z
    if (name[i] >= 0x61 && name[i] <= 0x7A) continue; // a-z
    return 0; // Reject
  }

  // Strip # comments from values
  char *found_comment = strstr(value, "#");
  if (found_comment) {
    for(; found_comment > (value + 1) ;) {
      if ( *(found_comment) >= 0x21 && *(found_comment-1) <= 0x7E ) break;
      found_comment--;
    }
    *found_comment = 0x00;
  }

  // Find or create bond
  struct pmlag_configuration_bond *config_bond = mindex_get(config->bonds, (&(struct pmlag_configuration_bond){ .name = (char *)section }));
  if (!config_bond) {
    config_bond = calloc(1, sizeof(struct pmlag_configuration_bond));
    config_bond->name = strdup(section);
    mindex_set(config->bonds, config_bond);
  }

  // // Find the bond
  // struct pmlag_configuration_bond *config_bond = NULL;
  // for( i = 0 ; i < config->bond_len ; i++ ) {
  //   if (!strcmp(config->bonds[i].name, section)) {
  //     config_bond = &(config->bonds[i]);
  //     break;
  //   }
  // }
  // printf("%s:%d\n", __FILE__, __LINE__);
  // // Build new bond if missing
  // if (!config_bond) {
  //   // Expand array size if needed
  //   if ((config->bond_len+1) > config->bond_cap) {
  //     config->bond_cap = MIN(1, config->bond_cap*2);
  //     config->bonds = realloc(config->bonds, config->bond_cap * sizeof(struct pmlag_configuration_bond));
  //   }
  //   // Append new entry to list
  //   memset(&(config->bonds[config->bond_len]), 0, sizeof(struct pmlag_configuration_bond));
  //   config_bond = &(config->bonds[config->bond_len]);
  //   config->bond_len++;
  // }

  // Detect # comments and remove them

  return 1;
}

struct pmlag_configuration * config_load(char * filepath, struct pmlag_configuration *config) {
  // Load config, entry-by-entry
  if (!config) {
    config = calloc(1, sizeof(struct pmlag_configuration));
  }
  if (!config->bonds) {
    config->bonds = mindex_init(mindex_bond_cmp, mindex_bond_purge, NULL);
  }

  if (ini_parse(filepath, config_load_handler, config) < 0) {
    fprintf(stderr, "Can not load %s\n", filepath);
    return NULL;
  }

  printf("bond_len: %ld\n", mindex_length(config->bonds));
  printf("\n");
  printf("bonds:\n");
  int l = mindex_length(config->bonds);
  for(int i = 0 ; i < l ; i++) {
    struct pmlag_configuration_bond *bond = mindex_nth(config->bonds, i);
    printf("  - %s\n", bond->name);
  }
  printf("\n");

  return config;
}
