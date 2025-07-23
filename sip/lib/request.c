#include "sip.h"
#include <_string.h>
#include <stdio.h>
#include <stdlib.h>

static bool starts_with(const char *string, const char *substring) {
  return strncmp(string, substring, strlen(substring)) == 0;
}

static bool parse_port(const char *string, char *port) {
  const size_t max_suffix_len = 6;
  const size_t len = strlen(string);

  if (len < max_suffix_len) {
    return false;
  }

  const char *end_string;
  for (int i = 1; i < max_suffix_len; i++) {
    end_string = string + len - i;

    // We've found the port signifier.
    if (strncmp(end_string, ":", 1) == 0) {
      // 0 is not a valid port.
      if (strncmp(end_string + 1, "0", i) == 0) {
        return false;
      }
      strncpy(port, end_string + 1, i);
      return true;
    }
  }
  return false;
}

bool sip_parse_address(const char *uri, SIPAddress *address) {
  // General format:
  //  sip[s]:user:password@host:port;uri-parameters?headers
  //
  // This implementation will _not_ support:
  //    1. password (insecure anyway)
  //    2. URI parameters (instead define them on the struct).
  //    3. header parameters (instead define them on the struct).
  if (address == nullptr) {
    return false;
  }

  char user[64];
  char host[64];
  // Possible values 1 - 65536
  char port[5];

  bool is_secure_uri = starts_with(uri, SIPS_PREFIX);
  bool is_insecure_uri = starts_with(uri, SIP_PREFIX);

  // Failed to parse port, use default port.
  if (!parse_port(uri, port)) {
    strcpy(port, is_secure_uri ? SIPS_PORT : SIP_PORT);
  }

  const char *uri_format = "%[^@ \t\n]@%[^: \t\n]";

  if (is_insecure_uri) {
    char format[64] = "sip:";
    strcat(format, uri_format);

    if (sscanf(uri, format, user, host) != 2) {
      fprintf(stderr, "Failed to parse request SIP uri %s\n", uri);
      return false;
    }
  } else if (is_secure_uri) {
    char format[64] = "sips:";
    strcat(format, uri_format);

    if (sscanf(uri, format, user, host) != 2) {
      fprintf(stderr, "Failed to parse request SIPS uri %s.\n", uri);
      return false;
    }
  } else {
    return false;
  }

  address->user = user;
  address->host = host;
  address->port = port;

  return true;
}

SIPRequest *sip_create_request(unsigned long num_headers) {
  SIPRequest *request = malloc(sizeof(SIPRequest));
  if (!request) {
    return nullptr;
  }

  // Write basics
  request->protocol_version = SIP2;
  request->method = ACK;

  // Write recipient
  request->recipient = strdup("");
  if (!request->recipient) {
    free(request);
    return nullptr;
  }

  // Write headers
  request->header_count = 0;
  request->header_capacity = num_headers;
  request->headers = malloc(num_headers * sizeof(SIPRequestHeader));
  if (!request->headers) {
    free(request->recipient);
    free(request);
    return nullptr;
  }

  for (int i = 0; i < num_headers; i++) {
    request->headers[i].name = nullptr;
    request->headers[i].value = nullptr;
  }

  return request;
}

void sip_delete_request(SIPRequest *request) {
  if (!request) {
    return;
  }

  if (request->headers) {
    for (int i = 0; i < request->header_capacity; i++) {
      free(request->headers[i].name);
      free(request->headers[i].value);
    }
    free(request->headers);
  }

  free(request->recipient);
  free(request);
}

bool sip_validate_request(const SIPRequest *request) {
  if (request == nullptr) {
    return false;
  }

  SIPAddress address = {
      .user = "",
      .host = "",
      .port = "",
  };

  if (!sip_parse_address(request->recipient, &address)) {
    return false;
  }

  if (!(strcmp(request->protocol_version, SIP1) == 0 ||
        strcmp(request->protocol_version, SIP2)) == 0) {
    return false;
  }

  return true;
}
