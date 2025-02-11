#include "all.h"

void setTtl(int socketFd, int ttl)
{
    static int currentTtl = 0;
    if (currentTtl != ttl && setsockopt(socketFd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        error("setsockopt IP_TTL");
    currentTtl = ttl;
}

void setRecverrOrExitFailure(int socketFd)
{
    int val = 1;
    if (setsockopt(socketFd, SOL_IP, IP_RECVERR, &val, sizeof(val)) < 0)
        error("setsockopt IP_RECVERR");
}

int prepareSocketOrExitFailure(const int protocol)
{
    const int sd = socket(AF_INET, protocol == IPPROTO_ICMP ? SOCK_RAW : SOCK_DGRAM, protocol);
    if (protocol == IPPROTO_UDP)
        setRecverrOrExitFailure(sd);
    if (sd < 0)
        error("socket");
    return sd;
}
