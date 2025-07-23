#include "lib/sip.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  const char *host = "127.0.0.1";
  const unsigned long buffer_size = 256;

  char buffer[buffer_size];
  memset(buffer, 0, buffer_size);

  SIPRequest *request = sip_create_request(1);

  // Await a request from the client, then deserialize it.
  if (sip_listen(host, 4, buffer, buffer_size)) {
    sip_deserialise_request(buffer, request);
    fprintf(stdout, "Request: %s with method %u\n", request->recipient, request->method);
  } else {
    fprintf(stdout, "Failed to find client.\n");
  }

  sip_delete_request(request);

  return 0;
}
