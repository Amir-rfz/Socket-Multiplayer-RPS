#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <map>
#include <algorithm>
#include <utility>
#include <memory>
#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <csignal>
#include <fcntl.h>

using namespace std;

typedef struct pollfd pollfd;

#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024

const char* SERVER_LAUNCHED = "Server Successfully Launched!\n";
const char* NEW_CONNECTION = "New player add to the game!\n";
const char* ASK_NAME = "Welcome to the game!\nPlease enter your name: ";
const char* CHOOSE_ROOM = "Choose a room number:\n";
const char* ROOM_FULL = "The selected room is full. Please choose another.\n";
const char* ROOM_WAIT = "Waiting for second player...\n";
const char* ROOM_GAME_START = "Choose your move in 10 sec \n1: rock \n2: paper \n3: scissors\n";
const char* MAIN_SERVER = "Connect to main server";

const int COUNTDOWN_TIME = 10;

#endif