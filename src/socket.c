#include <fcntl.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

int iface_idx(int sockfd, char * ifname) {
  struct ifreq ifr;
  bzero(&ifr, sizeof(ifr));

  // Get interface index
  strncpy((char *)ifr.ifr_name, ifname, IFNAMSIZ);
  if ((ioctl(sockfd, SIOCGIFINDEX, &ifr)) == -1) {
    fprintf(stderr, "Error getting interface index for %s\n", ifname);
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

int tap_alloc(char * ifname) {
    struct ifreq ifr;
    int fd, err;

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
      perror("Open tun");
      return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if( *ifname ) {
       strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
       close(fd);
       return err;
    }

    strcpy(ifname, ifr.ifr_name);
    return fd;
}
