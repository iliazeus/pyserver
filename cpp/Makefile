.PHONY: clean

all: ./cat_server ./client

./cat_server: ./daemon_cli.h ./cat_server.cpp
	g++ ./cat_server.cpp -o ./cat_server

./client: ./daemon_cli.h ./client.cpp
	g++ ./client.cpp -o ./client

clean:
	rm -rf ./cat_server ./client
