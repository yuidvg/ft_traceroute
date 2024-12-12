#pragma once

#include "external.h"

typedef struct Probe
{
    const int socketFd;
    const struct sockaddr_in destination;
    const int ttl;
    const int seq;
} Probe;

typedef struct ProbeResult
{
    const Probe probe;
    const int rtt;
} ProbeResult;