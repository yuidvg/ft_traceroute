#pragma once

#include "external.h"

typedef struct Probe
{
    int ttl;
    int seq;
    struct sockaddr_in destination;
    int socketFd;
    struct timeval timeSent;
    struct timeval timeReceived;
    bool final;
    char errorString[ERROR_STRING_SIZE_MAX];
} Probe;

typedef struct Args
{
    const char *host;
    const bool help;
} Args;