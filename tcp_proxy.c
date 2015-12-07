#include "tcp_proxy.h"

struct conn {
  int client_sock;
  int upstream_sock;
  pthread_t sender;
  pthread_t receiver;
  struct sockaddr_in *address;
};


struct listener {
  struct sockaddr_in *address;
  int port;
};


int
main(int argc, char *argv[])
{
  struct sockaddr_in *address_http;
  int opt, i, num_ports;

  char *ip_addr = NULL, *host = NULL;
  char* ports_array[MAX_PORTS];
  pthread_t threads[MAX_PORTS];
  char ports_string[2048] = "80,443";
  struct listener *listener;

  struct option opts[] = {
    { "ip", required_argument, NULL, 'i' },
    { "host", required_argument, NULL, 'h' },
    { "ports", required_argument, NULL, 'p' },
		{ NULL,  0,  NULL, 0 }
  };

  /* Ignore broken pipes */
  signal(SIGPIPE, SIG_IGN);

  while ((opt = getopt_long(argc, argv, "i:h:p:", opts, NULL)) != -1) {
    switch(opt) {
      case 'i':
        ip_addr = optarg;
        break;
      case 'h':
        host = optarg;
        break;
      case 'p':
        strcpy(ports_string, optarg);
        break;
      default: print_usage(argv[0]);
    }
  }

  if (argc < 2) {
    print_usage(argv[0]);
  }

  if (ip_addr != NULL && host != NULL) {
    printf("You can't specify both ip and host\n");
    return 1;
  }

  if (ip_addr == NULL && host == NULL) {
    printf("You need to specify either ip or host\n");
    return 1;
  }

  debug("Starting server");

  char *stringp = ports_string;
  for(num_ports = 0; num_ports < MAX_PORTS && stringp; num_ports++) {
    const char *delim = ",";
    char *token = strsep(&stringp, delim);
    ports_array[num_ports] = token;
  }

  if (ip_addr) {
    address_http = addr_from_ip(ip_addr);
  }

  if (host) {
    address_http = addr_from_host(host);
  }

  for (i = 0; i < num_ports; i++) {
    int port = atoi(ports_array[i]);
    debug("Starting listener thread for port %d", port);
    listener = malloc(sizeof(struct listener));
    listener->address = malloc(sizeof(struct sockaddr_in));
    listener->port = port;
    memcpy((void *)listener->address, (void *)address_http, sizeof(struct sockaddr_in));
    listener->address->sin_port=htons(port);
    pthread_create(&threads[i], NULL, listen_conn, listener);
  }

  for (i = 0; i < num_ports; i++) {
    pthread_join(threads[i], NULL);
  }

	return 0;
}


void
print_address(struct sockaddr_in* p)
{
  char ipstr[INET6_ADDRSTRLEN];
  struct in_addr *addr = &(p->sin_addr);
  inet_ntop(AF_INET, addr, ipstr, sizeof ipstr);
}

struct sockaddr_in* 
addr_from_host(char host[])
{
	struct addrinfo hints, *res, *p;
	int status;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((status = getaddrinfo(host, NULL, &hints, &res) != 0)) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		die("getaddrinfo failed");
	}

	for(p=res; p!=NULL; p=p->ai_next) {
    print_address((struct sockaddr_in *)p->ai_addr);
	}

	return (struct sockaddr_in *)res->ai_addr;
}

struct sockaddr_in*
addr_from_ip(char ip[])
{
	struct sockaddr_in *res;

  res = malloc(sizeof(struct sockaddr_in));
  memset(res, 0, sizeof(struct sockaddr_in));

  inet_aton(ip, &(res->sin_addr));
  res->sin_family = AF_INET;

  return res;
}

void
sock_setreuse(int fd, int reuse)
{
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
		die("Failed to set SO_REUSEADDR");
}

void
sock_settimeout(int sockfd, int timeout)
{
  struct timeval tv;

  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
}

int
listen_to_port(int port)
{
	char sport[100];
	struct addrinfo hints, *res;
	int sockfd;

	sprintf(sport, "%d", port);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo(NULL, sport, &hints, &res);

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		die("socket creation failed");
	}

	sock_setreuse(sockfd, 1);
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		die("bind failed");
	}

	listen(sockfd, 5);

	return sockfd;
}

void *
sender(void *args)
{
  struct conn *session;
  int read_size;
  char message[MAX_MSG_SIZE];

  session = (struct conn*) args;

  while((read_size = recv(session->client_sock, message, MAX_MSG_SIZE, 0)) > 0 )
  {
    send(session->upstream_sock , message , read_size, 0);

    memset(message, 0, MAX_MSG_SIZE);
  }

  if (errno != 0) {
    printf("sender: %d %s\n", errno, strerror(errno));
  }

  return NULL;
}

void *
receiver(void *args)
{
  struct conn *session;
  int read_size;
  char message[MAX_MSG_SIZE];

  session = (struct conn*) args;

  while((read_size = recv(session->upstream_sock, message, MAX_MSG_SIZE, 0)) > 0 )
  {
    send(session->client_sock , message , read_size, 0);

    memset(message, 0, MAX_MSG_SIZE);
  }

  if (errno != 0) {
    log_errno("receiver");
  }

  return NULL;
}

void *
client_handler(void *args)
{
  struct conn *session;
  struct sockaddr_in *p;

  session = (struct conn*)args;
  p = session->address;

  if ((session->upstream_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    die("Couldn't open socket for connect");
  }

  sock_settimeout(session->upstream_sock, 60);
  sock_settimeout(session->client_sock, 60);

  if (connect(session->upstream_sock, (struct sockaddr *)p, sizeof(struct sockaddr_in))) {
    die("Couldn't connect");
  }

  if (pthread_create( &session->sender, NULL, sender, session)) {
    die("Can't create sender thread");
  }

  if (pthread_create( &session->receiver, NULL, receiver, session)) {
    die("Can't create receiver thread");
  }

  pthread_join(session->receiver, NULL);
  pthread_join(session->sender, NULL);

  close(session->client_sock);
  close(session->upstream_sock);
  free(session);
  return NULL;
}


void
accept_connections(int socketfd, struct sockaddr_in *address)
{
  int client_sock;
  struct sockaddr_storage *client_addr;
  socklen_t addr_size;
  struct conn *session;
  pthread_t thread_id;

  client_addr = malloc(sizeof(struct sockaddr_storage));
  addr_size = sizeof(client_addr);

  while ((client_sock = accept(socketfd, (struct sockaddr *)client_addr, &addr_size)))
  {
    session = malloc(sizeof(struct conn));
    session->client_sock=client_sock;
    session->address=address;
    if (pthread_create( &thread_id, NULL, client_handler, session)) {
      die("Couldn't create thread");
    }
    pthread_detach(thread_id);
  }
}


void *
listen_conn(void *args)
{
  struct listener* listener = (struct listener*)args;

	int socket_http = listen_to_port(listener->port);

	accept_connections(socket_http, listener->address);

  return NULL;
}


void
print_usage(char* name)
{
  printf("Usage: %s [--ip IP_ADDRESS] [--host HOST] [--ports PORT1,PORT2]\n", name);
  exit(1);
}


