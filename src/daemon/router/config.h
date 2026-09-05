#ifndef __PMLAG_DAEMON_ROUTER_CONFIG_H__
#define __PMLAG_DAEMON_ROUTER_CONFIG_H__

#include <stdio.h>

#include "finwo/cnfparse.h"

struct cnf_directive * cfg_parse_router(FILE *fd, struct cnf_directive *dir, void *user);

#endif // __PMLAG_DAEMON_ROUTER_CONFIG_H__
