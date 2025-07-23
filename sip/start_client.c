#include "lib/sip.h"

int main(int argc, char *argv[]) {
  char *user = "127.0.0.1";

  const SIPRequest request = {
      .protocol_version = SIP2,
      .recipient = user,
      .method = INVITE,
  };

  sip_send_request(&request);

  return 0;
}
