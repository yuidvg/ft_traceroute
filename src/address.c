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
    const struct addrinfo hints = {
        .ai_family = AF_INET,
    };
    struct addrinfo *res;

    const int status = getaddrinfo(host, NULL, &hints, &res);
    if (status == 0 && res && res->ai_family == AF_INET)
    {
        struct sockaddr_in addr = *(struct sockaddr_in *)res->ai_addr;
        freeaddrinfo(res);
        addr.sin_port = htons(DEFAULT_PORT);
        return addr;
    }
    else
    {
        // freeaddrinfo(res);
        fprintf(stderr, "%s: %s\n", host, gai_strerror(status));
        exit(EXIT_FAILURE);
    }
}
