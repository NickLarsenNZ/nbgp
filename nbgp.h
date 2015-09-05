//#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>        // struct addrinfo
#include <sys/queue.h>    // TAILQ_* macros.

#define BGP_PORT 179

char *friendly_ip(struct addrinfo*);
void *start_server_cycle();
void *start_client_cycle();


// BGP Peer Definition
typedef struct bgp_peer {
  TAILQ_ENTRY (bgp_peer)    entries;  // Pointers to the previous and next peers
  struct addrinfo           *address;  // AF Independent Peer
  uint16_t                  as;       // Autonomous System Number (2 Bytes)
} bgp_peer;

TAILQ_HEAD(, bgp_peer) bgp_peer_q;    // Queue containing all of our peers

enum bgp_states {
  STATE_IDLE,
};