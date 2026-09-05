#include "util/config.h"

#include "setup.h"
#include "config.h"

void daemon_bond_setup() {
  cfg_register_directive("bond", cfg_parse_bond);
}
