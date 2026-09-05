#include "util/config.h"

#include "setup.h"
#include "config.h"

void daemon_router_setup() {
  cfg_register_directive("router", cfg_parse_router);
}
