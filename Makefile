all: server subscriber

server:
	g++ server.cpp  utils.h -g -Wall -std=c++11 -o server;

subscriber:
	g++ subscriber.cpp -g -Wall -std=c++11 -o subscriber;

clean:
	rm -rf server subscriber
