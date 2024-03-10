#include "utils.h"

using namespace std;

struct UDP_packet {
    string payload;
	string type;
	string topic;
};

struct client {
    bool connection;
	bool SF;
    string id;
	vector<string> packets;
    int fd;
};

fd_set readfd, tmpfd;

// case if the udp packet's payload is integer
string get_int_payload(char buf[MAX_LEN]) {
    uint32_t number = *(uint32_t*)(buf + TOPIC_SIZE + 2);

    if (buf[TOPIC_SIZE + 1] == 1) {
        return "-" + std::to_string(ntohl(number));
    } else {
        return std::to_string(ntohl(number));
    }
}

// case if the udp packet's payload is short real
string get_short_real_payload(char buf[MAX_LEN]) {
    uint16_t number = *(uint16_t*)(buf + TOPIC_SIZE + 1);

    return std::to_string(static_cast<double>(ntohs(number)) / 100.0);
}

// case if the udp packet's payload is float
string get_float_payload(char buf[MAX_LEN]) {

    uint32_t number = *(uint32_t*)(buf + TOPIC_SIZE + 2);
    uint8_t power = *(uint8_t*)(buf + TOPIC_SIZE + 2 + sizeof(uint32_t));

    double result = static_cast<double>(ntohl(number)) / std::pow(10, power);

    if (buf[TOPIC_SIZE + 1] == 1) {
        return "-" + std::to_string(result);
    } else {
        return std::to_string(result);
    }
}

// case if the udp packet's payload is string
string get_string_payload(char buf[MAX_LEN]) {
    return string(buf + TOPIC_SIZE + 1);
}

//verify which type of packet we have
UDP_packet process_packet(int port, char buf[MAX_LEN], string udpIp) {
    UDP_packet packet;
    stringstream payload;

    switch(buf[TOPIC_SIZE]) {
        case PACKET_TYPE_INT:
            packet.payload = get_int_payload(buf);
            packet.type = "INT";
            break;
        case PACKET_TYPE_SHORT_REAL:
            packet.payload = get_short_real_payload(buf);
            packet.type = "SHORT_REAL";
            break;
        case PACKET_TYPE_FLOAT:
            packet.payload = get_float_payload(buf);
            packet.type = "FLOAT";
            break;
        case PACKET_TYPE_STRING:
            packet.payload = get_string_payload(buf);
            packet.type = "STRING";
            break;
        default:
            return packet;
    }

    buf[TOPIC_SIZE] = '\0';

	string topic_string = buf; // Convert the C-style string to a C++ string
    packet.topic = topic_string;

    return packet;
}

// initialization of the ID for the client
string id_init(char buf[MAX_LEN]) {
    string client_ID;
    client_ID = buf;

    return client_ID;
}

//intialization for a server_address
struct sockaddr_in server_address_init(int PORT) {
    struct sockaddr_in server_address;
    memset((char *) &server_address, 0, sizeof(server_address));
    server_address.sin_port        = htons((uint16_t)PORT);
	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;

    return server_address;
}

//initialization for client
struct client client_init(bool SF, std::unordered_map<int, string>::iterator client_connection) {
    struct client new_client;
    new_client.SF = SF;
    new_client.id = client_connection->second;

    return new_client;
}

// initialization for client
struct client new_client_init(int newfd, char buf[MAX_LEN]) {
    struct client new_client;
    new_client.fd = newfd;
    new_client.connection = true;
    new_client.id = buf;

    return new_client;
}

// print case for new client
void print_client(char buf[MAX_LEN], sockaddr_in client_adress) {
    std::cout << "New client " << buf; 
    std::cout << " connected from " << inet_ntoa(client_adress.sin_addr);
    std::cout << ":" << ntohs(client_adress.sin_port) << "." << std::endl;
}

// verify when file descriptor is STDIN the commands
// for exit we close the connection and announce the clients
void verify_stdin(char buf[MAX_LEN], std::unordered_map<int, string> connected_clients) {
    fgets(buf, MAX_LEN - 1, stdin);
    if (strcmp(buf, "exit\n") == 0) {
        for (auto& client : connected_clients) {
            strcpy(buf, "exit\n");

            int rc = send(client.first, buf, strlen(buf), 0);
            if (rc < 0) {
                DIE(rc, "FAILED receive udp");
            }

            close(client.first);
        }
    }
}

