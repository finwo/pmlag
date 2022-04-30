#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>
#include <sys/socket.h>

#include "argparse.h"
#include "ini.h"
#include "socket.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NAME
#define NAME "pmlag"
#endif

#define BROADCAST_FLOOD    1
#define BROADCAST_BALANCED 2

#define MODE_SLAVE         0
#define MODE_ACTIVE_BACKUP 1
#define MODE_BROADCAST     2
#define MODE_BALANCE_RR    3

static const char *const usage[] = {
  NAME " [options]",
  NULL
};

typedef struct {
  // Linked list
  void *next;    // Ref to the next interface
  // Config
  char *name;    // Name of the interface
  char *master;  // Name of the controlling interface
  char *mac;     // Mac address to set on the interface
  int broadcast; // Broadcast mode
  int mode;      // Balancing mode
  int weight;    // Weight of the interface
  // Activity
  int socket;    // Socket file descriptor
  int remainder; // Round robin remainder
} pmlag_interface;

typedef struct {
  pmlag_interface *interfaces;
} pmlag_configuration;

pmlag_configuration *configuration_active  = NULL;
pmlag_configuration *configuration_loading = NULL;

static int config_load_handler(
  void *user,
  const char *section,
  const char *name,
  const char *value
) {
  pmlag_configuration* config = (pmlag_configuration *) user;

  // Get interface being configured
  pmlag_interface *iface = config->interfaces;
  while(iface) {
    if (!strcmp(iface->name, section)) {
      break;
    }
    iface = iface->next;
  }

  // Create interface if not found
  if (!iface) {
    iface              = calloc(1, sizeof(pmlag_interface));
    iface->next        = config->interfaces;
    iface->name        = strdup(section);
    config->interfaces = iface;
  }


  // Set found value
  if (!strcmp("master", name)) {
    iface->master = strdup(value);
  } else if (!strcmp("weight", name)) {
    iface->weight = atoi(value);
  } else if (!strcmp("broadcast", name)) {
    if (!strcmp("flood", value)) {
      iface->broadcast = BROADCAST_FLOOD;
    } else if (!strcmp("balanced", value)) {
      iface->broadcast = BROADCAST_BALANCED;
    } else {
      fprintf(stderr, "Unknown broadcast: %s\n", value);
      return 0;
    }
  } else if (!strcmp("mode", name)) {
    if (!strcmp("slave", value)) {
      iface->mode = MODE_SLAVE;
    } else if (!strcmp("active-backup", value)) {
      iface->mode = MODE_ACTIVE_BACKUP;
    } else if (!strcmp("broadcast", value)) {
      iface->mode = MODE_BROADCAST;
    } else if (!strcmp("balance-rr", value)) {
      iface->mode = MODE_BALANCE_RR;
    } else {
      fprintf(stderr, "Unknown mode: %s\n", value);
      return 0;
    }
  } else if (!strcmp("mac", name)) {
    // TODO: parse mac address
    iface->mac = strdup(value);
  } else {
    return 0;
  }

  return 1;
}

void config_free(pmlag_configuration *config) {
  if (!config) return;
  pmlag_interface *iface_next;
  pmlag_interface *iface = config->interfaces;
  while(iface) {
    iface_next = iface->next;
    if (iface_next == config->interfaces) break;
    if (iface->mac   ) free(iface->mac);
    if (iface->master) free(iface->master);
    if (iface->name  ) free(iface->name);
    free(iface);
    iface = iface_next;
  }
  free(config);
}

pmlag_configuration * config_load(const char * filename) {
  // Load config, entry-by-entry
  pmlag_configuration *config = calloc(1, sizeof(pmlag_configuration));
  if (ini_parse(filename, config_load_handler, config) < 0) {
    fprintf(stderr, "Can not load %s\n", filename);
    return NULL;
  }
  // Loop interface list
  pmlag_interface *iface = config->interfaces;
  while (iface && iface->next) iface = iface->next;
  if (iface) iface->next = config->interfaces;
  // Return produce
  return config;
}

