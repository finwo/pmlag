#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../util/config.h"
#include "../util/socket.h"

int task_iface_onpacket(struct pmlag_iface *iface, unsigned char *buffer, size_t buflen) {
  size_t send_len;

  // Debug: print eth header
  printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x < %.2x:%.2x:%.2x:%.2x:%.2x:%.2x, %.4x (%ld)\n",
    buffer[0],buffer[1],buffer[2],buffer[3],buffer[ 4],buffer[ 5], // DST
    buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11], // SRC
    ((unsigned int)((unsigned char)buffer[12]) << 8) + buffer[13], // PROTO
    buflen
  );

  // Redirect packet to bond socket as-is
  send_len = write(iface->bond->sockfd, buffer, buflen);
  if (buflen != send_len) {
    perror("write(bond)");
    return 1;
  }

  return 0;
}

void * task_iface_thread(void *arg) {
  struct pmlag_iface *iface = arg;

  // Open socket for the interface in the bond
  iface->sockfd = sockraw_open(iface->name);
  if (iface->sockfd < 0) {
    pthread_exit(NULL);
    return NULL;
  }

  // Get the interface's idx on the socket
  iface->ifidx = iface_idx(iface->sockfd, iface->name);

  // Reserve receive buffer, support 64k packets just in case
  size_t buflen;
  unsigned char *buffer = (unsigned char *) malloc(RCVBUFSIZ);
  struct sockaddr saddr;
  int saddr_len  = sizeof(saddr);

  // Wait for the bond thread to finish initializing
  pthread_mutex_lock(&(iface->bond->mtx_rt));
  pthread_mutex_unlock(&(iface->bond->mtx_rt));

  printf("Thread started for iface %s\n", iface->name);

  // This is our life now
  while(1) {

    // Zero out buffer, to prevent pollution, & receive packet
    /* memset(buffer, 0, RCVBUFSIZ); */
    buflen = recvfrom(iface->sockfd, buffer, RCVBUFSIZ, 0, &saddr, (socklen_t *)&saddr_len);
    if (buflen < 0) {
      perror("recvfrom");
      pthread_exit(NULL);
      return NULL;
    }

    // And let the more specific method handle the packet
    if(task_iface_onpacket(iface, buffer, buflen)) {
      pthread_exit(NULL);
      return NULL;
    }
  }

  return NULL;
}


/*     // Update routing table if our custom protocol is seen */
/*     proto = ((uint16_t)((unsigned char)buffer[(ETH_ALEN*2)+0]) << 8) + buffer[(ETH_ALEN*2)+1]; */
/*     if (proto == 0x0666) { */
/*       bcidx = ((uint16_t)((unsigned char)buffer[(ETH_ALEN*2)+2]) << 8) + buffer[(ETH_ALEN*2)+3]; */
/*       iface_add_rt(iface, buffer+ETH_ALEN, bcidx); */
/*       continue; */
/*     } else { */
/*       iface_add_rt(iface, buffer+ETH_ALEN, 0); */
/*     } */

/*     /1* trim_rt(iface->bond, *1/ */ 
/*     rt_len = btree_count(iface->bond->rt); */
/*     printf("Current RT length: %ld\n", rt_len); */


