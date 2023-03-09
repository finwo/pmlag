#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../util/config.h"
#include "../util/socket.h"
#include "announce.h"

void * task_announce_thread(void *arg) {
  struct pmlag_bond *bond = arg;
  struct sockaddr_ll saddr_ll;
  saddr_ll.sll_halen = ETH_ALEN;
  pmlag_iface_llist *iface_entry;

  uint16_t ethtype = htons(0x0666);
  size_t buflen = (ETH_ALEN*2) + sizeof(ethtype) + 46;
  size_t sendlen;

  unsigned char *mac = iface_mac(bond->name);
  unsigned char *buffer = calloc(1, buflen);

  // Prepare saddr_ll and buffer
  memcpy(saddr_ll.sll_addr, mac, ETH_ALEN);
  memset(buffer, 0xFF, ETH_ALEN);                          // Destination = Broadcast
  memcpy(buffer+ETH_ALEN, mac, ETH_ALEN);                  // Source      = bond
  memcpy(buffer+(ETH_ALEN*2), &ethtype, sizeof(uint16_t)); // EtherType   = 0x0666 = custom

  int16_t bcidx = htons(rand() | 1);

  while(1) {
    sleep(1);
    iface_entry = bond->interfaces;

    // Increment bcidx
    bcidx = ntohs(bcidx);
    bcidx++;
    if (!bcidx) bcidx = 1;
    bcidx = htons(bcidx);

    // Store bcidx on buffer
    memcpy(buffer + (ETH_ALEN*2) + sizeof(uint16_t), &bcidx, sizeof(int16_t));

    // Send the buffer as-is over all ifaces
    while(iface_entry) {
      if (iface_entry->data->sockfd) {
        saddr_ll.sll_ifindex = iface_entry->data->ifidx;
        sendlen = sendto(iface_entry->data->sockfd, buffer, buflen, 0, (const struct sockaddr*)&saddr_ll, sizeof(struct sockaddr_ll));
        if (sendlen < 0) {
          perror("SENDTO");
        }
      }
      iface_entry = iface_entry->next;
    }
  }

  return NULL;
}
