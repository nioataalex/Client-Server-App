Copyright Nioata Alexandra 322CA 2023

Homework 2 - PCOM -TCP and UDP client-server application for
message management

The purpose of the assignment is to simulate a socket-based client-server application and have greater knowledge of TCP and UDP.
There must be a single server in the program that connects the platform's two different sorts of clients, TCP clients and UDP clients.
UDP clients send messages suggested by the defined protocol to the server and TCP clients connect to the server and show on the screen messages received from it.
In essence, the server acts as a go-between the two different kinds of clients.

I used 2 structures:
- UDP_packet: in which I keep what the types of data sent by UDP to the server look like (payload, type, topic)
- client: that keeps data for each client

Server:
- check if the arguments given from the command line are ok;
- initialize the sockets (TCP and UDP) and test their functionality;
- I need to check these four situations:
    - file descriptor on STDIN, in which case I exit
    - descriptor file for UDP, new UDP packet, which is examined to see if it has to
                    be sent to any clients and, if so, is placed in each client's packet queue.
    - using the TCP file descriptor, I examine the circumstances in which a TCP client receives a subscribe, unsubscribe topic or disconnect.
    - file descriptor for new client, if the client's id already exists as another client, we do not add the new client to the client queue.

Subscriber:
- check if the arguments given from the command line are ok;
- initialize the socket (TCP) and test their functionality;
- send the client Id to the server;
- I need to check these two situations:
    - file descriptor on STDIN: check the commands from the command line subscribe, unsubscribe, exit and check if they are valid, 
                send appropriate messages
    - file descriptor on TCP: read data up to newline, '\n', which also delimits packages, check if the client id already exists
             or if it has to exit

Disclaimer:
- I used C++;
- the homework was interesting, but I encountered problems with the checker, and my implementation is not as I would have liked;
- I should have allocated more time for this;

Useful resources:
1. PCOM Labs(6, 7, 8);
2. https://www.techtarget.com/searchnetworking/definition/TCP
3. https://www.youtube.com/watch?v=5Ywo4X0ujaU&ab_channel=LearnTCPIP
4. https://www.youtube.com/watch?v=qKqlv9i4aQA&ab_channel=LearnTCPIP

