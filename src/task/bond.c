#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "finwo/mindex.h"
#include "../util/config.h"
#include "../util/routing-table.h"
#include "../util/socket.h"

#include "announce.h"
#include "iface.h"

int task_bond_onpacket(struct pmlag_bond *bond, unsigned char *buffer, size_t buflen) {
  /* struct sockaddr_ll saddr_ll; */
  /* saddr_ll.sll_halen = ETH_ALEN; */

#ifdef DEBUG
  // Debug: print eth header
  printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x > %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%ld)\n",
    buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC
    buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST
    ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO
    buflen
  );
#endif // DEBUG

  // Get interface to send the packet from
  /* memcpy(saddr_ll.sll_addr, buffer, ETH_ALEN); */
  struct pmlag_iface *iface = (memcmp(buffer, "\xFF\xFF\xFF\xFF\xFF\xFF", ETH_ALEN) == 0)
    ? NULL
    : rt_find(bond->rt, &(bond->mtx_rt), buffer);
  pmlag_iface_llist *iface_entry;
  size_t send_len;

#ifdef DEBUG
  printf("  got iface? %s\n", iface ? "yes" : "no");
#endif

  // Broadcast on ALL interfaces if no rt entry OR broadcast packet
  if (!iface) {
    iface_entry = bond->interfaces;
    while(iface_entry) {
      // saddr_ll.sll_ifindex = iface_entry->data->ifidx;
      send_len = sendto(iface_entry->data->sockfd, buffer, buflen, 0, NULL, 0);
      // (const struct sockaddr*)&saddr_ll, sizeof(struct sockaddr_ll));
      if(send_len != buflen) {
        perror("sendto");
        return 1;
      }
      iface_entry = iface_entry->next;
    }
    return 0;
  }

  // Forward packet to iface as-is
  // saddr_ll.sll_ifindex = iface->ifidx;
  send_len = sendto(iface->sockfd, buffer, buflen, 0, NULL, 0);
  // (const struct sockaddr*)&saddr_ll, sizeof(struct sockaddr_ll));
  if(send_len != buflen) {
    perror("sendto");
    return 1;
  }

  return 0;
}

// Responsible for init + calling task_bond_onpacket
void * task_bond_thread(void *arg) {
  struct pmlag_bond *bond = arg;

  // Initialize routing table lock
  if (pthread_mutex_init(&(bond->mtx_rt), NULL) != 0) {
    perror("Initializing mutex for bond");
    pthread_exit(NULL);
    return NULL;
  }

  // Initialize routing table
  bond->rt = rt_init(bond);

  // Assign bond interface
  /* unsigned char *mac = iface_mac(bond->interfaces->data->name); */
  bond->sockfd = tap_alloc(bond->name, bond->hwaddr);
  /* free(mac); */
  if (bond->sockfd < 0) {
    perror("Allocating bond interface");
    pthread_exit(NULL);
    return NULL;
  };

  // Lock this bond's routing table
  pthread_mutex_lock(&(bond->mtx_rt));

  // Start thread for each interface of this bond
  pmlag_iface_llist *iface_entry = bond->interfaces;
  while(iface_entry) {
    if(pthread_create(&(iface_entry->data->tid), NULL, task_iface_thread, iface_entry->data)) {
      perror("Starting iface thread");
      pthread_exit((void*)1);
      return (void*)1;
    }
    iface_entry = iface_entry->next;
  }

  // Start an announcement thread
  if(pthread_create(&(bond->tid_announce), NULL, task_announce_thread, bond)) {
    perror("Starting announce thread");
    pthread_exit((void*)1);
    return (void*)1;
  }

  // Free this bond's routing table
  pthread_mutex_unlock(&(bond->mtx_rt));

  // Predefine things we'll re-use
  size_t buflen;
  unsigned char *buffer = (unsigned char *) malloc(RCVBUFSIZ);

  // TODO: replace by waiting for the iface to finish initializing
  sleep(1);

  // This is our life now
  while(1) {

    // Wait for a packet to arrive
    buflen = read(bond->sockfd, buffer, RCVBUFSIZ);
    if (buflen < 0) {
      perror("read(bond)");
      pthread_exit(NULL);
      return NULL;
    }

    // And let the more specific method handle the packet
    if(task_bond_onpacket(bond, buffer, buflen)) {
      pthread_exit(NULL);
      return NULL;
    }

  }

  // Wait for iface threads to finish
  iface_entry = bond->interfaces;
  while(iface_entry) {
    pthread_join(iface_entry->data->tid, NULL);
    iface_entry = iface_entry->next;
  }

  pthread_exit(NULL);
  return NULL;
}
