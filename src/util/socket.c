#include <fcntl.h>
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

unsigned char * iface_mac(char * ifname) {
  struct ifreq ifr;
  unsigned char *mac = calloc(1, ETH_ALEN);
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  ifr.ifr_addr.sa_family = AF_PACKET;
  strncpy(ifr.ifr_name , ifname , IFNAMSIZ-1);
  ioctl(sockfd, SIOCGIFHWADDR, &ifr);
  close(sockfd);
  memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
  return mac;
}

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

int if_ioctl(int cmd, struct ifreq* req) {
  int ret, sock;
  sock = socket(AF_INET, SOCK_PACKET, 0);
  if (sock < 0) {
    return -1;
  }
  ret = ioctl(sock, cmd, req);
  close(sock);
  return ret;
}

int sockraw_open(char * ifname) {
  struct ifreq ifr;

  // Open socket (P_ALL, due to custom ethertype field)
  int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sockfd < 0) {
    perror("Error opening socket");
    close(sockfd);
    return -1;
  }

  /* // Bind socket to interface */
  /* if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname)) < 0) { */
  /*   perror("Error binding raw socket to interface"); */
  /* } */

  // Old bind to socket
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

  // Get current iface configuration
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, ifname);
  if (if_ioctl(SIOCGIFFLAGS, &ifr) < 0) {
    perror("Get interface flags");
  }

  // Set iface to promiscuous mode
  ifr.ifr_flags |= IFF_PROMISC;
  if (if_ioctl(SIOCSIFFLAGS, &ifr) < 0) {
    perror("Configure interface promiscuous mode");
  }

  // Return the prepared socket
  return sockfd;
}

int tap_alloc(char * ifname, unsigned char * mac) {
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
    perror("Open tun");
    return -1;
  }

  // Bring up the interface
  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TAP | IFF_MULTI_QUEUE | IFF_NO_PI;
  if( *ifname ) {
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
  }
  if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
    perror("Open tun");
    close(fd);
    return err;
  }
  strcpy(ifname, ifr.ifr_name);

  // Set the interface's mac address
  if ( mac ) {
    // TODO: random, copied or specific mac address pulled from ini
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data, mac, ETH_ALEN);
    if ((err = if_ioctl(SIOCSIFHWADDR, &ifr)) < 0) {
      perror("Set if hwaddr");
      close(fd);
      return err;
    }
  }

  // Fetch the current flags
  memset(&ifr, 0, sizeof(ifr));
  strcpy(ifr.ifr_name, ifname);
  if (if_ioctl(SIOCGIFFLAGS, &ifr) < 0) {
    perror("failed to get the interface flags");
    close(fd);
    return err;
  }

  // Bring up the interface
  if (!(ifr.ifr_flags & IFF_UP)) {
    ifr.ifr_flags |= IFF_UP;
    if (if_ioctl(SIOCSIFFLAGS, &ifr) < 0) {
      perror("failed to get the interface flags");
      close(fd);
      return err;
    }
  }

  return fd;
}
