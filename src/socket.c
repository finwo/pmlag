#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

int iface_idx(int sockfd, char * ifname) {
  struct ifreq ifr;
  bzero(&ifr, sizeof(ifr));

  // Get interface index
  strncpy((char *)ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ioctl(sockfd, SIOCGIFINDEX, &ifr)) == -1) {
    perror("Error getting interface index");
    close(sockfd);
    return -1;
  }

  return ifr.ifr_ifindex;
}

int sockraw_open(char * ifname) {

  // Open socket
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sockfd < 0) {
    perror("Error opening socket");
    close(sockfd);
    return -1;
  }

  // Bind socket to interface
  struct sockaddr_ll sll;
  bzero(&sll, sizeof(sll));
  sll.sll_family   = AF_PACKET;
  sll.sll_ifindex  = iface_idx(sockfd, ifname);
  sll.sll_protocol = htons(ETH_P_ALL);
  if (sll.sll_ifindex < 0) {
    // Error already printed, not reprinting
    return -1;
  }
  if((bind(sockfd, (struct sockaddr *)&sll, sizeof(sll))) == -1) {
    perror("Error binding raw socket to interface");
    close(sockfd);
    return -1;
  }

  // Return the prepared socket
  return sockfd;
}