#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/if_packet.h>

#include "socket.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INTERFACE "wlp2s0"
#define ERR_NAME "pmlag"

int main(int argc, char **argv) {
  int sockfd = sockraw_open(INTERFACE);

  // Prepare ingress buffer
  int buflen;
  unsigned char *buffer = (unsigned char *) malloc(65536);
  memset(buffer, 0, 65536);
  struct sockaddr saddr;
  int saddr_len  = sizeof(saddr);

  // Fetch a packet
  buflen = recvfrom(sockfd, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);
  if (buflen < 0) {
    perror(ERR_NAME ": recvfrom");
    exit(EXIT_FAILURE);
  }

  printf("\n");
  printf("Got packet: %d bytes\n", buflen);
  printf("\n");

  struct ethhdr *eth = (struct ethhdr *)(buffer);
  printf("Ethernet header\n");
  printf("\t|- DST    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
  printf("\t|- SRC    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
  printf("\t|- PROTO  %d\n",eth->h_proto);

  printf("\n");

  struct sockaddr_in source;
  struct sockaddr_in dest;
  if (eth->h_proto == 8) {
    unsigned short iphdrlen;
    struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = ip->saddr;
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = ip->daddr;

    printf("IP header\n");
    printf("\t|- Version          %d\n",(unsigned int)ip->version);
    printf("\t|- Internet hlen    %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4);
    printf("\t|- Type Of Service  %d\n",(unsigned int)ip->tos);
    printf("\t|- Total Length     %d Bytes\n",ntohs(ip->tot_len));
    printf("\t|- Identification   %d\n",ntohs(ip->id));
    printf("\t|- Time To Live     %d\n",(unsigned int)ip->ttl);
    printf("\t|- Protocol         %d\n",(unsigned int)ip->protocol);
    printf("\t|- Header Checksum  %d\n",ntohs(ip->check));
    printf("\t|- Source IP        %s\n", inet_ntoa(source.sin_addr));
    printf("\t|- Destination IP   %s\n",inet_ntoa(dest.sin_addr));

    printf("\n");
  }

  return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif
