#include "all.h"

void setTtl(int socketFd, int ttl)
{
    setsockopt(socketFd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
}

void setRecverr(int socketFd)
{
    int val = 1;
    if (setsockopt(socketFd, SOL_IP, IP_RECVERR, &val, sizeof(val)) < 0)
        error("setsockopt IP_RECVERR");
}

int sendToAddress(int socketFd, struct sockaddr_in addr)
{
    char data[DATA_SIZE];
    for (size_t i = 0; i < DATA_SIZE; i++)
        data[i] = 0x40 + (i & 0x3f);
    const ssize_t res = sendto(socketFd, data, DATA_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
    // printf("res: %d\n", errno);
    if (res != -1)
    {
        return res;
    }
    else
    {
        if (errno == ENOBUFS || errno == EAGAIN)
            return res;
        if (errno == EMSGSIZE)
            return 0;
        error("send");
    }
    return res;
}
