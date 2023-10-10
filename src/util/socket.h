#ifndef __PMLAG_UTIL_SOCKET_H__
#define __PMLAG_UTIL_SOCKET_H__

unsigned char * iface_mac(char * ifname);
int tap_alloc(char * ifname, unsigned char * mac);
int sockraw_open(char * ifname);

#endif // __PMLAG_UTIL_SOCKET_H__
