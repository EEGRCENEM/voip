#ifndef SIP_H
#define SIP_H

#ifndef SIP_DEBUG
#define SIP_DEBUG 0
#endif

#define SIP1 "SIP/1.0"
#define SIP2 "SIP/2.0"

#define SIP_PORT "5060"
#define SIPS_PORT "5061"

typedef enum {
  // REGISTER for registering contact information.
  REGISTER,
  // INVITE, ACK, and CANCEL for setting up sessions.
  INVITE,
  ACK,
  CANCEL,
  // BYE for terminating sessions.
  BYE,
  // OPTIONS for querying servers about their capabilities.
  OPTIONS,
} SIPMethod;

typedef enum {
  //  The 100 (Trying) response indicates that the INVITE has
  // been received and that the proxy is working on her behalf to route
  // the INVITE to the destination.
  TRYING = 100,
  RINGING = 180,
  OK = 200,
  BUSY_HERE = 486,
  NOT_ACCEPTABLE_HERE = 488,
} SIPStatusCode;

/*  Its general form, in the case of a SIP URI, is:
 *
 *  sip:user:password@host:port;uri-parameters?headers
 *
 *  The format for a SIPS URI is the same, except that the scheme is
 *  "sips" instead of sip
 */
#define SIP_PREFIX "sip:"
#define SIPS_PREFIX "sips:"

typedef struct {
  char *name;
  char *value;
} SIPRequestHeader;

typedef struct {
  char *protocol_version;
  char *recipient;
  SIPMethod method;
  // Header information
  unsigned long header_count;
  unsigned long header_capacity;
  SIPRequestHeader *headers;
} SIPRequest;

typedef struct {
  char *user;
  char *host;
  char *port;
} SIPAddress;

/** @brief Parse a SIP URI into and an address.
 *
 *  @param uri the URI to parse.
 *  @param address The parsed address.
 *  @return true if parsing succeeded.
 */
bool sip_parse_address(const char *uri, SIPAddress *address);

/** @brief Allocates a new request struct with the given number
 *         of headers preallocated.
 *
 *  @param num_headers the number of headers to preallocate.
 *  @return pointer to allocated SIPRequest.
 */
SIPRequest *sip_create_request(unsigned long num_headers);

/** @brief Frees a request struct.
 *
 *  @param request the structure to free.
 */
void sip_delete_request(SIPRequest *request);

/** @brief Tests a request for validity.
 *
 *  @param request the validate.
 *  @return true if request is valid.
 */
bool sip_validate_request(const SIPRequest *request);

// Serialisation / deserialisation.
void sip_serialise_request(const SIPRequest *request, char *dst);
void sip_deserialise_request(char *src, SIPRequest *request);

// Requests.
bool sip_send_request(const SIPRequest *request);
bool sip_listen(const char *host, unsigned int max_clients, char *buffer,
                const unsigned long buffer_size);

// RFC 3261 SIP: Session Initiation Protocol June 2002
// https://datatracker.ietf.org/doc/html/rfc3261

#endif
