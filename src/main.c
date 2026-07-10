#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cofyc/argparse.h"

#include "util/config.h"

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

int main(int argc, const char **argv) {
  char *config_file="/etc/pmlag/pmlag.conf";

  // Seed random
  uint32_t seed;
  FILE* urandom = fopen("/dev/urandom", "r");
  fread(&seed, sizeof(uint32_t), 1, urandom);
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

  struct pmlag_configuration *config = calloc(1, sizeof(struct pmlag_configuration));

  printf("Config file: %s\n", config_file);
  config_load(config_file, config);

  return 0;
}
