#include "all.h"

static void parseResponse(Probe *probePointer, int type, int code, int info)
{
    char *str = NULL;
    char buf[sizeof(probePointer->errorString)];

    if (type == ICMP_TIME_EXCEEDED)
    {
        if (code == ICMP_EXC_TTL)
            return;
    }
    else if (type == ICMP_DEST_UNREACH)
    {

        switch (code)
        {
        case ICMP_UNREACH_NET:
        case ICMP_UNREACH_NET_UNKNOWN:
        case ICMP_UNREACH_ISOLATED:
        case ICMP_UNREACH_TOSNET:
            str = "!N";
            break;

        case ICMP_UNREACH_HOST:
        case ICMP_UNREACH_HOST_UNKNOWN:
        case ICMP_UNREACH_TOSHOST:
            str = "!H";
            break;

        case ICMP_UNREACH_NET_PROHIB:
        case ICMP_UNREACH_HOST_PROHIB:
        case ICMP_UNREACH_FILTER_PROHIB:
            str = "!X";
            break;

        case ICMP_UNREACH_PORT:
            /*  dest host is reached   */
            str = "";
            break;

        case ICMP_UNREACH_PROTOCOL:
            str = "!P";
            break;

        case ICMP_UNREACH_NEEDFRAG:
            snprintf(buf, sizeof(buf), "!F-%d", info);
            str = buf;
            break;

        case ICMP_UNREACH_SRCFAIL:
            str = "!S";
            break;

        case ICMP_UNREACH_HOST_PRECEDENCE:
            str = "!V";
            break;

        case ICMP_UNREACH_PRECEDENCE_CUTOFF:
            str = "!C";
            break;

        default:
            snprintf(buf, sizeof(buf), "!<%u>", code);
            str = buf;
            break;
        }
    }

    if (!str)
    {
        snprintf(buf, sizeof(buf), "!<%u-%u>", type, code);
        str = buf;
    }

    if (*str)
    {
        snprintf(probePointer->errorString, sizeof(probePointer->errorString), "%s", str);
    }

    probePointer->final = 1;

    return;
}

void parseOffender(struct msghdr msg, Probe *probePointer)
{
    struct cmsghdr *cmsg;
    struct sock_extended_err *ee = NULL;
    int recv_ttl = 0;
    struct timeval timeReceived = {0};

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        void *ptr = CMSG_DATA(cmsg);

        if (cmsg->cmsg_level == SOL_SOCKET)
        {

            if (cmsg->cmsg_type == SO_TIMESTAMP)
            {
                timeReceived = *(struct timeval *)ptr;
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
                    return;

                /*  dgram icmp sockets might return extra things...  */
                if (ee->ee_origin == SO_EE_ORIGIN_ICMP &&
                    (ee->ee_type == ICMP_SOURCE_QUENCH || ee->ee_type == ICMP_REDIRECT))
                    return;
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
                    return;
            }
        }
    }
    (void)recv_ttl;
    if (isTimeNonZero(timeReceived))
    {
        probePointer->timeReceived = timeReceived;
    }
    if (ee->ee_type == ICMP_DEST_UNREACH)
        probePointer->final = true;
    if (ee->ee_type == ICMP_TIME_EXCEEDED)
        probePointer->final = false;
    const struct sockaddr *offender = SO_EE_OFFENDER(ee);
    char hostname[NI_MAXHOST];

    getnameinfo(offender, sizeof(offender) + sizeof(struct sockaddr_in), hostname, sizeof(hostname), NULL, 0, 0);
    // printf("Offender: %s\n", hostname);
    probePointer->destination = *(struct sockaddr_in *)offender;
    parseResponse(probePointer, ee->ee_type, ee->ee_code, ee->ee_info);
}
