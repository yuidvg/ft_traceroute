#pragma once

#include "values.h"
#include "external.h"

typedef struct
{
    struct icmphdr icmpHeader;
    uint8_t data[PAYLOAD_SIZE_MAX]; // Payload data
    size_t dataLen; // Data length
} IcmpEchoRequest;


union common_sockaddr {
	struct sockaddr sa;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
};
typedef union common_sockaddr sockaddr_any;

typedef struct Probe
{
    int ttl;
    uint16_t seq;
    struct sockaddr_in destination;
    struct timeval timeSent;
    struct timeval timeReceived;
    bool final;
    char errorString[ERROR_STRING_SIZE_MAX + 1];
    bool expired;
    bool printed;
    int sd;
    sockaddr_any offender;
} Probe;

typedef struct Args
{
    const char *host;
    const bool help;
} Args;

typedef struct Sds
{
    const int outBound;
    const int inBound;
} Sds;

typedef struct ProcessProbeResult
{
    struct timeval nextTimeToProcess;
    int sentNumber;
} ProcessProbeResult;

typedef struct ParseProbeResult
{
    bool success;
    Probe probe;
} ParseProbeResult;