// UDP Socket
// verify if message from the sockets were received and 
// if the message  was send to the tcp client
bool receive_UPD(char buf[MAX_LEN], int UDP_sock,sockaddr_in server_address,
                std::unordered_map<std::string, std::vector<client>> topics,
                std::unordered_map<std::string, client> clients) {

    socklen_t socklen = sizeof(struct sockaddr_in);

    int rc = recvfrom(UDP_sock, buf, MAX_LEN, 0, (struct sockaddr *) &server_address, &socklen);
    if (rc < 0) {
        DIE(rc, "FAILED receive udp");
    }

	char udpIp[IP_LEN];
    inet_ntop(AF_INET, &(server_address.sin_addr), udpIp, IP_LEN);

    struct UDP_packet UPD_data = process_packet(server_address.sin_port, buf, udpIp);

    // message for client
    std::ostringstream stream;
    stream << udpIp << ":";
    stream << server_address.sin_port << " - ";
    stream << UPD_data.topic << " - " ;
    stream << UPD_data.type<< " - " ;
    stream << UPD_data.payload << std::endl;

    unordered_map<string, vector<client>>::iterator topic = topics.find(UPD_data.topic);
    if (topic == topics.end()) {
        return false;
    }

    // adds the message and sends it to the clients
    for (auto& iterator : topic->second) { 
        std::unordered_map<std::string, client>::iterator client_data = clients.find(iterator.id);

        if (!client_data->second.connection) {
            client_data->second.packets.push_back(stream.str());
        } else {
            rc = send(client_data->second.fd, stream.str().c_str(), stream.str().size(), 0);
            if (rc < 0) {
                DIE(rc, "FAILED send tcp");
            }   
        }
    }

    return true;
}

// new client
//checks if the clients already exists in our database
// if not the packets will be send to this new client
bool receive_new_client(char* buf, int TCP_sock, int &maxfd, sockaddr_in client_adress, socklen_t socklen,
                        std::unordered_map<std::string, client> &clients, 
                        std::unordered_map<int, std::string> &connected_clients) {
    int newfd = accept(TCP_sock, (struct sockaddr *) &client_adress, (socklen_t *) &socklen) ;
    if (newfd < 0) {
        DIE(newfd < 0, "FAILED receive TCP");
    }
                
    FD_SET(newfd, &readfd);
    maxfd = max(maxfd, newfd);

    memset(buf, 0, MAX_LEN);
    int rc = recv(newfd, buf, sizeof(buf), 0);

    string client_connection = id_init(buf);
                
    std::unordered_map<std::string, client>::iterator client_connect = clients.find(string(buf));

    if (client_connect != clients.end() && client_connect->second.connection == true ) {
        std::cout << "Client" << buf << "already connected."<< std::endl; 
                    
        memset(buf, 0, MAX_LEN);
        strcpy(buf, "id_exists\n");

        rc = send(newfd, buf, strlen(buf), 0);
        if (rc < 0) {
            DIE(rc, "FAILED send tcp");
        }
                    
        return false;
    }

    std::unordered_map<std::string, client>::iterator client_it = clients.find(buf);

    if (client_it == clients.end()) {
        struct client new_client = new_client_init(newfd, buf);
        clients[buf] = new_client;
    } else {
        client_it->second.fd = newfd;
        client_it->second.connection = true;
    }

    print_client(buf, client_adress);
    connected_clients[newfd] = client_connection;

    std::unordered_map<std::string, client>::iterator iterator = clients.find(client_connection);
    long unsigned int j = 0;

    while (j < iterator->second.packets.size()) {
        rc = send(newfd, iterator->second.packets[j].c_str(), iterator->second.packets[j].size(), 0);
        if (rc < 0) {
            DIE(rc, "FAILED send tcp");
        }

        j++;
    }
                
    iterator->second.packets.clear();
    return true;
}

