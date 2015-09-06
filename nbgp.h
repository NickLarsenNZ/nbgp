//#include <sys/types.h>
//#include <arpa/inet.h>
#include <netdb.h>        // struct addrinfo.
#include <sys/queue.h>    // TAILQ_* macros.

#define BGP_PORT 179

// Making a bool type since it's no part of standard C.
typedef enum { false, true } bool;

// BGP Peer Definition
typedef struct bgp_peer {
  TAILQ_ENTRY (bgp_peer)    entries;  // Pointers to the previous and next peers.
  struct addrinfo           *address; // AF Independent Peer.
  uint16_t                  as;       // Autonomous System Number (2 Bytes).
  uint8_t                   state;    // BGP peer state.
  uint16_t                  holdtime; // 0, or >3, default is 180, in the peering session, the lowest hold time wins.
  bool                      passive;  // Whether to attempt an outbound conneciton, or wait for incoming connections.
  uint32_t                  id;       // BGP/Router ID. Host Byte Order, must change to Network Byte Order (htonl).
} bgp_peer;

TAILQ_HEAD(, bgp_peer) bgp_peer_q;    // Queue containing all of our peers.

// Finite State Machine states.
enum bgp_states {
  STATE_NULL,                         // Added this so I could do a NULL state check.
  STATE_IDLE,                         // Initial peer state.
  STATE_CONNECT,                      // When an outgoing TCP connection is being established.
  STATE_ACTIVE,                       // When we are listening for incoming TCP connections to be established.
  STATE_OPENSENT,                     // When a TCP socket is established, and OPEN message has been sent.
  STATE_OPENCONFIRM,                  // When we get an OPEN reply from the peer.
  STATE_ESTABLISHED                   // After checking everything is ok between the OPEN messages.
};

enum bgp_msg_types {
  OPEN          = 1,
  UPDATE        = 2,
  NOTIFICATION  = 3,
  KEEPALIVE     = 4
};

// Common BGP Header
struct bgp_header {
  u_char    marker[16];               // All ones.
  uint16_t  len;                      // Total message length.
  uint8_t   type;                     // The type of the following message (OPEN, UPDATE, NOTIFICATION, KEEPALIVE).
} __attribute__((__packed__));        // Very important to have the packed attribute, otherwise padding will be added, making it hard to use sizeof on the struct.

// TLVs
struct bgp_opt_param {
  uint8_t   type;                     // Param type.
  uint8_t   len;                      // TLV length.
  u_char   *value;                    // Variable length value.
} __attribute__((__packed__));        // Very important to have the packed attribute, otherwise padding will be added, making it hard to use sizeof on the struct.

// OPEN message
struct bgp_msg_open {
  uint8_t   version;                  // BGP version 4 (currently).
  uint16_t  as;                       // 2-Byte Autonomous System Number (if using a 4-Byte ASN, set this to 23456, then send a capability message with the 4-Byte ASN).
  uint16_t  holdtime;                 // 0, or >3, default is 180, in the peering session, the lowest hold time wins.
  uint32_t  id;                       // BGP/Router ID.
  uint8_t   optlen;                   // Optional Parameters Total Length.
  struct    bgp_opt_param params[];   // Optional Paramater TLVs.
} __attribute__((__packed__));        // Very important to have the packed attribute, otherwise padding will be added, making it hard to use sizeof on the struct.

char *friendly_ip(struct addrinfo*);
uint16_t friendly_port(struct addrinfo*);
void *start_server_cycle();
void *start_client_cycle();
void check_state(bgp_peer*);
void bgp_open(bgp_peer*);





















