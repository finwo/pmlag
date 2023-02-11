#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

#include "cofyc/argparse.h"
#include "config.h"
#include "socket.h"

#define INTERFACE "enp0s13f0u2c2"

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

void * thread_iface(void *arg) {
  struct pmlag_iface *iface = (struct pmlag_iface *)arg;

  int sockfd = sockraw_open(iface->name);
  printf("Thread started for iface: %s->%s(%d)\n", iface->bond->name, iface->name, sockfd);

  // TODO: open raw socket on iface
  // TODO: blocking listen on iface, send to bond
  // TODO: track announces in routing table

  sleep(3);
  pthread_exit(NULL);
  return NULL;
}

void * thread_bond(void *arg) {
  struct pmlag_bond *bond = (struct pmlag_bond *)arg;
  printf("Thread started for bond: %s\n", bond->name);

  // For each bond of config->bonds
  struct pmlag_iface *iface = bond->interfaces;
  while(iface) {
    if(pthread_create(&(iface->tid), NULL, thread_iface, iface)) {
      perror("Starting iface thread");
      pthread_exit((void*)1);
      return (void*)1;
    }
    iface = iface->next;
  }

  // TODO: create bonded interface here
  // TODO: blocked listen on bond, send through routing table to other ifaces
  // TODO: send broadcasts to all interfaces
  // TODO: timer to broadcast announce our presence to ifaces (keepalive-ish)
  //          hint: include sequence id, so tracking can react quickly (seq dist > 1)

  // Wait for iface threads to finish
  iface = bond->interfaces;
  while(iface) {
    pthread_join(iface->tid, NULL);
    iface = iface->next;
  }

  pthread_exit(NULL);
  return NULL;
}

