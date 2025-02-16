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

struct sockaddr_in parseAddrOrExitFailure(const char *host)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int errcode;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    if ((errcode = getaddrinfo(host, NULL, &hints, &res)) != 0)
    {
        fprintf(stderr, "ft_ping: unknown host\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = *(struct sockaddr_in *)res->ai_addr;

    freeaddrinfo(res);
    return addr;
}
