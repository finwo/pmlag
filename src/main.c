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
#include "config.h"
#include "socket.h"

#define RCVBUFSIZ 65536

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

void iface_add_rt(struct pmlag_iface *iface, unsigned char *mac, uint16_t bcidx) {
  struct pmlag_rt_entry *rt_entry;
  int iface_list_len;
  struct pmlag_iface *iface_list_entry;

  pthread_mutex_lock(&(iface->bond->mtx_rt));
  /* printf("\nRCV bc: %s\n", iface->name); */

  // Fetch or build rt entry
  rt_entry = btree_get(iface->bond->rt, &(struct pmlag_rt_entry){ .mac = mac });
  if (!rt_entry) {
    /* printf("  new mac\n"); */
    // Build new entry
    rt_entry = malloc(sizeof(struct pmlag_rt_entry));
    rt_entry->bcidx = 0;

    // Insert the mac address
    rt_entry->mac = malloc(ETH_ALEN);
    memcpy(rt_entry->mac, mac, ETH_ALEN);

    // And an empty list of interfaces
    rt_entry->interfaces = calloc(1, sizeof(struct pm_rt_entry *));
  } else {
    /* printf("  known mac\n"); */
  }

  // Bail if we receive a regular packet on an interface with bcidx
  if (!bcidx && rt_entry->bcidx) {
    /* printf("  bail, regular packet on pmlag remote\n"); */
    pthread_mutex_unlock(&(iface->bond->mtx_rt));
    return;
  }

  // Clear list of known interfaces if
  if (
    (bcidx && (rt_entry->bcidx != bcidx)) || // We got a NEW broadcast index
    (!bcidx && (rt_entry->bcidx == 0))       // Or we're updating a non-pmlag remote
  ) {
    free(rt_entry->interfaces);
    rt_entry->interfaces = calloc(1, sizeof(struct pm_rt_entry *));
    rt_entry->bcidx = bcidx;
    /* printf("  clear interface list\n"); */
  } else {
    // Don't track the interface
    /* printf("  keep interface list\n"); */
  }

  // Add given interface to the entry's interface list
  iface_list_len = 0;
  iface_list_entry = rt_entry->interfaces[iface_list_len];
  while(iface_list_entry) {
    iface_list_entry = rt_entry->interfaces[++iface_list_len];
  }
  /* printf("  len before: %d\n", iface_list_len); */
  rt_entry->interfaces = realloc(rt_entry->interfaces, (iface_list_len+2) * sizeof(struct pm_rt_entry *));
  /* printf("  reallocated\n"); */
  rt_entry->interfaces[iface_list_len  ] = iface;
  rt_entry->interfaces[iface_list_len+1] = NULL;
  /* printf("  len after: %d\n", iface_list_len+1); */

  // Save rt entry in the routing table again
  btree_set(iface->bond->rt, rt_entry);
  /* printf("  saved to tree\n"); */
  /* printf("\n"); */

  pthread_mutex_unlock(&(iface->bond->mtx_rt));
}

void * thread_iface(void *arg) {
  struct pmlag_iface *iface = (struct pmlag_iface *)arg;

  // Open socket for the interface in the bond
  iface->sockfd = sockraw_open(iface->name);
  if (iface->sockfd < 0) {
    pthread_exit(NULL);
    return NULL;
  }

  // Reserve receive buffer, support 64k packets just in case
  int buflen;
  unsigned char *buffer = (unsigned char *) malloc(RCVBUFSIZ);
  struct sockaddr saddr;
  int saddr_len  = sizeof(saddr);

  // Get the interface's idx on the socket
  iface->ifidx = iface_idx(iface->sockfd, iface->name);

  /* printf("Thread started for iface: %s->%s(%d)\n", iface->bond->name, iface->name, iface->sockfd); */

  // Wait for the bond thread to finish initializing
  pthread_mutex_lock(&(iface->bond->mtx_rt));
  pthread_mutex_unlock(&(iface->bond->mtx_rt));

  // Find bond socket iface_idx
  int send_len;
  uint16_t proto;
  uint16_t bcidx;

  while(1) {

    // Zero out buffer, to prevent pollution, & receive packet {{{
    /* memset(buffer, 0, RCVBUFSIZ); */
    buflen = recvfrom(iface->sockfd, buffer, RCVBUFSIZ, 0, &saddr, (socklen_t *)&saddr_len);
    if (buflen < 0) {
      perror("recvfrom");
      pthread_exit(NULL);
      return NULL;
    }
    // }}}

    printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x < %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%d)\n",
        buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST
        buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC
        ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO
        buflen
    );

    // Update routing table if our custom protocol is seen
    proto = ((uint16_t)((unsigned char)buffer[(ETH_ALEN*2)+0]) << 8) + buffer[(ETH_ALEN*2)+1];
    if (proto == 0x0666) {
      bcidx = ((uint16_t)((unsigned char)buffer[(ETH_ALEN*2)+2]) << 8) + buffer[(ETH_ALEN*2)+3];
      iface_add_rt(iface, buffer+ETH_ALEN, bcidx);
      continue;
    } else {
      iface_add_rt(iface, buffer+ETH_ALEN, 0);
    }

    // Redirect packet to bond socket as-is
    send_len = write(iface->bond->sockfd, buffer, buflen);
    if (buflen != send_len) {
      perror("write(bond)");
      /* pthread_exit(NULL); */
      /* return NULL; */
    }

  }

  pthread_exit(NULL);
  return NULL;
}

