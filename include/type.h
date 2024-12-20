#pragma once

#include "external.h"

typedef struct Probe
{
    int ttl;
    int seq;
    struct sockaddr_in destination;
    int sd;
    struct timeval timeSent;
    struct timeval timeReceived;
    bool final;
    char errorString[ERROR_STRING_SIZE_MAX + 1];
    bool expired;
    bool printed;
} Probe;

typedef struct Args
{
    const char *host;
    const bool help;
} Args;

typedef struct ProcessProbeResult
{
    struct timeval nextTimeToProcess;
    int sentNumber;
} ProcessProbeResult;
