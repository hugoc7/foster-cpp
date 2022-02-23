#pragma once

#include "SDL_net.h"
#include <iostream>
#include <string>
#include <cassert>
#include "Containers.h"
#include "Packing.h"

#define MAX_MESSAGES 100
#define MAX_16_BIT_VALUE 65535
#define MAX_8_BIT_VALUE 255
#define MAX_32_BIT_VALUE 4294967295
#define MAX_TCP_PACKET_SIZE 1200 //it is not the MTU though I put the same value ^^
#define TCP_SLEEP_DURATION 10 //sleep duration of a TCP sevrer or client in its main loop
#define INITIAL_TCP_BUFFER_SIZE 30u
#define MAX_TCP_SOCKETS 16
#define NO_BUFFER -1

#define THREAD_CLOSING_DELAY 20

std::string getIpAdressAsString(Uint32 ip);