int main(int argc, const char **argv) {
  char *config_file="/etc/pmlag/pmlag.ini";

  int rx = 0;
  int tx = 0;

  // Define which options we support
  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_STRING('c', "config", &config_file, "Config file to use", NULL, 0, 0),
    OPT_BOOLEAN('t', "tx", &tx, "Send a message", NULL, 0, 0),
    OPT_BOOLEAN('r', "rx", &rx, "eth dump", NULL, 0, 0),
    OPT_END(),
  };

  // Parse command line arguments
  struct argparse argparse;
  argparse_init(&argparse, options, usage, 0);
  argparse_describe(&argparse, NULL,
      // TODO: format to terminal width
      "\n" __NAME " is a tool for bonding network interfaces together when the "
      "hardware\non the other side of the cable(s) doesn't support it."
      "\n"
  );
  argc = argparse_parse(&argparse, argc, argv);

  /* // Load configuration file */
  /* struct pmlag_configuration *config = config_load(config_file); */
  /* if (!config) { */
  /*   return 1; */
  /* } */

  /* // For each bond of config->bonds */
  /* struct pmlag_bond *bond = config->bonds; */
  /* while(bond) { */
  /*   if(pthread_create(&(bond->tid), NULL, thread_bond, bond)) { */
  /*     perror("Starting bond thread"); */
  /*     return 1; */
  /*   } */
  /*   bond = bond->next; */
  /* } */

  /* // Wait for all bonds to finish */
  /* bond = config->bonds; */
  /* while(bond) { */
  /*   pthread_join(bond->tid, NULL); */
  /*   bond = bond->next; */
  /* } */

  int sockfd;
  /* int sockfd = sockraw_open(INTERFACE); */
  /* if (sockfd < 0) { */
  /*   return 1; */
  /* } */

  if (rx) {
    sockfd = sockraw_open("enp0s13f0u1c2");
    if (sockfd < 0) {
      return 1;
    }

    // Prepare ingress buffer
    int buflen;
    unsigned char *buffer = (unsigned char *) malloc(65536);
    memset(buffer, 0, 65536);
    struct sockaddr saddr;
    int saddr_len  = sizeof(saddr);

    struct ethhdr *eth = (struct ethhdr *)(buffer);
    struct sockaddr_in source;
    struct sockaddr_in dest;

    while(1) {

      // Fetch a packet
      buflen = recvfrom(sockfd, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len);
      if (buflen < 0) {
        perror("recvfrom");
        exit(EXIT_FAILURE);
      }

      printf("\n");
      printf("Got packet: %d bytes\n", buflen);
      printf("\n");

      printf("Ethernet header\n");
      printf("\t|- DST    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
      printf("\t|- SRC    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
      printf("\t|- PROTO  %.4X\n",be16toh(eth->h_proto));

      printf("\n");

      if (be16toh(eth->h_proto) == 0x0800) {
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

    }

  }

  if (tx) {
    sockfd = sockraw_open(INTERFACE);
    if (sockfd < 0) {
      return 1;
    }

    // Prepare egress buffer
    int buflen;
    unsigned char *buffer = (unsigned char *) malloc(65536);
    memset(buffer, 0, 65536);
    struct sockaddr_ll saddr;
    int saddr_len  = sizeof(saddr);

    // Get MAC address of the interface
    struct ifreq ifreq_c;
    memset(&ifreq_c,0,sizeof(ifreq_c));
    strncpy(ifreq_c.ifr_name,INTERFACE,IFNAMSIZ-1);//giving name of Interface
    if((ioctl(sockfd,SIOCGIFHWADDR,&ifreq_c))<0) {
      perror("error in SIOCGIFHWADDR ioctl reading");
      return 1;
    }

    // Get ethhdr reference to the buffer
    struct ethhdr *eth = (struct ethhdr *)(buffer);

    // Fill ethernet source address
    memcpy(eth->h_source, ifreq_c.ifr_hwaddr.sa_data, 6);
    /* eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]); */
    /* eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]); */
    /* eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]); */
    /* eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]); */
    /* eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]); */
    /* eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]); */

    // Broadcast packet
    eth->h_dest[0] = 0xFF;
    eth->h_dest[1] = 0xFF;
    eth->h_dest[2] = 0xFF;
    eth->h_dest[3] = 0xFF;
    eth->h_dest[4] = 0xFF;
    eth->h_dest[5] = 0xFF;

    // Custom protocol (not shown as registered on https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml)
    eth->h_proto = htons(0x0666);

    struct sockaddr_ll sadr_ll;
    sadr_ll.sll_ifindex = iface_idx(sockfd, INTERFACE);
    sadr_ll.sll_halen   = ETH_ALEN; // length of destination mac address
    memcpy(sadr_ll.sll_addr, eth->h_dest, ETH_ALEN);

    /* sadr_ll.sll_addr[0] = 0xFF; */
    /* sadr_ll.sll_addr[1] = 0xFF; */
    /* sadr_ll.sll_addr[2] = 0xFF; */
    /* sadr_ll.sll_addr[3] = 0xFF; */
    /* sadr_ll.sll_addr[4] = 0xFF; */
    /* sadr_ll.sll_addr[5] = 0xFF; */

    /* saddr.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface */
    /* saddr.sll_halen = ETH_ALEN; // length of destination mac address */
    /* saddr.sll_addr[0] = DESTMAC0; */
    /* saddr.sll_addr[1] = DESTMAC1; */
    /* saddr.sll_addr[2] = DESTMAC2; */
    /* saddr.sll_addr[3] = DESTMAC3; */
    /* saddr.sll_addr[4] = DESTMAC4; */
    /* saddr.sll_addr[5] = DESTMAC5; */


    int send_len = sendto(sockfd,buffer, 12 + 2 + 46,0,(const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll));
    if(send_len<0) {
      printf("error in sending....sendlen=%d....errno=%d\n",send_len,errno);
      return -1;
    }

    printf("\n");
    printf("Sent packet: %d bytes\n", send_len);
    printf("\n");

    printf("Ethernet header\n");
    printf("\t|- DST    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
    printf("\t|- SRC    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
    printf("\t|- PROTO  %.4X\n",be16toh(eth->h_proto));

  }

  /* printf("name: %s\n", __NAME); */
  /* printf("conf: %s\n", config_file); */
  /* printf("sock: %d\n", sockfd); */

  return 0;
}
