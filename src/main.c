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

/* #define INTERFACE "enp0s13f0u2c2" */

#define RCVBUFSIZ 65536

static const char *const usage[] = {
  __NAME " [options]",
  NULL
};

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

  printf("Thread started for iface: %s->%s(%d)\n", iface->bond->name, iface->name, iface->sockfd);

  // Wait for the bond thread to finish initializing
  pthread_mutex_lock(&(iface->bond->mtx_rt));
  pthread_mutex_unlock(&(iface->bond->mtx_rt));

  // Find bond socket iface_idx
  int send_len;
  uint16_t proto;

  while(1) {

    // Zero out buffer, to prevent pollution, & receive packet
    /* memset(buffer, 0, RCVBUFSIZ); */
    buflen = recvfrom(iface->sockfd, buffer, RCVBUFSIZ, 0, &saddr, (socklen_t *)&saddr_len);
    if (buflen < 0) {
      perror("recvfrom");
      pthread_exit(NULL);
      return NULL;
    }

    // Update routing table if our custom protocol is seen
    proto = ((uint16_t)((unsigned char)buffer[12]) << 8) + buffer[13];
    if (proto == 0x0666) {
      pthread_mutex_lock(&(iface->bond->mtx_rt));
      // TODO: update routing table
      pthread_mutex_unlock(&(iface->bond->mtx_rt));
      continue; // Don't forward the packet
    }

    printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x < %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%d)\n",
        buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST
        buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC
        ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO
        buflen
    );

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
  printf("Thread started for bond: %s\n", bond->name);

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


    // Start the bond's thread
    if(pthread_create(&(bond->tid), NULL, thread_bond, bond)) {
      perror("Starting bond thread");
      return 1;
    }

    bond = bond->next;
  }

  // Wait for all bonds to finish
  bond = config->bonds;
  while(bond) {
    pthread_join(bond->tid, NULL);
    bond = bond->next;
  }

  return 0;
}
