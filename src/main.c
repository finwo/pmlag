#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>

#include "cofyc/argparse.h"

#include "util/config.h"
#include "util/routing-table.h"
#include "util/socket.h"

#define RCVBUFSIZ 131072

#define TICK            100
#define PMLAG_RT_FAST    64

#define PMLAG_COMMAND_STREE_BROADCAST 0

unsigned char *rcvbuf = NULL;
static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

int64_t millis() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * ((int64_t)1000)) + (tv.tv_usec / 1000);
}

void handle_packet_bond(struct pmlag_bond *bond) {
  int i, buflen;
  buflen = read(bond->sockfd, rcvbuf, RCVBUFSIZ);

  // Get interface to send the packet from
  struct pmlag_iface *iface = (memcmp(rcvbuf, "\xFF\xFF\xFF\xFF\xFF\xFF", ETH_ALEN) == 0)
    ? NULL
    : rt_find(bond->rt, rcvbuf);

  if (iface) {
    printf(
      "Sending to iface %s for %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
      iface->name,
      rcvbuf[0],
      rcvbuf[1],
      rcvbuf[2],
      rcvbuf[3],
      rcvbuf[4],
      rcvbuf[5]
    );
  } else {
    printf(
      "No iface for %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
      rcvbuf[0],
      rcvbuf[1],
      rcvbuf[2],
      rcvbuf[3],
      rcvbuf[4],
      rcvbuf[5]
    );
  }

  // Broadcast on ALL interfaces if no rt entry OR broadcast packet
  if (!iface) {
    for( i = 0 ; i < bond->iface_count ; i++ ) {
      if (sendto(bond->iface[i]->sockfd, rcvbuf, buflen, 0, NULL, 0) != buflen) {
        perror("sendto");
        printf("Error during sendto on %d\n", __LINE__);
      }
    }
    return;
  }

  // Forward packet to iface as-is
  if (sendto(iface->sockfd, rcvbuf, buflen, 0, NULL, 0) != buflen) {
    perror("sendto");
    printf("Error during sendto on %d\n", __LINE__);
  }
}

void handle_packet_iface(struct pmlag_iface *iface) {
  printf("Packet on iface %s\n", iface->name);
  struct pmlag_bond *bond = iface->bond;
  struct pmlag_rt_entry *rt_entry;
  int send_len, buflen, idx;
  uint16_t ethtype;
  uint16_t command;
  uint16_t bcid;

  // Read the packet
  buflen = recvfrom(iface->sockfd, rcvbuf, RCVBUFSIZ, 0, NULL, 0);
  if (buflen < 0) {
    perror("recvfrom");
    return;
  }

  // Basic error checking
  // ethernet says minimum of 60, we just need 18
  if (buflen < 18) return;

  // Always add mac to routing table
  rt_upsert(
    bond->rt,
    iface,
    rcvbuf + ETH_ALEN,
    bond->bc_id
  );

  // Get the ethtype
  memcpy(&ethtype, rcvbuf + (2*ETH_ALEN), sizeof(ethtype));
  ethtype = ntohs(ethtype);

  printf("Read %d bytes (%d)  --  rt = %ld\n", buflen, ethtype, bond->rt->length);

  // Don't touch packets that don't use our described protocol
  if (ethtype != 0x0666) {
    send_len = write(iface->bond->sockfd, rcvbuf, buflen);
    if (send_len != buflen) {
      perror("write");
      return;
    }
  }

  // Ignore our own packets (loop detected, may do something with it later)
  if (!memcmp(rcvbuf + ETH_ALEN, iface->bond->hwaddr, ETH_ALEN)) {
    printf("Loop detected\n");
    return;
  }

  // Fetch the command that was called
  memcpy(&command, rcvbuf + (2*ETH_ALEN) + sizeof(ethtype), sizeof(command));
  command = ntohs(command);

  switch(command) {
    case PMLAG_COMMAND_STREE_BROADCAST:

      // Stop if we already triggered this round
      if (bond->state & PMLAG_STATE_TRIGGERED) break;
      bond->state |= PMLAG_STATE_TRIGGERED;

      // Have the network fight for leader status
      if (bond->bc_timer) {
        // Demote to follower if we were waiting
        bond->bc_timer = 3;
      } else {
        // Random between now leader and pensive
        bond->bc_timer = (rand() % 2) * 2;
      }

      // Fetch the broadcast id
      memcpy(&bcid, rcvbuf + (2*ETH_ALEN) + sizeof(ethtype) + sizeof(command), sizeof(bcid));
      bcid = ntohs(bcid);
      bond->bc_id = bcid;

      /* // Remove old entries from routing table */
      /* for( idx = 0; idx < bond->rt->length ; idx++ ) { */
      /*   rt_entry = bond->rt->items[idx]; */
      /*   if ((bcid - rt_entry->bcidx) > PMLAG_RT_FAST) { */
      /*     mindex_delete(bond->rt, rt_entry); // rt_entry is now unsafe */
      /*     continue; */
      /*   } */
      /*   // More things here? */
      /* } */

      break;
    default:
      // Unknown command
      return;
  }
}

void handle_packet(void *entity) {
  struct pmlag_iface *iface = entity;
  struct pmlag_bond  *bond  = entity;
  if (iface->type == PMLAG_ENTITY_TYPE_IFACE) {
    return handle_packet_iface(iface);
  }
  if (bond->type == PMLAG_ENTITY_TYPE_BOND) {
    return handle_packet_bond(bond);
  }
}

