#include "all.h"

int equal_addr(const struct sockaddr_in *a, const struct sockaddr_in *b)
{
    const struct sockaddr *sa = (const struct sockaddr *)a;
    const struct sockaddr *sb = (const struct sockaddr *)b;

    if (!sa->sa_family)
        return 0;

    if (sa->sa_family != sb->sa_family)
        return 0;

    if (sa->sa_family == AF_INET6)
    {
        const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)a;
        const struct sockaddr_in6 *b6 = (const struct sockaddr_in6 *)b;
        return !ft_memcmp(&a6->sin6_addr, &b6->sin6_addr, sizeof(a6->sin6_addr));
    }
    else
        return !ft_memcmp(&a->sin_addr, &b->sin_addr, sizeof(a->sin_addr));
}

struct sockaddr_in parseAddr(char *host)
{
    struct addrinfo hints, *res, *p;
    struct sockaddr_in addr;
    int status;

    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use AF_INET for IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host, NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        if (p->ai_family == AF_INET)
        {
            addr = *(struct sockaddr_in *)p->ai_addr;
            break;
        }
    }

    freeaddrinfo(res);

    return addr;
}
