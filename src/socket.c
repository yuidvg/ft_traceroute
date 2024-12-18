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

int sendToAddress(int socketFd, const void *data, size_t len, const struct sockaddr_in addr)
{
    const ssize_t res = sendto(socketFd, data, len, 0, &addr.sin_addr, sizeof(addr));

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
}