int main(int argc, const char **argv) {
  char *config_file="/etc/pmlag/pmlag.ini";
  int epfd;
  struct pmlag_bond *bond;
  struct pmlag_iface *iface;

  // Seed random
  uint32_t seed;
  FILE* urandom = fopen("/dev/urandom", "r");
  fread(&seed, sizeof(uint32_t), 1, urandom);
  fclose(urandom);
  srand(seed);

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
  struct pmlag_configuration *config = config_load(config_file, NULL);
  if (!config) {
    return 1;
  }

  // Initialize packet buffer
  rcvbuf = (unsigned char *)malloc(RCVBUFSIZ);

  // Initialize epoll
  epfd = epoll_create1(0);

  // DEBUG: Display bonds
  int b = 0;
  int i = 0;
  for( b = 0 ; b < config->bond_count ; b++ ) {
    printf("%s\n", config->bond[b]->name);
    for( i = 0 ; i < config->bond[b]->iface_count ; i++ ) {
      printf("  - %s\n", config->bond[b]->iface[i]->name);
    }
  }

  // Prepare announce buffer
  uint16_t ethtype  = htons(0x0666);
  size_t anc_buflen = (ETH_ALEN*2) + sizeof(ethtype) + sizeof(uint16_t) + sizeof(uint16_t); // mac + ethertype + command + broadcast-index
  char *anc_buffer  = malloc(anc_buflen);

  // Wait for things to happen & call tick methods periodically
  int ev_count;
  struct epoll_event events[8];
  int64_t ttime = millis();
  int64_t tdiff = 0;
  while(1) {

    // Calculate waiting time & periodic actions
    tdiff = ttime - millis();
    if (tdiff <= 0) {
      ttime += TICK;
      tdiff += TICK;

      // Handle sockets
      for( b = 0 ; b < config->bond_count ; b++ ) {

        // (re)open bond tap
        bond = config->bond[b];
        if (!bond->sockfd) {
          bond->sockfd   = tap_alloc(bond->name, bond->hwaddr);
          bond->state    = 0;
          bond->bc_timer = 3; // set to 2 in combination with the decrement below
          if (bond->sockfd < 0) {
            perror("tap_alloc");
            bond->sockfd = 0;
            continue;
          }
          bond->epev = calloc(1, sizeof(struct epoll_event));
          if (!bond->epev) {
            perror("calloc");
            bond->epev = NULL;
            continue;
          }
          bond->epev->events   = EPOLLIN;
          bond->epev->data.ptr = bond;
          if (epoll_ctl(epfd, EPOLL_CTL_ADD, bond->sockfd, bond->epev)) {
            free(bond->epev);
            close(bond->sockfd);
            bond->sockfd = 0;
            bond->epev   = NULL;
            continue;
          }
        }

        // (re)initialize routing table
        if (!bond->rt) {
          bond->rt = rt_init(bond);
        }

        // Fetch bond's mac address if missing (from random)
        if (!bond->hwaddr) {
          bond->hwaddr = iface_mac(bond->name);
        }

        // Mark the bond as untriggered, decrement timer and increment bc_id
        bond->state &= ~PMLAG_STATE_TRIGGERED;
        bond->bc_timer--;

        for( i = 0 ; i < bond->iface_count ; i++ ) {
          iface = bond->iface[i];

          // (re)open interface socket
          if (!iface->sockfd) {
            iface->sockfd = sockraw_open(iface->name);
            if (iface->sockfd < 0) {
              perror("sockraw_open");
              iface->sockfd = 0;
              continue;
            }
            iface->ifidx = iface_idx(iface->sockfd, iface->name);
            /* printf("Opened socket for %s\n", iface->name); */
          }

          // Register it with epoll
          if (!iface->epev) {
            iface->epev = calloc(1, sizeof(struct epoll_event));
            if (!iface->epev) {
              perror("calloc");
              iface->epev = NULL;
              continue;
            }
            iface->epev->events   = EPOLLIN;
            iface->epev->data.ptr = iface;
            if (epoll_ctl(epfd, EPOLL_CTL_ADD, iface->sockfd, iface->epev)) {
              free(iface->epev);
              close(iface->sockfd);
              iface->sockfd = 0;
              iface->epev   = NULL;
              continue;
            }
            /* printf("Registered %s with epoll\n", iface->name); */
          }
        }

        // Handle broadcasts
        if (bond->bc_timer < 0) {
          bond->bc_timer = 0;

          // Increment broadcast id & prepare network orientation
          bond->bc_id++;
          bond->bc_id = htons(bond->bc_id);

          // Prepare packet
          memset(anc_buffer                                                    , 0xFF          , ETH_ALEN);            // Send to broadcast
          memcpy(anc_buffer + (1*ETH_ALEN)                                     , bond->hwaddr  , ETH_ALEN);            // From bond
          memcpy(anc_buffer + (2*ETH_ALEN)                                     , &ethtype      , sizeof(ethtype));     // ethtype 0x0666
          memset(anc_buffer + (2*ETH_ALEN) + sizeof(ethtype)                   , 0x00          , sizeof(uint16_t));    // command (interface detection broadcast)
          memcpy(anc_buffer + (2*ETH_ALEN) + sizeof(ethtype) + sizeof(uint16_t), &(bond->bc_id), sizeof(bond->bc_id)); // broadcast id

          // Revert bc_id to host-orientation
          bond->bc_id = ntohs(bond->bc_id);

          // Send packet on all interfaces
          for( i=0; i < bond->iface_count ; i++ ) {
            iface = bond->iface[i];
            if (sendto(iface->sockfd, anc_buffer, anc_buflen, 0, NULL, 0) != anc_buflen) {
              perror("sendto");
              printf("Error during sendto on %d\n", __LINE__);
            }
          }

        }
      }
    }

    // Handle incoming data
    if (tdiff < 0) tdiff = 0;
    ev_count = epoll_wait(epfd, events, 8, tdiff);
    for( i=0 ; i<ev_count; i++) {
      handle_packet(events[i].data.ptr);
    }

  }

  return 0;
}
