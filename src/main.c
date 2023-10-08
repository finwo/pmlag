#include <stdio.h>
#include <stdlib.h>

#include "cofyc/argparse.h"

#include "util/config.h"

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
  struct pmlag_configuration *config = config_load(config_file, NULL);
  if (!config) {
    return 1;
  }

  // Display bonds
  int b = 0;
  int i = 0;
  for( b = 0 ; b < config->bond_count ; b++ ) {
    printf("%s\n", config->bond[b]->name);
    for( i = 0 ; i < config->bond[b]->iface_count ; i++ ) {
      printf("  - %s\n", config->bond[b]->iface[i]->name);
    }
  }

  return 69;
}
