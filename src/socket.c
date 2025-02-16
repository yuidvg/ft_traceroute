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

int prepareSocketOrExitFailure(const int protocol, struct sockaddr_in destination)
{
    const int sd = socket(AF_INET, protocol == IPPROTO_ICMP ? SOCK_RAW : SOCK_DGRAM, protocol);
    if (sd < 0)
        error("socket");
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = 0;
    // if (bind(sd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    //     error("bind");
    {
        int i = IP_PMTUDISC_DONT;
        if (setsockopt(sd, SOL_IP, IP_MTU_DISCOVER, &i, sizeof(i)) < 0)
            error("setsockopt IP_MTU_DISCOVER");
    }
    {
        int n = 1;
        if (setsockopt(sd, SOL_SOCKET, SO_TIMESTAMP, &n, sizeof(n)) < 0)
            error("setsockopt SO_TIMESTAMP");
    }
    {
        int n = 1;
        if (setsockopt(sd, SOL_IP, IP_RECVTTL, &n, sizeof(n)) < 0)
            error("setsockopt IP_RECVTTL");
    }

    if (connect(sd, (struct sockaddr *)&destination, sizeof(destination)) < 0)
        error("connect");
    if (protocol == IPPROTO_UDP)
        setRecverrOrExitFailure(sd);
    return sd;
}
