#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "util/config.h"

#include "config.h"

struct cnf_directive * cfg_parse_redistribute(FILE *fd, struct cnf_directive *dir, void *user) {
  int i;

  if (dir->argc < 1) {
    fprintf(stderr, "`redistribute` directive needs at least 1 argument\n");
    exit(1);
  }

  // TODO: start building redistribute entry


  for(;;) {
    if (dir) cnf_directive_free(dir);
    dir = cnf_directive_read(fd);
    if (!dir) break; // EOF = done
    // Make following blocks identical in structure
    if(0) {}

    else if (!strcasecmp("ttl", dir->name)) {
      // TODO: route->ttl = atoi(argv[0])
    }

    else if (!strcasecmp("metric", dir->name)) {
      // TODO: route->metric = atoi(argv[0])
    }

    else {
      // unknown directive
      break;
    }
  }

  // Cleanup
  // TODO: store redist in current router def

  // Return the unparsed dir if set
  return dir;
}

struct cnf_directive * cfg_parse_route(FILE *fd, struct cnf_directive *dir, void *user) {
  int i;

  if (dir->argc < 1) {
    fprintf(stderr, "`route` directive needs at least 1\n");
    exit(1);
  }

  // TODO: start building route

  for(;;) {
    if (dir) cnf_directive_free(dir);
    dir = cnf_directive_read(fd);
    if (!dir) break; // EOF = done
cfg_parse_route_reparse:
    // Make following blocks identical in structure
    if(0) {}

    else if (!strcasecmp("ttl", dir->name)) {
      // TODO: route->ttl = atoi(argv[0])
    }

    else if (!strcasecmp("metric", dir->name)) {
      // TODO: route->metric = atoi(argv[0])
    }

    else if (!strcasecmp("neighbour", dir->name)) {
      // TODO: route->neighbour[] = parse_mac(argv[0])
    }

    else {
      // unknown directive
      break;
    }
  }

  // Cleanup
  // TODO: store route in current router def

  // Return the unparsed dir if set
  return dir;
}

struct cnf_directive * cfg_parse_router(FILE *fd, struct cnf_directive *dir, void *user) {
  int i;

  if (dir->argc != 1) {
    fprintf(stderr, "`router` directive accepts only 1 argument\n");
    exit(1);
  }

  printf("Defining router: %s\n", dir->argv[0]);

  for(;;) {
    if (dir) cnf_directive_free(dir);
    dir = cnf_directive_read(fd);
    if (!dir) break; // EOF = done

cfg_parse_router_reparse:

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

    // else if (!strcasecmp("mac", dir->name)) {
    //   if (dir->argc != 1) {
    //     fprintf(stderr, "`mac` directive accepts only 1 argument\n");
    //     exit(1);
    //   }
    //   printf("  set mac: %s\n", dir->argv[0]);
    // }

    else if (!strcasecmp("route", dir->name)) {
      dir = cfg_parse_route(fd, dir, user);
      if (!dir) continue;
      goto cfg_parse_router_reparse;
    }

    else if (!strcasecmp("redistribute", dir->name)) {
      dir = cfg_parse_redistribute(fd, dir, user);
      if (!dir) continue;
      goto cfg_parse_router_reparse;
    }


    else {
      // unknown directive
      break;
    }

  }

  // TODO: store router in actual config

  return dir;
}
