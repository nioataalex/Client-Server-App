#include "utils.h"

using namespace std;

int main(int argc, char *argv[]) {

    int PORT = atoi(argv[3]);

    fd_set clientfd, tmpfd;
    struct sockaddr_in server_address;
    char buf[MAX_LEN];
    
    //verify errors
    if (PORT == 0) {
        DIE(PORT, "FAILED port incorrect");
    }
    
    if (argc <= 3) {
        DIE(argc, "FAILED usage subscriber");
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int TCP_sock, rc;
    TCP_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (TCP_sock < 0) {
        DIE(TCP_sock, "FAILED open sock");
    }

    server_address.sin_port   = htons(PORT);
    server_address.sin_family = AF_INET;
    rc = inet_aton(argv[2], &server_address.sin_addr);

    setsockopt(TCP_sock, IPPROTO_TCP, TCP_NODELAY, (int *)1, sizeof(int));
    rc = connect(TCP_sock, (struct sockaddr *)&server_address, sizeof(server_address));
    if (rc < 0) {
        DIE(rc, "FAILED sock connect");
    }

    memset(buf, 0, MAX_LEN);
    strcpy(buf, argv[1]);

    rc = send(TCP_sock, buf, strlen(buf), 0);
    if (rc < 0) {
        DIE(rc, "FAILED id send");
    }
    
    FD_ZERO(&clientfd);
    FD_SET(0, &clientfd);
    FD_SET(TCP_sock, &clientfd);

    FD_ZERO(&tmpfd);

    while (true) {
        tmpfd = clientfd;

        rc = select(TCP_sock + 1, &tmpfd, NULL, NULL, NULL);
        if (rc < 0) {
            DIE(rc, "FAILED select sock");
        }

        // case if the file descriptor is STDIN
        if (FD_ISSET(STDIN, &tmpfd)) {
            memset(buf, 0, MAX_LEN);
            fgets(buf, sizeof(buf), stdin);

            if (strncmp(buf, "exit", 4) == 0) {
                std::cout << "Exit" << std::endl;
                break;
            } else if (strncmp(buf, "subscribe", 9) == 0) {
                rc = send(TCP_sock, buf, strlen(buf), 0);
                if (rc < 0) {
                    DIE(rc, "FAILED sending cmd");
                }
                std::cout << "Subscribed to topic."<< std::endl;
            } else if (strncmp(buf, "unsubscribe", 11) == 0) {
                rc = send(TCP_sock, buf, strlen(buf), 0);
                if (rc < 0) {
                    DIE(rc, "FAILED sending cmd");
                }
                std::cout << "Unsubscribed from topic."<< std::endl;
            }
        }

        // case if file descriptor is TCP
        if (FD_ISSET(TCP_sock, &tmpfd)) {
            size_t buf_len = 0;
            memset(buf, 0, MAX_LEN);

            while (recv(TCP_sock, &buf[buf_len], 1, 0)  ==  1 && buf_len < MAX_LEN)  {
                if ('\n' == buf[buf_len] && buf_len > 0 ) {
                    break;
                }
                buf_len++;
            }

            if (strncmp(buf, "exit", 4) == 0 ) {
                std::cout << "Exit" <<std::endl;
                break;
            } else if (strncmp(buf, "id_exists", 9) == 0 ) {
                std::cerr << "Id already exists" << std::endl;
                break;
            }
            std::cout << buf;
        }
    }

    close(TCP_sock);
    return  0;
}