void * thread_bond(void *arg) {
  struct pmlag_bond *bond = (struct pmlag_bond *)arg;
  /* printf("Thread started for bond: %s\n", bond->name); */

  // Assign bond interface
  unsigned char *mac = iface_mac(bond->interfaces->name);
  bond->sockfd = tap_alloc(bond->name, mac);
  free(mac);
  if (bond->sockfd < 0) {
    perror("Allocating bond interface");
    pthread_exit(NULL);
    return NULL;
  };

  // Lock this bond's routing table
  pthread_mutex_lock(&(bond->mtx_rt));

  // Start thread for each interface of this bond
  struct pmlag_iface *iface = bond->interfaces;
  while(iface) {
    if(pthread_create(&(iface->tid), NULL, thread_iface, iface)) {
      perror("Starting iface thread");
      pthread_exit((void*)1);
      return (void*)1;
    }
    iface = iface->next;
  }

  // Give iface threads time to run into the lock
  /* sleep(1); */

  // Free this bond's routing table
  pthread_mutex_unlock(&(bond->mtx_rt));

  int buflen, send_len;
  unsigned char *buffer = (unsigned char *) malloc(RCVBUFSIZ);
  struct sockaddr_ll saddr_ll;
  saddr_ll.sll_halen = ETH_ALEN;

  sleep(1);

  while(1) {
    /* sleep(1); */

    buflen = read(bond->sockfd, buffer, RCVBUFSIZ);
    if (buflen < 0) {
      perror("read(bond)");
      pthread_exit(NULL);
      return NULL;
    }

    /* printf("\n"); */
    /* printf("Got packet: %d bytes\n", buflen); */

    /* printf("Ethernet header\n"); */
    printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x > %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%d)\n",
        buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC
        buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST
        ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO
        buflen
    );

    // Select interface from routing table
    // DEBUG: only the first iface
    iface = bond->interfaces;

    // Prepare saddr_ll for sendto
    saddr_ll.sll_ifindex = iface->ifidx;
    memcpy(saddr_ll.sll_addr, buffer, ETH_ALEN);

    // Forward packet to iface as-is
    send_len = sendto(iface->sockfd, buffer, buflen, 0, (const struct sockaddr*)&saddr_ll, sizeof(struct sockaddr_ll));
    if(send_len != buflen) {
      perror("sendto");
      /* pthread_exit(NULL); */
      /* return NULL; */
    }

  }

  // TODO: send through routing table to other ifaces
  // TODO: send broadcasts to all interfaces
  // TODO: timer to broadcast announce our presence to ifaces (vrrp-ish)
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

static int compare_rt_entries(const void *a, const void *b, void *udata) {
  struct pmlag_rt_entry *ta = (struct pmlag_rt_entry*)a;
  struct pmlag_rt_entry *tb = (struct pmlag_rt_entry*)b;
  return memcmp(ta->mac, tb->mac, ETH_ALEN);
}

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

    // Initialize routing table lock
    if (pthread_mutex_init(&(bond->mtx_rt), NULL) != 0) {
      perror("Initializing mutex for bond");
      return 1;
    }

    // Initialize routing table
    bond->rt = btree_new(sizeof(void*), 0, compare_rt_entries, bond);

    // Start the bond's thread
    if(pthread_create(&(bond->tid), NULL, thread_bond, bond)) {
      perror("Starting bond thread");
      return 1;
    }

    bond = bond->next;
  }

  // Timer, keep broadcasting "I'm here" packets
  struct pmlag_iface *iface;
  struct sockaddr_ll saddr_ll;
  saddr_ll.sll_halen = ETH_ALEN;
  unsigned char *mac;
  uint16_t ethtype = htons(0x0666);
  int buflen = (ETH_ALEN*2) + 2 + 46;
  unsigned char *buffer = calloc(1, buflen);
  int send_len;
  memset(buffer, 0xFF, ETH_ALEN);                          // DST = broadcast
  memcpy(buffer+(ETH_ALEN*2), &ethtype, sizeof(uint16_t)); // EtherType = 0x0666 = custom
  while(1) {
    bond = config->bonds;
    while(bond) {
      // Set source address in saddr_ll and packet
      mac = iface_mac(bond->name);
      memcpy(saddr_ll.sll_addr, mac, ETH_ALEN);
      memcpy(buffer+ETH_ALEN, mac, ETH_ALEN);
      free(mac);
      // Set the bcidx
      memcpy(buffer + (ETH_ALEN*2) + sizeof(uint16_t), &(bond->bcidx), sizeof(uint16_t));

      /* printf("Ethernet header\n"); */
      /* printf("\nSending: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x > %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%d)\n\n", */
      /*     buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC */
      /*     buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST */
      /*     ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO */
      /*     buflen */
      /* ); */

      // Output to all interfaces
      iface = bond->interfaces;
      while(iface) {
        if (iface->sockfd) {
          saddr_ll.sll_ifindex = iface->ifidx;
          send_len = sendto(iface->sockfd, buffer, buflen, 0, (const struct sockaddr*)&saddr_ll, sizeof(struct sockaddr_ll));
          if (send_len < 0) {
            perror("SENDTO");
          }
        }
        iface = iface->next;
      }
      bond->bcidx = htons((ntohs(bond->bcidx)+1)|1);
      bond = bond->next;
    }
    /* usleep(100000); // 100ms */
    sleep(1);
  }


  // Wait for all bonds to finish
  bond = config->bonds;
  while(bond) {
    pthread_join(bond->tid, NULL);
    bond = bond->next;
  }

  return 0;
}
