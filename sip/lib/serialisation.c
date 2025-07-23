#include "sip.h"
#include <stdio.h>
#include <string.h>

// Conversion from method to string.
const char *const method_name[] = {
    [REGISTER] = "REGISTER", [INVITE] = "INVITE", [ACK] = "ACK",
    [CANCEL] = "CANCEL",     [BYE] = "BYE",       [OPTIONS] = "OPTIONS",
};

static SIPMethod convert_name_to_method(const char *name) {
  if (strcmp(name, "REGISTER") == 0) {
    return REGISTER;
  }
  if (strcmp(name, "INVITE") == 0) {
    return INVITE;
  }
  if (strcmp(name, "ACK") == 0) {
    return ACK;
  }
  if (strcmp(name, "CANCEL") == 0) {
    return CANCEL;
  }
  if (strcmp(name, "BYE") == 0) {
    return BYE;
  }
  if (strcmp(name, "OPTIONS") == 0) {
    return OPTIONS;
  }
  return CANCEL;
}

static void serialise_request(char *dst, SIPMethod method,
                              const char *request_uri, const char *protocol) {
  strcpy(dst, method_name[method]);
  strcat(dst, " ");
  strcat(dst, "sip:");
  strcat(dst, request_uri);
  strcat(dst, " ");
  strcat(dst, protocol);
  strcat(dst, "\n");
}

static void serialise_header(char *dst, const SIPRequestHeader *header) {
  strcat(dst, header->name);
  strcat(dst, ": ");
  strcat(dst, header->value);
  strcat(dst, "\n");
}

void sip_serialise_request(const SIPRequest *request, char *dst) {
  serialise_request(dst, request->method, request->recipient,
                    request->protocol_version);

  // Headers are not strictly required at the moment.
  if (request->headers == nullptr) {
    return;
  }
  for (int i = 0; i < request->header_count; i++) {
    serialise_header(dst, &request->headers[i]);
  }
}

static void deserialise_request(const char *src, SIPRequest *request) {
  char method_name[8];
  char version[8];
  // TODO: This is large.
  char identity[256];

  const char *format = "%s sip:%s %s";
  if (sscanf(src, format, method_name, identity, version) != 3) {
    fprintf(stderr, "Failed to parse request.\n");
  }

  const SIPMethod method = convert_name_to_method(method_name);

  request->protocol_version = version;
  request->method = method;
  request->recipient = identity;
}

static void deserialise_header(const char *src, SIPRequestHeader *header) {
  char name[256];
  char value[256];

  const char *format = "\n%[^:]: %[^\n]";
  if (sscanf(src, format, name, value) != 2) {
    fprintf(stderr, "Failed to parse header.\n");
    return;
  }

  header->name = name;
  header->value = value;
}

void sip_deserialise_request(char *src, SIPRequest *request) {
  char *current = src;
  char *next;

  deserialise_request(src, request);

  int i = 0;
  while (current) {
    if (request->header_capacity < i) {
      fprintf(stderr, "No capacity for additional headers\n");
    }

    next = strchr(current, '\n');
    if (strcmp(next, "\n") == 0) {
      break;
    }

    deserialise_header(next, &request->headers[i]);
    current = next ? (next + 1) : NULL;
    i++;
  }

  request->header_count = i;
}
