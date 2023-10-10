#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>

#include "cofyc/argparse.h"

#include "util/config.h"
#include "util/socket.h"

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

int64_t millis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * ((int64_t)1000)) + (tv.tv_usec / 1000);
}

int main(int argc, const char **argv) {
  char *config_file="/etc/pmlag/pmlag.ini";
  struct epoll_event *epev;
  int epfd;
  struct pmlag_iface *iface;

  // Seed random
  unsigned int seed;
  FILE* urandom = fopen("/dev/urandom", "r");
  fread(&seed, sizeof(int), 1, urandom);
  fclose(urandom);
  srand(seed);

  // Define which options we support
  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_STRING('c', "config", &config_file, "Config file to use", NULL, 0, 0),
    OPT_END(),
  };

  // Parse command line arguments
  struct argparse argparse;
  argparse_init(&argparse, options, usage, 0);
  argparse_describe(&argparse, NULL,
      // TODO: format to terminal width
      "\n" __NAME " is a tool for bonding network interfaces together when the "
      "hardware\non the other side of the cable(s) doesn't support it."
      "\n"
  );
  argc = argparse_parse(&argparse, argc, argv);

  // Load configuration file
  struct pmlag_configuration *config = config_load(config_file, NULL);
  if (!config) {
    return 1;
  }

  // Initialize epoll
  epfd = epoll_create1(0);

  // DEBUG: Display bonds
  int b = 0;
  int i = 0;
  for( b = 0 ; b < config->bond_count ; b++ ) {
    printf("%s\n", config->bond[b]->name);
    for( i = 0 ; i < config->bond[b]->iface_count ; i++ ) {
      printf("  - %s\n", config->bond[b]->iface[i]->name);
    }
  }

  // Wait for things to happen & call tick methods periodically
  int ev_count;
  struct epoll_event events[8];
  int64_t ttime = millis();
  int64_t tdiff = 0;
  while(1) {

    // Calculate waiting time & periodic actions
    tdiff = ttime - millis();
    if (tdiff <= 0) {
      ttime += 1000;
      tdiff += 1000;

      // Handle sockets
      for( b = 0 ; b < config->bond_count ; b++ ) {

        // (re)open bond tap

        for( i = 0 ; i < config->bond[b]->iface_count ; i++ ) {
          iface = config->bond[b]->iface[i];

          // (re)open interface socket
          if (!iface->sockfd) {
            iface->sockfd = sockraw_open(iface->name);
            if (iface->sockfd < 0) {
              perror("sockraw_open");
              iface->sockfd = 0;
              continue;
            }
            printf("Opened socket for %s\n", iface->name);
          }

          // Register it with epoll
          if (!iface->epev) {
            iface->epev = calloc(1, sizeof(struct epoll_event));
            if (!iface->epev) {
              perror("calloc");
              iface->epev = NULL;
              continue;
            }
            iface->epev->events   = EPOLLIN;
            iface->epev->data.ptr = iface;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, iface->sockfd, iface->epev)) {
              free(iface->epev);
              close(iface->sockfd);
              iface->sockfd = 0;
              iface->epev   = NULL;
              continue;
            }
            printf("Registered %s with epoll\n", iface->name);
          }

        }
      }

      // TODO: time tick method (for announce handling)
      printf("ttime: %ld\n", ttime);
    }

    // Handle incoming data
    ev_count = epoll_wait(epfd, events, 8, tdiff);
    for( i=0 ; i<ev_count; i++) {
      // TODO: process(events[i].data.ptr);
    }

  }

  return 69;
}
