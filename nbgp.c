#include "nbgp.h"
#include <stdio.h>        // sprintf()
#include <stdlib.h>       // malloc()
#include <string.h>       // memset()
#include <arpa/inet.h>
#include <sys/queue.h>    // TAILQ_* macros.
#include <pthread.h>
//#include <sys/types.h>
//#include <sys/socket.h>
#include <netdb.h>        // struct addrinfo





int main(int argc, char *argv[]) {

  bgp_peer        *peer;
  struct addrinfo standard_hints;

  // Set client side (us) hints.
  memset(&standard_hints, 0, sizeof(standard_hints));
  standard_hints.ai_family    = AF_UNSPEC;    // IPv4, or IPv6, or IPv...
  standard_hints.ai_socktype  = SOCK_STREAM;  // TCP

  // Initialise the queue.
  TAILQ_INIT(&bgp_peer_q);

///////////////////////////////////////
// Temporarily populate the peers queue.

  char *addresses[] = {"192.168.1.2", "192.168.1.3"};
  int asns[] = {1400, 1401};

  int sizes = (int) (sizeof(addresses)/sizeof(char*));

  int i;
  for(i = 0; i<sizes; i++) {

    peer = malloc(sizeof(*peer));
    if(peer == NULL) {
      perror("malloc failed");
      exit(EXIT_FAILURE);
    }

    getaddrinfo(addresses[i], "bgp", &standard_hints, &peer->address);
    peer->as = asns[i];

    TAILQ_INSERT_TAIL(&bgp_peer_q, peer, entries);

    printf("Added AS %d peer %s\n", peer->as, friendly_ip(peer->address));

  }

///////////////////////////////////////


  pthread_t server_cycle_thread;
  pthread_t client_cycle_thread;


  pthread_create(&server_cycle_thread, NULL, start_server_cycle, NULL);

  pthread_join(server_cycle_thread, NULL);


  return 0;

}


char *friendly_ip(struct addrinfo *p) {
  void *addr;
  char *str = malloc(INET6_ADDRSTRLEN); // Max address length so far
  if(str == NULL) {
    perror("malloc failed");
    return NULL;
  }

  //
  switch(p->ai_family) {
    case AF_INET:
      addr = &((struct sockaddr_in *)p->ai_addr)->sin_addr;
      break;
    case AF_INET6:
      addr = &((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
      break;
  }

  inet_ntop(p->ai_family, addr, str, INET6_ADDRSTRLEN);

  return str;
}

void *start_server_cycle() {

  bgp_peer *next;

  next = TAILQ_FIRST(&bgp_peer_q);
  if(next == NULL) {
    printf("No peers configured\n");
    return;
  }
  
  printf("Server cycle thread\n");

  for(;;) {
    
    printf("Next address is %s\n", friendly_ip(next->address));

    // Finite State Machine
    check_state(next);


    next = TAILQ_NEXT(next, entries);
    if(next == NULL) {
      next = TAILQ_FIRST(&bgp_peer_q);
    }
    usleep(3000000);
  }
}

// Finite State Machine
void check_state(bgp_peer *peer) {
  switch(peer->state) {
    // Set initial state
    case STATE_NULL:
      peer->state = STATE_IDLE;
      printf("Peer %s changed state to IDLE\n", friendly_ip(peer->address));
      break;
    case STATE_IDLE:

      break;
  }
  return;
}