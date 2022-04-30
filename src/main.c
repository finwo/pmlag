#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>
#include <sys/socket.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "argparse.h"
#include "config.h"
#include "main.h"
#include "socket.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NAME
#define NAME "pmlag"
#endif

static const char *const usage[] = {
  NAME " [options]",
  NULL
};

typedef struct {
  void *next;
  pthread_t tid;
  pmlag_interface *iface;
  sem_t *sem_int;
  int sig;
} pmlag_thread_ll;

pmlag_configuration *config         = NULL;
pmlag_configuration *config_pending = NULL;
pmlag_thread_ll     *threads        = NULL;

int counter = 0;
pthread_mutex_t counter_lock;
void * iface_thread(void *arg) {
  pmlag_thread_ll *thread = (pmlag_thread_ll *)arg;


  printf("Thread started for %s\n", thread->iface->name);

  while(1) {
    usleep(100000); // 100ms

    // Handle sigint
    if (thread->sig & SIGINT) {
      printf("Thread for %s received SIGINT\n", thread->iface->name);
      sem_post(thread->sem_int);
      return NULL;
    }

  }
  /* unsigned long i = 0; */
  /* int local_id; */

  /* pthread_mutex_lock(&counter_lock); */
  /* counter+=1; */
  /* local_id = counter; */
  /* pthread_mutex_unlock(&counter_lock); */

  /* printf("Job started: %d\n", local_id); */
  /* for(i=0; i<(0xFFFFFFF);i++); */
  /* printf("  Job %d finished\n", local_id); */

  return NULL;
}


void update_workers() {

  // Only act on config updates
  if (!config_pending) return;

  // First shutdown all active threads
  pmlag_thread_ll *thread = threads;
  pmlag_thread_ll *oldthread = NULL;
  while(thread) {
    thread->sig = SIGINT;         // Signal the thread to interrupt
    sem_wait(thread->sem_int);    // Wait for the thread to shut down
    sem_destroy(thread->sem_int); // Desroy the semaphore
    free(thread->sem_int);
    oldthread = thread;           // Free thread & move on the the next
    thread = thread->next;
    free(oldthread);
  }
  threads = NULL;

  // For each interface, start a new thread
  pmlag_interface *iface = config_pending->interfaces;
  while(iface) {
    thread          = calloc(1, sizeof(pmlag_thread_ll));
    thread->next    = threads;
    thread->iface   = iface;
    thread->sem_int = calloc(1, sizeof(sem_t));
    threads = thread;
    sem_init(thread->sem_int, 0, 0);
    thread->tid = pthread_create(&(thread->tid), NULL, iface_thread, thread);
    iface = iface->next;
    if (iface == config_pending->interfaces) break;
  }

  // Mark the updated config as current
  if (config) config_free(config);
  config = config_pending;
  config_pending = NULL;
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

  // Basic config validation
  if (!loaded->interfaces) {
    fprintf(stderr, "No interfaces defined in config\n");
    return 1;
  }

  // Trigger update
  config_pending = loaded;
  update_workers();

  // TODO: monitor threads here
  // TODO: capture USR1 for config reload
  sleep(10);

  // Trigger graceful shutdown
  config_pending = calloc(1, sizeof(pmlag_configuration));
  update_workers();
  return 0;

  /* // DEBUG: Show loaded configuration */
  /* fprintf(stderr, "Loaded configuration:\n"); */
  /* pmlag_interface *iface = loaded->interfaces; */
  /* while(iface) { */
  /*   fprintf(stderr, "  Interface: %s\n", iface->name); */
  /*   fprintf(stderr, "    master   : %s\n", iface->master ? iface->master : ""); */
  /*   fprintf(stderr, "    mac      : %s\n", iface->mac    ? iface->mac    : ""); */
  /*   fprintf(stderr, "    broadcast: %s%s\n", */
  /*     iface->broadcast == BROADCAST_FLOOD    ? "flood"    : "", */
  /*     iface->broadcast == BROADCAST_BALANCED ? "balanced" : "" */
  /*   ); */
  /*   fprintf(stderr, "    mode     : %s%s%s%s\n", */
  /*     iface->mode == MODE_SLAVE         ? "slave"         : "", */
  /*     iface->mode == MODE_ACTIVE_BACKUP ? "active-backup" : "", */
  /*     iface->mode == MODE_BROADCAST     ? "broadcast"     : "", */
  /*     iface->mode == MODE_BALANCE_RR    ? "balance-rr"    : "" */
  /*   ); */
  /*   fprintf(stderr, "    weight   : %d\n", iface->weight); */
  /*   iface = iface->next; */
  /*   if (iface == loaded->interfaces) break; */
  /* } */




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

  /* return 0; */
}

#ifdef __cplusplus
} // extern "C"
#endif
