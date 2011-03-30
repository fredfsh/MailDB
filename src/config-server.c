/* Refer to the comments at the beginning of "config-server.h".

   By fredfsh (fredfsh@gmail.com)
*/
#include "config-server.h"
#include "redis.h"
#include "router.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

chMap list;
unsigned char snapshot[MAX_SNAPSHOT_LENGTH];

// Logs.
void _log(const char *log) {
  printf("[log]config-server.c: %s\n", log);
}

// Heartbeats to each live servers in the system.
// Handler of SIGALRM.
//
// Config server heartbeats to each server in the list one by one. If the
// connection cannot be established, the target is regared as unreachable. If
// the Redis PING command fails, the Redis process on the target is regarded
// as dead.
void _heartbeat(int signum, siginfo_t *info, void *unused) {
  int rv;
  int sockfd;
  int status;
  chNode *node;
  char log[MAX_LOG_LENGTH];

  // Schedules next heartbeat.
  alarm(HEARTBEAT_INTERVAL);
  node = list.list;
  while (node) {
    sockfd = connectToNode(node);
    if (sockfd == -1) {
      // No connection.
      if (++node->nNoHeartbeat == HEARTBEAT_RETRY) {
        sprintf(log, "Server unreachable: %s. %s", inet_ntoa(node->addr),
            "Please check networks connection.");
        _log(log);
        node = dropNode(&list, node);
      }
    } else {
      // Connection established.
      rv = ping(sockfd, &status);
      sprintf(log, "PING %s.", inet_ntoa(node->addr));
      _log(log);
      while (rv != REDIS_OK || !status) {
        // No ping.
        if (++node->nNoPing == PING_RETRY) {
          sprintf(log, "Redis process unresponsive: %s. %s",
              inet_ntoa(node->addr),
              "Please restart Redis process on that machine.");
          _log(log);
          node = dropNode(&list, node);
          break;
        }
        usleep(PING_INTERVAL);
        rv = ping(sockfd, &status);
        sprintf(log, "PING %s.", inet_ntoa(node->addr));
        _log(log);
      }
      close(sockfd);
      if (rv == REDIS_OK && status) {
        // Pong.
        sprintf(log, "PONG from %s.", inet_ntoa(node->addr));
        _log(log);
      }
    }
    if (node) node = node->next;
  }
}

// Listens to client, sends them with the latest liveness snapshot.
//
// If a client connects to this config server and asks for latest liveness
// snapshot, this config server immediately sends back the sequence of
// serialized in_addr.
void _listenToClient() {
  int sockfd, newSockfd;
  int rv;
  int n;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t clilen;
  char log[MAX_LOG_LENGTH];

  // Creates a socket.
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    printf("config-server.c: %s %s\n", "Error listening to client.",
        "Failed to create socket.");
    return;
  }

  // Binds to port.
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(CONFIG_SERVER_PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  rv = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
  if (rv == -1) {
    printf("config-server.c: %s %s\n", "Error listening to client.",
        "Failed to bind to port.");
    return;
  }

  // Starts to listen.
  rv = listen(sockfd, 5);
  if (rv == -1) {
    printf("config-server.c: %s %s\n", "Error listening to client.",
        "Failed to start to listen.");
    return;
  }
  _log("Starts to listen to client...");

  // Accepts an incoming connection.
  while (1) {
    newSockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    //printf("[debug]config-server.c: sockfd = %d\n", newSockfd);  // debug
    if (newSockfd == -1) {
      // A -1 sockfd doesn't always mean error. As we have registered handler
      // for SIGALRM, this may be caused by that routine. In this case, errno
      // should be EINTR.
      if (errno == EINTR) continue;
      printf("config-server.c: %s %s\n", "Error listening to client.",
          "Failed to accept an incoming connection.");
      return;
    }

    // Sends back the snapshot.
    n = list.nNode * sizeof(in_addr);
    rv = write(newSockfd, snapshot, n);
    if (rv == -1) {
      printf("config-server.c: %s %s\n", "Error listening to client.",
          "Failed to send back snapshot.");
    } else if (rv != n) {
      printf("config-server.c: %s %s\n", "Failed to listen to client.",
          "Failed to send back snapshot.");
    } else {
      sprintf(log, "Snapshot of live Redis servers sent to %s.",
          inet_ntoa(cli_addr.sin_addr));
      _log(log);
    }
    close(newSockfd);
  }
}

// Loads information about live servers in the cluster from config file.
// Handler of SIGUSR1.
//
// Each line in the config file is a hostname/ip of a live server. Config
// server saves the corresponding address in the form of a in_addr struct.
void _loadConfigFile(int signum, siginfo_t *info, void *unused) {
  FILE *fin;
  char line[MAX_LINE_LENGTH];
  int rv;
  struct addrinfo hints, *result;
  in_addr *addr;

  // Frees all the memory pre-allocated by the consistent hashing map.
  clearMap(&list);

  // Adds live servers to the map according to the config file.
  fin = fopen(REDISSRV_CONFIG_FILE, "r");
  if (!fin) {
    printf("config-server.c: %s %s\n", "Failed to load config file.",
        "Config file for Redis servers not found.");
    return;
  }
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  rv = fscanf(fin, "%s", line);
  while (!feof(fin)) {
    rv = getaddrinfo(line, NULL, &hints, &result);
    if (!rv && result) {
      addr = &((sockaddr_in *) result->ai_addr)->sin_addr;
      memcpy(&snapshot[list.nNode * sizeof(in_addr)], addr, sizeof(in_addr));
      addNode(addr, &list);
    }
    freeaddrinfo(result);
    rv = fscanf(fin, "%s", line);
  }
  fclose(fin);
  _log("Config file loaded.");
}

// Registers signal handlers.
//
// SIGUSR1 -> _loadConfigFile. Afters an administrator modifies this file, he
// signals the config server process for a load.
// SIGALRM -> _heartbeat.
int _registerSignalHandlers() {
  int rv;
  struct sigaction sa;

  // SIGUSR1 -> _loadConfigFile.
  sa.sa_sigaction = _loadConfigFile;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  rv = sigaction(SIGUSR1, &sa, NULL);
  if (rv == -1) {
    printf("config-server.c: %s\n", "Error registering SIGUSR1 signal.");
    return CONFSRV_ERR;
  }

  // SIGALRM -> _heartbeat.
  sa.sa_sigaction = _heartbeat;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;

  rv = sigaction(SIGALRM, &sa, NULL);
  if (rv == -1) {
    printf("config-server.c: %s\n", "Error registering SIGALRM signal.");
    return CONFSRV_ERR;
  }

  _log("Signal handlers registered.");
  return CONFSRV_OK;
}

int main(int argc, char *argv[]) {
  int rv;

  // Registers signal handlers.
  rv = _registerSignalHandlers();
  if (rv == CONFSRV_ERR) {
    printf("config-server.c: %s\n", "Error registering signals.");
    return 1;
  } else if (rv == CONFSRV_FAILED) {
    printf("config-server.c: %s\n", "Failed to register signals.");
    return 0;
  }

  // Intializes Redis server information from config file.
  _loadConfigFile(0, NULL, NULL);

  // Starts up the alarm clock.
  alarm(HEARTBEAT_INTERVAL);

  // Starts to listen to client. Never returns.
  _listenToClient();

  // Never reachable when everything is fine.
  printf("config-server.c: %s\n", "Config server exits unexpectedly.");
  return 1;
}
