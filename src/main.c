// vim: fdm=marker

#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

#include "cofyc/argparse.h"
#include "util/config.h"

#include "task/bond.h"

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

int main(int argc, const char **argv) {
  char *config_file="/etc/pmlag/pmlag.ini";

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
  struct pmlag_configuration *config = config_load(config_file);
  if (!config) {
    return 1;
  }

  // Initialize interfaces for all configured bonds
  struct pmlag_bond *bond = config->bonds;
  while(bond) {

    // Start the bond's thread
    if(pthread_create(&(bond->tid_bond), NULL, task_bond_thread, bond)) {
      perror("Starting bond thread");
      return 1;
    }

    bond = bond->next;
  }

  // Wait for all bonds to finish
  bond = config->bonds;
  while(bond) {
    pthread_join(bond->tid_bond, NULL);
    bond = bond->next;
  }

  return 0;
}