int main(int argc, const char **argv) {
  const char *config_file = "/etc/" NAME ".conf";

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
      "\n" NAME " is a tool for bonding network interfaces together when the "
      "hardware\non the other side of the cable(s) doesn't support it."
      "\n"
  );
  argc = argparse_parse(&argparse, argc, argv);

  // Load configuration
  pmlag_configuration *loaded = config_load(config_file);

  // VERY validation
  if (!loaded->interfaces) {
    fprintf(stderr, "No interfaces defined in config\n");
    return 1;
  }

  // TODO:
  // - start thread for every interface
  // - let thread create bond
  // - let thread create socket
  // - make thread close bond if not found in config
  // - make thread close socket if not found in config
  // - incoming on socket = send to master
  // - incoming on master = <mode> to slaves
  //   - mode_broadcast = to all
  //   - mode_active_backup = preferred based upon socket open or not?
  //   - mode_rr
  //     - move interface to back of line when remainder = 0
  //     - send packet and decrement remainder
  // - implement config replacement on USR1 in parent
  // - mutex all the things

  config_free(loaded);

  /* int sockfd = sockraw_open(INTERFACE); */

  /* // Prepare ingress buffer */
  /* int buflen; */
  /* unsigned char *buffer = (unsigned char *) malloc(65536); */
  /* memset(buffer, 0, 65536); */
  /* struct sockaddr saddr; */
  /* int saddr_len  = sizeof(saddr); */

  /* // Fetch a packet */
  /* buflen = recvfrom(sockfd, buffer, 65536, 0, &saddr, (socklen_t *)&saddr_len); */
  /* if (buflen < 0) { */
  /*   perror("recvfrom"); */
  /*   exit(EXIT_FAILURE); */
  /* } */

  /* printf("\n"); */
  /* printf("Got packet: %d bytes\n", buflen); */
  /* printf("\n"); */

  /* struct ethhdr *eth = (struct ethhdr *)(buffer); */
  /* printf("Ethernet header\n"); */
  /* printf("\t|- DST    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]); */
  /* printf("\t|- SRC    %.2X-%.2X-%.2X-%.2X-%.2X-%.2X\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]); */
  /* printf("\t|- PROTO  %d\n",eth->h_proto); */

  /* printf("\n"); */

  /* struct sockaddr_in source; */
  /* struct sockaddr_in dest; */
  /* if (eth->h_proto == 8) { */
  /*   unsigned short iphdrlen; */
  /*   struct iphdr *ip = (struct iphdr*)(buffer + sizeof(struct ethhdr)); */
  /*   memset(&source, 0, sizeof(source)); */
  /*   source.sin_addr.s_addr = ip->saddr; */
  /*   memset(&dest, 0, sizeof(dest)); */
  /*   dest.sin_addr.s_addr = ip->daddr; */

  /*   printf("IP header\n"); */
  /*   printf("\t|- Version          %d\n",(unsigned int)ip->version); */
  /*   printf("\t|- Internet hlen    %d DWORDS or %d Bytes\n",(unsigned int)ip->ihl,((unsigned int)(ip->ihl))*4); */
  /*   printf("\t|- Type Of Service  %d\n",(unsigned int)ip->tos); */
  /*   printf("\t|- Total Length     %d Bytes\n",ntohs(ip->tot_len)); */
  /*   printf("\t|- Identification   %d\n",ntohs(ip->id)); */
  /*   printf("\t|- Time To Live     %d\n",(unsigned int)ip->ttl); */
  /*   printf("\t|- Protocol         %d\n",(unsigned int)ip->protocol); */
  /*   printf("\t|- Header Checksum  %d\n",ntohs(ip->check)); */
  /*   printf("\t|- Source IP        %s\n", inet_ntoa(source.sin_addr)); */
  /*   printf("\t|- Destination IP   %s\n",inet_ntoa(dest.sin_addr)); */

  /*   printf("\n"); */
  /* } */

  return 0;
}

#ifdef __cplusplus
} // extern "C"
#endif
