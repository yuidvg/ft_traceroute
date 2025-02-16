#include "all.h"

struct sockaddr_in parseOffender(struct msghdr msg)
{
    struct cmsghdr *cmsg;
    struct sock_extended_err *ee = NULL;
    int recv_ttl = 0;
    double recv_time = 0;

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        void *ptr = CMSG_DATA(cmsg);

        if (cmsg->cmsg_level == SOL_SOCKET)
        {

            if (cmsg->cmsg_type == SO_TIMESTAMP)
            {
                struct timeval *tv = (struct timeval *)ptr;

                recv_time = tv->tv_sec + tv->tv_usec / 1000000.;
            }
        }
        else if (cmsg->cmsg_level == SOL_IP)
        {

            if (cmsg->cmsg_type == IP_TTL)
                recv_ttl = *((int *)ptr);
            else if (cmsg->cmsg_type == IP_RECVERR)
            {

                ee = (struct sock_extended_err *)ptr;

                if (ee->ee_origin != SO_EE_ORIGIN_ICMP && ee->ee_origin != SO_EE_ORIGIN_LOCAL)
                    return (struct sockaddr_in){0};

                /*  dgram icmp sockets might return extra things...  */
                if (ee->ee_origin == SO_EE_ORIGIN_ICMP &&
                    (ee->ee_type == ICMP_SOURCE_QUENCH || ee->ee_type == ICMP_REDIRECT))
                    return (struct sockaddr_in){0};
            }
        }
        else if (cmsg->cmsg_level == SOL_IPV6)
        {

            if (cmsg->cmsg_type == IPV6_HOPLIMIT)
                recv_ttl = *((int *)ptr);
            else if (cmsg->cmsg_type == IPV6_RECVERR)
            {

                ee = (struct sock_extended_err *)ptr;

                if (ee->ee_origin != SO_EE_ORIGIN_ICMP6 && ee->ee_origin != SO_EE_ORIGIN_LOCAL)
                    return (struct sockaddr_in){0};
            }
        }
    }
    (void)recv_ttl;
    (void)recv_time;
    const struct sockaddr *offender = SO_EE_OFFENDER(ee);
    char hostname[NI_MAXHOST];
    getnameinfo(offender, sizeof(offender) + sizeof(struct sockaddr_in), hostname, sizeof(hostname), NULL, 0, 0);
    // printf("Offender: %s\n", hostname);
    return *(struct sockaddr_in *)offender;
}
