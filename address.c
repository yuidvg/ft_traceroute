int equal_addr(const sockaddr_any *a, const sockaddr_any *b)
{

    if (!a->sa.sa_family)
        return 0;

    if (a->sa.sa_family != b->sa.sa_family)
        return 0;

    if (a->sa.sa_family == AF_INET6)
        return !memcmp(&a->sin6.sin6_addr, &b->sin6.sin6_addr,
                       sizeof(a->sin6.sin6_addr));
    else
        return !memcmp(&a->sin.sin_addr, &b->sin.sin_addr,
                       sizeof(a->sin.sin_addr));
    return 0; /*  not reached   */
}