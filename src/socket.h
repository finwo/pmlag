#ifndef __PMLAG_SOCKET_H__
#define __PMLAG_SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

unsigned char * iface_mac(char * ifname);
int tap_alloc(char * ifname, unsigned char * mac);
int sockraw_open(char * ifname);
int iface_idx(int sockfd, char * ifname);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __PMLAG_SOCKET_H__
