#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sip.h"

#define NO_FLAG 0

// Conversion from status to string.
const char *const reason_phrase[] = {
    [TRYING] = "TRYING",
    [RINGING] = "RINGING",
    [OK] = "OK",
    [BUSY_HERE] = "BUSY_HERE",
    [NOT_ACCEPTABLE_HERE] = "NOT_ACCEPTABLE_HERE",
};

const struct addrinfo hints = {
    .ai_family = AF_UNSPEC,     // Either IPv4 or IPv6
    .ai_socktype = SOCK_STREAM, // TCP sockets
    .ai_flags = NO_FLAG,
};

static int sip_establish_socket(const struct addrinfo *server_info) {
  return socket(server_info->ai_family, server_info->ai_socktype,
                server_info->ai_protocol);
}

bool sip_send_request(const SIPRequest *request) {
  const char *host = request->recipient;
  const char *port = SIP_PORT;

  struct addrinfo *server_info;
  int err_code = getaddrinfo(host, port, &hints, &server_info);

  if (err_code != 0) {
    fprintf(stderr, "Failed to establish address info %s.\n",
            gai_strerror(err_code));
    return false;
  }

  int sockfd = sip_establish_socket(server_info);

  if (sockfd == -1) {
    fprintf(stderr, "Failed to establish socket: %s on %s:%s.\n",
            strerror(sockfd), host, port);
    freeaddrinfo(server_info);
    return false;
  }

  err_code = connect(sockfd, server_info->ai_addr, server_info->ai_addrlen);
  if (err_code != 0) {
    fprintf(stderr, "Failed to connect socket: %s.\n", strerror(err_code));
    close(sockfd);
    freeaddrinfo(server_info);
    return false;
  }

  // Serialise request and send it through to the server.
  char message[256];
  sip_serialise_request(request, message);

  [[maybe_unused]] const int bytes_sent =
      send(sockfd, message, strlen(message), NO_FLAG);
#if SIP_DEBUG
  fprintf(stdout, "Sent %i bytes to server\n", bytes_sent);
#endif

  if (close(sockfd) != 0) {
    fprintf(stderr, "Failed to close socket: %s.\n", strerror(err_code));
    freeaddrinfo(server_info);
    return false;
  }

  freeaddrinfo(server_info);
  return true;
}

static bool sip_accept_loop(int server_sockfd, char *buffer,
                            size_t buffer_size) {
  bool buffer_filled = false;

  int err_code;
  struct sockaddr_in client_addr;
  socklen_t socket_size = sizeof client_addr;

  int client_sockfd;
  while (true) {
    client_sockfd =
        accept(server_sockfd, (struct sockaddr *)&client_addr, &socket_size);
    if (client_sockfd == -1) {
      fprintf(stderr, "Failed to accept client, continuing...");
      continue;
    }

    fprintf(stdout, "Got connection from %s...\n",
            inet_ntoa(client_addr.sin_addr));

    err_code = recv(client_sockfd, buffer, buffer_size, NO_FLAG);
    if (err_code == -1) {
      fprintf(stderr, "Failed to recieve message from client.");
      break;
    } else {
      buffer_filled = true;
      break;
    }
    // TODO: Catch this.
    err_code = close(client_sockfd);
  }
  return buffer_filled;
}

bool sip_listen(const char *host, unsigned int max_clients, char *buffer,
                const unsigned long buffer_size) {
  const char *port = SIP_PORT;

  struct addrinfo *server_info, *next_info;

  int err_code = getaddrinfo(host, port, &hints, &server_info);
  if (err_code != 0) {
    fprintf(stderr, "Failed to establish address info %s.\n",
            gai_strerror(err_code));
    return false;
  }

  int server_sockfd;
  int yes = 1;

  // Move through the hops to connect to the right host.
  for (next_info = server_info; next_info != nullptr;
       next_info = next_info->ai_next) {

    // TODO: Understand this.
    server_sockfd = sip_establish_socket(next_info);

    if (server_sockfd == -1) {
      continue;
    }
    err_code =
        setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (err_code == -1) {
      perror("setsockopt");
      return false;
    }

    err_code = bind(server_sockfd, next_info->ai_addr, next_info->ai_addrlen);
    if (err_code == -1) {
      close(server_sockfd);
      continue;
    }

    break;
  }

  freeaddrinfo(server_info);
  if (next_info == nullptr) {
    fprintf(stderr, "Failed to connect to host");
    return false;
  }

  // Defines the maximum length to which the queue of pending connections for
  // sockfd may grow.
  if (listen(server_sockfd, max_clients) == -1) {
    fprintf(stderr, "Failed to listen on host");
    return false;
  }

  // Accept loop.
  const bool buffer_filled =
      sip_accept_loop(server_sockfd, buffer, buffer_size - 1);
  if (close(server_sockfd) != 0) {
    fprintf(stdout, "Failed to close socket.\n");
    return false;
  }

  return buffer_filled;
}
