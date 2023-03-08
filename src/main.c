// vim: fdm=marker

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
#include "util/config.h"

#include "task/bond.h"

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

  // Initialize interfaces for all configured bonds
  struct pmlag_bond *bond = config->bonds;
  while(bond) {

    // Start the bond's thread
    if(pthread_create(&(bond->tid), NULL, task_bond_thread, bond)) {
      perror("Starting bond thread");
      return 1;
    }

    bond = bond->next;
  }

  /* // Timer, keep broadcasting "I'm here" packets */
  /* struct pmlag_iface *iface; */
  /* struct sockaddr_ll saddr_ll; */
  /* saddr_ll.sll_halen = ETH_ALEN; */
  /* unsigned char *mac; */
  /* uint16_t ethtype = htons(0x0666); */
  /* int buflen = (ETH_ALEN*2) + 2 + 46; */
  /* unsigned char *buffer = calloc(1, buflen); */
  /* int send_len; */
  /* memset(buffer, 0xFF, ETH_ALEN);                          // DST = broadcast */
  /* memcpy(buffer+(ETH_ALEN*2), &ethtype, sizeof(uint16_t)); // EtherType = 0x0666 = custom */
  /* while(1) { */
  /*   bond = config->bonds; */
  /*   while(bond) { */
  /*     // Set source address in saddr_ll and packet */
  /*     mac = iface_mac(bond->name); */
  /*     memcpy(saddr_ll.sll_addr, mac, ETH_ALEN); */
  /*     memcpy(buffer+ETH_ALEN, mac, ETH_ALEN); */
  /*     printf("FREE LINE %d (%p)\n", __LINE__, mac); */
  /*     free(mac); */
  /*     // Set the bcidx */
  /*     memcpy(buffer + (ETH_ALEN*2) + sizeof(uint16_t), &(bond->bcidx), sizeof(int16_t)); */

  /*     /1* printf("Ethernet header\n"); *1/ */
  /*     /1* printf("\nSending: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x > %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%d)\n\n", *1/ */
  /*     /1*     buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC *1/ */
  /*     /1*     buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST *1/ */
  /*     /1*     ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO *1/ */
  /*     /1*     buflen *1/ */
  /*     /1* ); *1/ */

  /*     // Output to all interfaces */
  /*     iface = bond->interfaces; */
  /*     while(iface) { */
  /*       if (iface->sockfd) { */
  /*         saddr_ll.sll_ifindex = iface->ifidx; */
  /*         send_len = sendto(iface->sockfd, buffer, buflen, 0, (const struct sockaddr*)&saddr_ll, sizeof(struct sockaddr_ll)); */
  /*         if (send_len < 0) { */
  /*           perror("SENDTO"); */
  /*         } */
  /*       } */
  /*       iface = iface->next; */
  /*     } */
  /*     bond->bcidx = htons((ntohs(bond->bcidx)+1)|1); */
  /*     bond = bond->next; */
  /*   } */
  /*   /1* usleep(100000); // 100ms *1/ */
  /*   sleep(1); */
  /* } */


  // Wait for all bonds to finish
  bond = config->bonds;
  while(bond) {
    pthread_join(bond->tid, NULL);
    bond = bond->next;
  }

  return 0;
}
