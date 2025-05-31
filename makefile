CC = g++
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = includes
CFLAGS = -I$(INCLUDE_DIR)

SERVER_OBJECT = $(BUILD_DIR)/server.o
CLIENT_OBJECT = $(BUILD_DIR)/client.o
PLAYER_OBJECT = $(BUILD_DIR)/player.o
ROOM_OBJECT = $(BUILD_DIR)/room.o

all: client.out server.out

client.out: $(BUILD_DIR) $(CLIENT_OBJECT)
	$(CC) $(CFLAGS) $(CLIENT_OBJECT) -o client.out

server.out: $(BUILD_DIR) $(SERVER_OBJECT) $(PLAYER_OBJECT) $(ROOM_OBJECT)
	$(CC) $(CFLAGS) $(SERVER_OBJECT) $(PLAYER_OBJECT) $(ROOM_OBJECT) -o server.out

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/server.o: $(SRC_DIR)/server.cpp $(INCLUDE_DIR)/server.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/server.cpp -o $(BUILD_DIR)/server.o

$(BUILD_DIR)/client.o: $(SRC_DIR)/client.cpp $(INCLUDE_DIR)/client.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/client.cpp -o $(BUILD_DIR)/client.o

$(BUILD_DIR)/player.o: $(SRC_DIR)/player.cpp $(INCLUDE_DIR)/player.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/player.cpp -o $(BUILD_DIR)/player.o

$(BUILD_DIR)/room.o: $(SRC_DIR)/room.cpp $(INCLUDE_DIR)/room.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/room.cpp -o $(BUILD_DIR)/room.o

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) *.o *.out
