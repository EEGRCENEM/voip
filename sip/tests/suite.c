#include <stdio.h>
#include <string.h>

#include "../lib/sip.h"

typedef struct {
  char *name;
  bool (*test_func)(void);
  bool passed;
} TestCase;

typedef struct {
  int total;
  int passed;
  int failed;
} TestResults;

static bool run_single_test(TestCase *test) {
  bool result = test->test_func();
  test->passed = result;
  return result;
}

TestResults run_test_suite(TestCase *tests, int count) {
  TestResults results = {0};

  printf("=== RUNNING TEST SUITE ===\n");

  for (int i = 0; i < count; i++) {
    if (run_single_test(&tests[i])) {
      printf("- Test: %s [PASSED]\n", tests[i].name);
      results.passed++;
    } else {
      printf("- Test: %s [FAILED]\n", tests[i].name);
      results.failed++;
    }
    results.total++;
  }

  printf("=== TEST SUITE: %i of %i tests PASSED ===\n", results.passed,
         results.total);

  return results;
}

int main(int argc, char *argv[]) {
  // Forward declare tests.
  extern bool test_serialise_request(void);
  extern bool test_deserialise_request(void);
  extern bool test_send_request(void);
  extern bool test_parse_address(void);

  TestCase tests[] = {
      {"Serialize SIP Request", test_serialise_request},
      {"Deserialize SIP Request", test_deserialise_request},
      {"Send SIP Request", test_send_request},
      {"Parse SIP URI", test_parse_address},
  };

  int test_count = sizeof(tests) / sizeof(tests[0]);
  TestResults results = run_test_suite(tests, test_count);

  return results.failed > 0 ? 1 : 0;
}

bool test_serialise_request(void) {
  char line[256];

  char *user = "bob@biloxi.com";

  SIPRequestHeader headers[2];
  headers[0] = (SIPRequestHeader){
      .name = "To",
      .value = "Bob <sip:bob@biloxi.com>",
  };
  headers[1] = (SIPRequestHeader){
      .name = "From",
      .value = "Alice <sip:alice@atlanta.com>",
  };

  const SIPRequest request = {
      .protocol_version = SIP2,
      .recipient = user,
      .method = INVITE,
      .headers = headers,
      .header_count = sizeof(headers) / sizeof(headers[0]),
      .header_capacity = 4,
  };

  sip_serialise_request(&request, line);

  const char expected[] =
      "INVITE sip:bob@biloxi.com SIP/2.0\nTo: Bob <sip:bob@biloxi.com>\nFrom: "
      "Alice <sip:alice@atlanta.com>\n";

  if (strcmp(line, expected) == 0) {
    return true;
  }
  return false;
}

bool test_deserialise_request(void) {
  SIPRequestHeader headers[] = {};

  char *recipient = "";

  SIPRequest request = {
      .protocol_version = "",
      .method = ACK,
      .recipient = recipient,
      .headers = headers,
      .header_count = 0,
      .header_capacity = 2,
  };

  char content[] =
      "INVITE sip:bob@biloxi.com SIP/2.0\nTo: Bob <sip:bob@biloxi.com>\nFrom: "
      "Alice <sip:alice@atlanta.com>\n";

  sip_deserialise_request(content, &request);

  if (request.method != INVITE) {
    fprintf(stderr, "Method not matching, got %d, expected %d\n",
            request.method, INVITE);
    return false;
  }

  if (strcmp(request.protocol_version, SIP2) != 0) {
    fprintf(stderr, "Protocol not matching, got %s, expected %s\n",
            request.protocol_version, SIP2);
    return false;
  }

  if (strcmp(request.recipient, "bob@biloxi.com") != 0) {
    fprintf(stderr, "Recipient not matching.\n");
    return false;
  }

  if (request.header_count != 2) {
    fprintf(stderr, "Header count not matching.\n");
    return false;
  }

  if (strcmp(request.headers[1].value, "Alice <sip:alice@atlanta.com>") != 0) {
    fprintf(stderr, "Header value not matching.\n");
    return false;
  }

  return true;
}

bool test_send_request(void) {
  char *user = "127.0.0.1";

  const SIPRequest request = {
      .protocol_version = SIP2,
      .recipient = user,
      .method = INVITE,
  };

  // TODO: Ensure that headers is not null pointer.

  const bool passed = sip_send_request(&request);
  if (!passed) {
    fprintf(stdout,
            "Ensure that TCP server is live, e.g. `nc -l " SIP_PORT "`.\n");
  }

  return passed;
}

bool test_parse_address(void) {
  SIPAddress address = {
    .user = "",
    .host = "",
    .port = "",
  };

  const char *sip_uri = "sip:bob@biloxi.com:5132";
  sip_parse_address(sip_uri, &address);

  if (strcmp(address.user, "bob") != 0) {
    return false;
  }
  if (strcmp(address.host, "biloxi.com") != 0) {
    return false;
  }
  if (strcmp(address.port, "5132") != 0) {
    return false;
  }

  const char *sips_uri = "sips:alice@test.com";
  sip_parse_address(sips_uri, &address);

  if (strcmp(address.user, "alice") != 0) {
    return false;
  }
  if (strcmp(address.host, "test.com") != 0) {
    return false;
  }
  // Port is not given, infer it from protocol.
  if (strcmp(address.port, SIPS_PORT) != 0) {
    return false;
  }


  return true;
}