// TCP already connected clients
// checks subscribe (the client want to subscribe to a new topic) 
//and unsubscribe commands (the client wants to unsusbscribe from a given topic)
void receive_TCP(char* buf, int& i, std::unordered_map<std::string, client>& clients,
                std::unordered_map<int, std::string>& connected_clients,
                std::unordered_map<std::string, std::vector<client>>& topics, fd_set& readfd) {
    memset(buf, 0, MAX_LEN);

    int rc = recv(i, buf, MAX_LEN, 0);
    if (rc < 0) {
        DIE(rc, "FAILED receive TCP");
    } else if (rc == 0) {
        if (connected_clients.count(i) != 0) {
            std::cout << "Client " << connected_clients[i] << " disconnected."<<std::endl;

            std::unordered_map<std::string, client>::iterator client = clients.find(connected_clients[i]);
            if (client != clients.end()) {
                client->second.connection = false;
            }
            connected_clients.erase(i);

            close(i);
            FD_CLR(i, &readfd);
        }
    }

    if (strncmp(buf, "unsubscribe", 11) == 0) { // pachet cu flag de unsubscribe
        std::istringstream stream(buf);
        std::string aux, topic_string;

        stream >> aux >> topic_string;

        std::unordered_map<std::string, std::vector<client>>::iterator topic = topics.find(topic_string);
        std::unordered_map<int, std::string>::iterator  client_connection = connected_clients.find(i);

        if (topic != topics.end() && client_connection != connected_clients.end()) {
            for (auto iterator = topic->second.begin(); iterator != topic->second.end(); ++iterator) {
                if (iterator->id == client_connection->second) {
                    topic->second.erase(iterator);
                    break;
                }
            }
        }
    } else if (strncmp(buf, "subscribe", 9) == 0) { // pachet cu flag de subscribe
        std::istringstream stream(buf);
        std::string aux, topic_string;
        int int_SF;
        bool SF;

        stream >> aux >> topic_string >> int_SF;

        SF = (int_SF == 1);

        int ok = 0;
        std::unordered_map<int, std::string>::iterator client_connection = connected_clients.find(i);
        std::unordered_map<std::string, std::vector<client>>::iterator topic = topics.find(topic_string);

        if (topic == topics.end()) {
            std::vector<client> new_clients;
            client new_client = client_init(SF, client_connection);
            new_clients.push_back(new_client);
            topics[topic_string] = new_clients;
        } else {
            for (auto& client : topic->second) {
                if (client.id == client_connection->second) {
                    ok = 1;
                    client.SF = SF;
                    break;
                }
            }

            if (ok == 0) {
                client new_client = client_init(SF, client_connection);
                topic->second.push_back(new_client);
            }
        }
    }
}


// verify different errors
void open_sockets(int TCP_sock,  int UDP_sock, sockaddr_in server_address) {
    if (TCP_sock < 0) {
        DIE(TCP_sock, "FAILED open tcp sock");
    }

    // disable nagle algorithm using TCP_NODELAY
    int nagle = 1;
    int rc = setsockopt(TCP_sock, IPPROTO_TCP, TCP_NODELAY, &nagle, sizeof(nagle));
    if (rc  == -1) {
        DIE(rc, "FAILED nagle");
    }

	
    rc = bind(TCP_sock, (struct sockaddr *) &server_address, sizeof(struct sockaddr));
    if (rc == -1) {
        DIE(rc, "FAILED tcp bind");
    }

    rc = listen(TCP_sock, MAX_CLIENTS);
    if (rc == -1) {
        DIE(rc, "FAILED tcp start listen");
    }

    if (UDP_sock < 0) {
        DIE(UDP_sock, "FAILED udp sock open");
    }

    rc = bind(UDP_sock, (struct sockaddr *) &server_address, sizeof(server_address));
    if (rc == -1) {
        DIE(rc, "FAILED udp bind");
    }
}

int main(int argc, char *argv[]) {
    string client_ID;
    unordered_map<int, std::string> connected_clients;
    unordered_map<string, vector<client>> topics;
    unordered_map<string, client> clients;

    int PORT = atoi(argv[1]); 

    if (PORT == 0) {
        DIE(PORT, "FAILED port");
    } 

    if (argc <= 1) {
        DIE(argc, "FAILED usage");
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    struct sockaddr_in server_address = server_address_init(PORT);
    struct sockaddr_in client_adress;

    int TCP_sock = socket(AF_INET, SOCK_STREAM, 0);
    int UDP_sock = socket(AF_INET, SOCK_DGRAM, 0);

    open_sockets(TCP_sock, UDP_sock, server_address);

	FD_ZERO(&readfd);
    FD_SET(TCP_sock, &readfd);
	FD_SET(UDP_sock, &readfd);
    FD_SET(0, &readfd);

	FD_ZERO(&tmpfd);

    int maxfd = max(TCP_sock, UDP_sock);
    socklen_t socklen = sizeof(struct sockaddr_in);

    while (true) {
        tmpfd = readfd;
        int rc = select(maxfd + 1, &tmpfd, NULL, NULL, NULL);
        char buf[MAX_LEN];

        if (rc == -1) {
            DIE(rc, "FAILED select");
        }

        if (FD_ISSET(STDIN, &tmpfd)) {
            verify_stdin(buf, connected_clients);
        }

        //goes through every file descriptor
        for (int i = 1; i <= maxfd; i++) {
            if (FD_ISSET(i, &tmpfd)) {
                if (i == UDP_sock) {
                    if(receive_UPD(buf, UDP_sock, server_address, topics,clients) == false) {
                        continue;
                    }
                } else if (i == TCP_sock) {
                    if(receive_new_client(buf, TCP_sock, maxfd, client_adress,socklen,clients, connected_clients) == false)
                            continue;
                } else {
                    receive_TCP(buf, i, clients, connected_clients, topics, readfd);
                    
                }
            }
        }
    }

    close(UDP_sock);
    close(TCP_sock);

    return 0;
}
