#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "util/config.h"

#include "config.h"

struct cnf_directive * cfg_parse_bond(FILE *fd, struct cnf_directive *dir, void *user) {
  int i;

  if (dir->argc != 1) {
    fprintf(stderr, "`bond` directive accepts only 1 argument\n");
    exit(1);
  }

  printf("Defining bond: %s\n", dir->argv[0]);

  for(;;) {
    if (dir) cnf_directive_free(dir);
    dir = cnf_directive_read(fd);
    if (!dir) break; // EOF = done

    // Make following blocks identical in structure
    if(0) {}

    else if (!strcasecmp("interface", dir->name)) {
      for( i = 0 ; i < dir->argc ; i++ ) {
        printf("  add iface: %s\n", dir->argv[i]);
      }
    }

    else if (!strcasecmp("address", dir->name)) {
      for( i = 0 ; i < dir->argc ; i++ ) {
        printf("  got addr: %s\n", dir->argv[i]);
      }
    }

    else if (!strcasecmp("mac", dir->name)) {
      if (dir->argc != 1) {
        fprintf(stderr, "`mac` directive accepts only 1 argument\n");
        exit(1);
      }
      printf("  set mac: %s\n", dir->argv[0]);
    }

    else {
      // unknown directive
      return dir;
    }

  }

  return NULL;
}
