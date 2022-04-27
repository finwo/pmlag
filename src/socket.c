#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <linux/if_packet.h>

int sockraw_open(char * ifname) {

  // Open socket
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sockfd < 0) {
    perror("Error opening socket");
    close(sockfd);
    return -1;
  }

  struct sockaddr_ll sll;
  struct ifreq ifr;
  bzero(&sll, sizeof(sll));
  bzero(&ifr, sizeof(ifr));

  // Get interface index
  strncpy((char *)ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ioctl(sockfd, SIOCGIFINDEX, &ifr)) == -1) {
    perror("Error getting interface index");
    close(sockfd);
    return -1;
  }

  // Bind socket to interface
  sll.sll_family   = AF_PACKET;
  sll.sll_ifindex  = ifr.ifr_ifindex;
  sll.sll_protocol = htons(ETH_P_ALL);
  if((bind(sockfd, (struct sockaddr *)&sll, sizeof(sll))) == -1) {
    perror("Error binding raw socket to interface");
    close(sockfd);
    return -1;
  }

  // Return the prepared socket
  return sockfd;
}


#ifdef __cplusplus
} // extern "C"
#endif
