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

#include "tidwall/btree.h"
#include "cofyc/argparse.h"
#include "util/config.h"

#include "task/bond.h"
/* #include "socket.h" */

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

/* void trim_rt(struct pmlag_bond *bond, size_t limit) { */

/* } */

/* void iface_add_rt(struct pmlag_iface *iface, unsigned char *mac, uint16_t bcidx) { */
/*   struct pmlag_rt_entry *rt_entry; */
/*   int iface_list_len; */
/*   struct pmlag_iface *iface_list_entry; */

/*   pthread_mutex_lock(&(iface->bond->mtx_rt)); */
/*   printf("\nRCV bc: %s\n", iface->name); */

/*   // Fetch or build rt entry */
/*   rt_entry = btree_get(iface->bond->rt, &(struct pmlag_rt_entry){ .mac = mac }); */
/*   if (!rt_entry) { */
/*     /1* printf("  new mac\n"); *1/ */
/*     // Build new entry */
/*     rt_entry = calloc(1, sizeof(struct pmlag_rt_entry)); */
/*     /1* bzero(rt_entry, sizeof(struct pmlag_rt_entry)); *1/ */
/*     /1* rt_entry->bcidx = 0; *1/ */

/*     // Insert the mac address */
/*     rt_entry->mac = malloc(ETH_ALEN); */
/*     memcpy(rt_entry->mac, mac, ETH_ALEN); */

/*     /1* // And an empty list of interfaces *1/ */
/*     /1* rt_entry->interfaces = NULL; *1/ */
/*   } else { */
/*     /1* printf("  known mac\n"); *1/ */
/*   } */

/*   // Bail if we receive a regular packet on an interface with bcidx */
/*   if (!bcidx && rt_entry->bcidx) { */
/*     /1* printf("  bail, regular packet on pmlag remote\n"); *1/ */
/*     pthread_mutex_unlock(&(iface->bond->mtx_rt)); */
/*     return; */
/*   } */

/*   // Clear list of known interfaces if */
/*   pmlag_iface_llist *iface_entry; */
/*   if ( */
/*     (bcidx && (rt_entry->bcidx != bcidx)) || // We got a NEW broadcast index */
/*     (!bcidx && (rt_entry->bcidx == 0))       // Or we're updating a non-pmlag remote */
/*   ) { */
/*     // Free list 1-by-1 */
/*     while(rt_entry->interfaces) { */
/*       printf("FREE LINE %d (%p)\n", __LINE__, iface_entry); */
/*       printf("FREE LINE %d (%p)\n", __LINE__, rt_entry->interfaces); */
/*       iface_entry = rt_entry->interfaces; */
/*       printf("FREE LINE %d (%p)\n", __LINE__, iface_entry); */
/*       printf("FREE LINE %d (%p)\n", __LINE__, iface_entry->next); */
/*       rt_entry->interfaces = iface_entry->next; */
/*       printf("FREE LINE %d (%p)\n", __LINE__, iface_entry); */
/*       free(iface_entry); */
/*     } */
/*     rt_entry->bcidx = bcidx; */
/*     /1* printf("  clear interface list\n"); *1/ */
/*   } else { */
/*     // Don't track the interface */
/*     /1* printf("  keep interface list\n"); *1/ */
/*   } */

/*   // Add given interface to the entry's interface list */
/*   iface_entry = calloc(1, sizeof(pmlag_iface_llist)); */
/*   iface_entry->next = rt_entry->interfaces; */
/*   iface_entry->data = iface; */
/*   rt_entry->interfaces = iface_entry; */

/*   /1* iface_list_len = 0; *1/ */
/*   /1* iface_list_entry = rt_entry->interfaces[iface_list_len]; *1/ */
/*   /1* while(iface_list_entry) { *1/ */
/*   /1*   iface_list_entry = rt_entry->interfaces[++iface_list_len]; *1/ */
/*   /1* } *1/ */

/*   /1* /2* printf("  len before: %d\n", iface_list_len); *2/ *1/ */
/*   /1* rt_entry->interfaces = realloc(rt_entry->interfaces, (iface_list_len+2) * sizeof(struct pm_rt_entry *)); *1/ */
/*   /1* /2* printf("  reallocated\n"); *2/ *1/ */
/*   /1* rt_entry->interfaces[iface_list_len  ] = iface; *1/ */
/*   /1* rt_entry->interfaces[iface_list_len+1] = NULL; *1/ */
/*   /1* /2* printf("  len after: %d\n", iface_list_len+1); *2/ *1/ */

/*   // Save rt entry in the routing table again */
/*   btree_set(iface->bond->rt, rt_entry); */
/*   /1* printf("  saved to tree\n"); *1/ */
/*   /1* printf("\n"); *1/ */

/*   pthread_mutex_unlock(&(iface->bond->mtx_rt)); */
/* } */



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
  /*     memcpy(buffer + (ETH_ALEN*2) + sizeof(uint16_t), &(bond->bcidx), sizeof(uint16_t)); */

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
