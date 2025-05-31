#ifndef CLIENT_H
#define CLIENT_H

using namespace std;

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <vector>
#include <iostream>
#include <string>


#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024

const char* MAIN_SERVER = "Connect to main server";

#endif