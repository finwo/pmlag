#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cofyc/argparse.h"

#include "util/config-root.h"

static const char *const usage[] = {
  __NAME " [options]",
  __NAME " --help",
  NULL,
};

int main(int argc, const char **argv) {
  char *config_file="/etc/network/pmlag";
  FILE *config_fd;

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
      "\n"
      __NAME " is a tool for bonding network interfaces together when the hardware\n"
      "on the other side of the cable(s) doesn't support it.\n"
  );
  argc = argparse_parse(&argparse, argc, argv);

  // Check the config exists
  char *config_file_real = realpath(config_file, NULL);
  if (!config_file_real) {
    fprintf(stderr, "Could not resolve config file `%s`\n", config_file);
    return 1;
  }
  config_fd = fopen(config_file_real, "r");
  if (!config_fd) {
    perror("Could not open config file");
    return 1;
  }

  // Start the actual parse
  char *config_dirname = calloc(strlen(config_file_real)+1, 1);
  strcpy(config_dirname, config_file_real);
  dirname(config_dirname);
  if (cfg_parse(config_dirname, config_fd, NULL) < 0) {
    fprintf(stderr, "Error during reading configuration from %s\n", config_file_real);
    return 1;
  }

//   struct pmlag_configuration *config = calloc(1, sizeof(struct pmlag_configuration));

  // printf("Config file: %s\n", config_file);
  // config_file = realpath(config_file, NULL);
  // printf("Realpath   : %s\n", config_file);
  // printf("dirname    : %s\n", dirname(config_file));
  // // cfg_parse(
// //   config_load(config_file, config);

//   return 0;
  return 42;
}

#ifdef __cplusplus
} // extern "C"
#endif
