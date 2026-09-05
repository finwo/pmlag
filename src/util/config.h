#ifndef __PMLAG_UTIL_CONFIG_H__
#define __PMLAG_UTIL_CONFIG_H__

#include <stdio.h>

#include "finwo/cnfparse.h"

#define CFG_RET_ERROR -1
#define CFG_RET_OK     0

typedef struct cnf_directive * (*cfg_directive_fn)(FILE *fd, struct cnf_directive *dir, void *user);
void cfg_register_directive(const char *name, cfg_directive_fn fn);
int cfg_parse(const char *wd, FILE *fd, void *user);

#endif // __PMLAG_UTIL_CONFIG_H__
