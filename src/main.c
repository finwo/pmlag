#include <arpa/inet.h>
#include <errno.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>

#include "cofyc/argparse.h"
#include "config.h"
#include "socket.h"

#define INTERFACE "docker0"

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

int main(int argc, const char **argv) {
  char *config_file="/etc/pmlag/pmlag.ini";

  // Define which options we support
  struct argparse_option options[] = {
    OPT_HELP(),
    OPT_STRING('c', "config", &config_file, "Config file to use", NULL, 0, 0),
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

  // Load configuration file
  struct pmlag_configuration *config = config_load(config_file);
  if (!config) {
    return 1;
  }

  printf("Loaded configuration:\n");
  // For each bond of config->bonds
  struct pmlag_bond *bond = config->bonds;
  struct pmlag_interface *iface;
  while(bond) {
    printf("\n%s:\n", bond->name);
    printf("    mode: %s\n", (
        bond->mode == PMLAG_MODE_NONE
          ? "NONE"
          : (
              bond->mode == PMLAG_MODE_BROADCAST
              ? "BROADCAST"
              : (
                  bond->mode == PMLAG_MODE_BALANCED_RR
                  ? "BROADCAST"
                  : "ACTIVE_BACKUP"
                )
            )
    ));
    iface = bond->interfaces;
    // For each iface of config->interfaces
    while(iface) {
      printf("\n    %s:\n", iface->name);
      printf("        weight: %d\n", iface->weight);
      iface = iface->next;
    }
    bond = bond->next;
  }

  /* int sockfd = sockraw_open(INTERFACE); */
  /* if (sockfd < 0) { */
  /*   return 1; */
  /* } */

  /* if (rx) { */

/*     // Prepare ingress buffer */
/*     int buflen; */
/*     unsigned char *buffer = (unsigned char *) malloc(65536); */
/*     memset(buffer, 0, 65536); */
/*     struct sockaddr saddr; */
/*     int saddr_len  = sizeof(saddr); */

/*     struct ethhdr *eth = (struct ethhdr *)(buffer); */
/*     struct sockaddr_in source; */
/*     struct sockaddr_in dest; */

/*     while(1) { */

/*       // Fetch a packet */
/*       buflen = recvfrom(sockfd, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len); */
/*       if (buflen < 0) { */
/*         perror("recvfrom"); */
/*         exit(EXIT_FAILURE); */
/*       } */

/*       printf("\n"); */
/*       printf("Got packet: %d bytes\n", buflen); */
/*       printf("\n"); */

/*       printf("Ethernet header\n"); */
/*       printf("\t|- DST    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]); */
/*       printf("\t|- SRC    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]); */
/*       printf("\t|- PROTO  %.4X\n",be16toh(eth->h_proto)); */

/*       printf("\n"); */

/*       if (be16toh(eth->h_proto) == 0x0800) { */
/*         unsigned short iphdrlen; */
/*         struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr)); */
/*         memset(&source, 0, sizeof(source)); */
/*         source.sin_addr.s_addr = ip->saddr; */
/*         memset(&dest, 0, sizeof(dest)); */
/*         dest.sin_addr.s_addr = ip->daddr; */

/*         printf("IP header\n"); */
/*         printf("\t|- Version          %d\n",(unsigned int)ip->version); */
/*         printf("\t|- Internet hlen    %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4); */
/*         printf("\t|- Type Of Service  %d\n",(unsigned int)ip->tos); */
/*         printf("\t|- Total Length     %d Bytes\n",ntohs(ip->tot_len)); */
/*         printf("\t|- Identification   %d\n",ntohs(ip->id)); */
/*         printf("\t|- Time To Live     %d\n",(unsigned int)ip->ttl); */
/*         printf("\t|- Protocol         %d\n",(unsigned int)ip->protocol); */
/*         printf("\t|- Header Checksum  %d\n",ntohs(ip->check)); */
/*         printf("\t|- Source IP        %s\n", inet_ntoa(source.sin_addr)); */
/*         printf("\t|- Destination IP   %s\n",inet_ntoa(dest.sin_addr)); */

/*         printf("\n"); */
/*       } */

/*     } */

  /* } */

  /* if (tx) { */

/*     // Prepare egress buffer */
/*     int buflen; */
/*     unsigned char *buffer = (unsigned char *) malloc(65536); */
/*     memset(buffer, 0, 65536); */
/*     struct sockaddr_ll saddr; */
/*     int saddr_len  = sizeof(saddr); */

/*     // Get MAC address of the interface */
/*     struct ifreq ifreq_c; */
/*     memset(&ifreq_c,0,sizeof(ifreq_c)); */
/*     strncpy(ifreq_c.ifr_name,INTERFACE,IFNAMSIZ-1);//giving name of Interface */
/*     if((ioctl(sockfd,SIOCGIFHWADDR,&ifreq_c))<0) { */
/*       perror("error in SIOCGIFHWADDR ioctl reading"); */
/*       return 1; */
/*     } */

/*     // Get ethhdr reference to the buffer */
/*     struct ethhdr *eth = (struct ethhdr *)(buffer); */

/*     // Fill ethernet source address */
/*     memcpy(eth->h_source, ifreq_c.ifr_hwaddr.sa_data, 6); */
/*     /1* eth->h_source[0] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[0]); *1/ */
/*     /1* eth->h_source[1] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[1]); *1/ */
/*     /1* eth->h_source[2] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[2]); *1/ */
/*     /1* eth->h_source[3] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[3]); *1/ */
/*     /1* eth->h_source[4] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[4]); *1/ */
/*     /1* eth->h_source[5] = (unsigned char)(ifreq_c.ifr_hwaddr.sa_data[5]); *1/ */

/*     // Broadcast packet */
/*     eth->h_dest[0] = 0xFF; */
/*     eth->h_dest[1] = 0xFF; */
/*     eth->h_dest[2] = 0xFF; */
/*     eth->h_dest[3] = 0xFF; */
/*     eth->h_dest[4] = 0xFF; */
/*     eth->h_dest[5] = 0xFF; */

/*     // Custom protocol (not shown as registered on https://www.iana.org/assignments/ieee-802-numbers/ieee-802-numbers.xhtml) */
/*     eth->h_proto = htons(0x0666); */

/*     struct sockaddr_ll sadr_ll; */
/*     sadr_ll.sll_ifindex = iface_idx(sockfd, INTERFACE); */
/*     sadr_ll.sll_halen   = ETH_ALEN; // length of destination mac address */
/*     memcpy(sadr_ll.sll_addr, eth->h_dest, ETH_ALEN); */

/*     /1* sadr_ll.sll_addr[0] = 0xFF; *1/ */
/*     /1* sadr_ll.sll_addr[1] = 0xFF; *1/ */
/*     /1* sadr_ll.sll_addr[2] = 0xFF; *1/ */
/*     /1* sadr_ll.sll_addr[3] = 0xFF; *1/ */
/*     /1* sadr_ll.sll_addr[4] = 0xFF; *1/ */
/*     /1* sadr_ll.sll_addr[5] = 0xFF; *1/ */

/*     /1* saddr.sll_ifindex = ifreq_i.ifr_ifindex; // index of interface *1/ */
/*     /1* saddr.sll_halen = ETH_ALEN; // length of destination mac address *1/ */
/*     /1* saddr.sll_addr[0] = DESTMAC0; *1/ */
/*     /1* saddr.sll_addr[1] = DESTMAC1; *1/ */
/*     /1* saddr.sll_addr[2] = DESTMAC2; *1/ */
/*     /1* saddr.sll_addr[3] = DESTMAC3; *1/ */
/*     /1* saddr.sll_addr[4] = DESTMAC4; *1/ */
/*     /1* saddr.sll_addr[5] = DESTMAC5; *1/ */


/*     int send_len = sendto(sockfd,buffer,60,0,(const struct sockaddr*)&sadr_ll,sizeof(struct sockaddr_ll)); */
/*     if(send_len<0) { */
/*       printf("error in sending....sendlen=%d....errno=%d\n",send_len,errno); */
/*       return -1; */
/*     } */

  /* } */

  /* printf("name: %s\n", __NAME); */
  /* printf("conf: %s\n", config_file); */
  /* printf("sock: %d\n", sockfd); */
  return 42;
}
