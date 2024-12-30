#include "all.h"

void setTtl(int socketFd, int ttl)
{
    static int currentTtl = 0;
    if (currentTtl != ttl && setsockopt(socketFd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        error("setsockopt IP_TTL");
    currentTtl = ttl;
}

void setRecverr(int socketFd)
{
    int val = 1;
    if (setsockopt(socketFd, SOL_IP, IP_RECVERR, &val, sizeof(val)) < 0)
        error("setsockopt IP_RECVERR");
}

