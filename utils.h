#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <math.h>

#define MAX_LEN 1500
#define STDIN 0
#define TOPIC_SIZE 50
#define MAX_CLIENTS 50
#define PACKET_TYPE_INT 0
#define PACKET_TYPE_SHORT_REAL 1
#define PACKET_TYPE_FLOAT 2
#define PACKET_TYPE_STRING 3
#define IP_LEN 16



#define DIE(assertion, call_description) \
	do { \
		if ((assertion)) { \
			fprintf(stderr, "%d %s \n", __LINE__, (call_description)); \
			perror(""); \
			exit(1); \
		} \
	} while (0)
