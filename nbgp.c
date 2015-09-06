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

bool      listening = false;
bgp_peer  my;
struct addrinfo standard_hints;

int main(int argc, char *argv[]) {

  bgp_peer        *peer;
  //struct addrinfo standard_hints; Moved outside of main().

  my.as = 5507;
  my.holdtime = 180;
  my.id = inet_addr("172.16.78.2");

  // Set client side (us) hints.
  memset(&standard_hints, 0, sizeof(standard_hints));
  standard_hints.ai_family    = AF_UNSPEC;    // IPv4, or IPv6, or IPv...
  standard_hints.ai_socktype  = SOCK_STREAM;  // TCP

  // Initialise the queue.
  TAILQ_INIT(&bgp_peer_q);

///////////////////////////////////////
// Temporarily populate the peers queue.

  char *addresses[] = {"172.16.78.1", "172.16.78.3", "172.16.78.5"};
  int asns[] = {1400, 1401, 1401};
  bool passive[] = {false, false, true};

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
    peer->passive = passive[i];

    TAILQ_INSERT_TAIL(&bgp_peer_q, peer, entries);

    printf("Added AS %d peer %s\n", peer->as, friendly_ip(peer->address));

  }

///////////////////////////////////////

  pthread_t client_cycle_thread;
  pthread_t server_cycle_thread;
  
  pthread_create(&client_cycle_thread, NULL, start_client_cycle, NULL);
  pthread_create(&server_cycle_thread, NULL, start_server_cycle, NULL);

  pthread_join(client_cycle_thread, NULL);
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

uint16_t friendly_port(struct addrinfo *p) {
  uint16_t port;
  
  //
  switch(p->ai_family) {
    case AF_INET:
      port = ((struct sockaddr_in *)p->ai_addr)->sin_port;
      break;
    case AF_INET6:
      port = ((struct sockaddr_in6 *)p->ai_addr)->sin6_port;
      break;
  }

  //inet_ntop(p->ai_family, addr, str, INET6_ADDRSTRLEN);

  return ntohs(port);
}

void *start_client_cycle() {

  bgp_peer *next;

  printf("Client cycle thread started...\n");

  next = TAILQ_FIRST(&bgp_peer_q);
  if(next == NULL) {
    printf("No peers configured\n");
    return;
  }
  
  

  for(;;) {
    
    //printf("Next address is %s\n", friendly_ip(next->address));

    // Finite State Machine
    check_state(next);


    next = TAILQ_NEXT(next, entries);
    if(next == NULL) {
      next = TAILQ_FIRST(&bgp_peer_q);
    }
    usleep(500000); // Half a second sleep
  }
}

void *start_server_cycle() {
  printf("Server cycle thread started...");
  printf("Nothing to do, so exiting server cycle.\n");

  // When listening, set listening to true.
  listening = true;
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
      // If not a passive peer, try and connect:
      if(peer->passive == false) {
        peer->state = STATE_CONNECT;
        printf("Peer %s changed state to CONNECT\n", friendly_ip(peer->address));
        bgp_open(peer);
      } else {
        // If passive, the state should change to Active as soon as TCP connections are being accepted.
        if(listening) {
          peer->state = STATE_CONNECT;
          printf("Peer %s changed state to ACTIVE\n", friendly_ip(peer->address));
        }
      } 
      break;
    case STATE_CONNECT:
      //
      
      break;
    case STATE_ACTIVE:
      //
      //peer->state = STATE_NULL;
      break;
  }
  return;
}

void bgp_open(bgp_peer *peer) {

  struct bgp_header header;
  struct bgp_msg_open open;

  uint16_t message_length = sizeof(header) + sizeof(open);


  open.version = 4;
  open.as = htons(my.as);
  open.holdtime = htons(my.holdtime);
  open.id = my.id;
  // Could add optional parameter TLVs here.
  open.optlen = sizeof((struct bgp_opt_param*)open.params)/sizeof(open.params[0]);

  printf("Checking opt values length. Expecting 0, got %ld\n", sizeof((struct bgp_opt_param*)open.params)/sizeof(open.params[0]));
  
  memset(&header.marker, 0xFF, sizeof(header.marker));
  header.type = OPEN;
  header.len =  htons(message_length);

  char buffer[message_length];
  printf("----> sizeof header %ld\n", sizeof(header));
  printf("----> sizeof open %ld\n", sizeof(open));
  printf("----> Made a buffer the size %d\n", message_length);
  int pos = 0;
  memcpy(buffer + pos, &header, sizeof(header));
  pos += sizeof(header);
  memcpy(buffer + pos, &open, sizeof(open));

///////////////////////////////////////
// Temporarily send a message to the peer.

  int fd = socket(peer->address->ai_family, peer->address->ai_socktype, peer->address->ai_protocol);
  if(fd == -1) {
    perror("socket");
    //exit(EXIT_FAILURE);
  }
  printf("Trying to connect to %s:%u\n", friendly_ip(peer->address), (unsigned int)friendly_port(peer->address));
  if(connect(fd, peer->address->ai_addr, peer->address->ai_addrlen) != 0) {
    perror("connect");
    //exit(EXIT_FAILURE);
  }
  printf("Looks like we are connected to %s:%u\n", friendly_ip(peer->address), (unsigned int)friendly_port(peer->address));

  // Now sent the OPEN message over the socket.
  if(send(fd, buffer, message_length, 0) < 0) {
    perror("send");
    // Error counter?
  }


  // Then peer->state = STATE_OPENSENT

/////////////////////////////////////// 



  printf("Checking outer header length. Expecting 19, got %ld\n", sizeof(header));
  printf("Checking OPEN message length (excl header). expecting 10, got %ld\n", sizeof(open));
  printf("Total packet len: %d\n", header.len);

}
























