#include "all.h"

// static Probe *udp_check_reply(int socketFd, int err, struct sockaddr_in *from, char *buf, size_t len)
// {
//     Probe *pb;

//     pb = probeBySocketFd(socketFd);
//     if (!pb)
//         return NULL;

//     if (pb->seq != from->sin_port)
//         return NULL;

//     if (!err)
//         // pb->final = 1;

//     return pb;
// }

static Args parseArgs(int argc, char *argv[])
{
    char *host = NULL;
    bool help = false;

    for (int i = 1; i < argc; ++i)
    {
        if (ft_strcmp(argv[i], "--help") == 0)
        {
            help = true;
        }
        else
        {
            if (host != NULL)
            {
                fprintf(stderr, "Error: Too many arguments\n");
                exit(1);
            }
            else
                host = argv[i];
        }
    }
    return (Args){host, help};
}

static void receiveProbeResponses(Probe *probes, const struct timeval nextTimeToProcessProbes, const int sd)
{
    fd_set watchSds;
    FD_ZERO(&watchSds);
    FD_SET(sd, &watchSds);

    while (1)
    {
        struct timeval timeout = isTimeNonZero(nextTimeToProcessProbes)
                                     ? timeDifference(timeOfDay(), nextTimeToProcessProbes)
                                     : (struct timeval){0, 0};
        fd_set tempSet = watchSds; // Preserve the original set for each select call
        errno = 0;
        const int numberOfReadableSockets = select(sd + 1, &tempSet, NULL, NULL, &timeout);
        if (numberOfReadableSockets > 0)
        {
            struct msghdr msg;
            struct sockaddr_in from;
            struct iovec iov;
            char buf[RESPONSE_SIZE_MAX]; /*  min mtu for ipv6 ( >= 576 for ipv4)  */
            char control[1024];

            memset(&msg, 0, sizeof(msg));
            msg.msg_name = &from;
            msg.msg_namelen = sizeof(from);
            msg.msg_control = control;
            msg.msg_controllen = sizeof(control);
            iov.iov_base = buf;
            iov.iov_len = sizeof(buf);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;

            const ssize_t bytesReceived = recvmsg(sd, &msg, MSG_ERRQUEUE);
            fprintf(stderr, "errno: %d\n", errno);
            struct cmsghdr *cmsg;
            for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg))
            {
                if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR)
                {
                    struct sock_extended_err *serr = (struct sock_extended_err *)CMSG_DATA(cmsg);

                    printf("Received error:\n");
                    printf("  ee_errno  : %u\n", serr->ee_errno);
                    printf("  ee_origin : %u\n", serr->ee_origin);
                    printf("  ee_type   : %u\n", serr->ee_type);
                    printf("  ee_code   : %u\n", serr->ee_code);
                    printf("  ee_info   : %u\n", serr->ee_info);
                    printf("  ee_data   : %u\n", serr->ee_data);

                    // Obtain the original packet from ancillary data.
                    unsigned char *orig_packet = (unsigned char *)serr + sizeof(struct sock_extended_err);

                    // Parse the IP header from the original packet.
                    struct iphdr *iph = (struct iphdr *)orig_packet;
                    char ip_str[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &iph->saddr, ip_str, sizeof(ip_str));
                    printf("Original packet source IP: %s\n", ip_str);

                    // Calculate IP header length and locate the UDP header.
                    int ip_header_length = iph->ihl * 4;
                    struct udphdr *udph = (struct udphdr *)(orig_packet + ip_header_length);
                    printf("Original UDP source port: %u\n", ntohs(udph->source));
                    printf("Original UDP dest port  : %u\n", ntohs(udph->dest));

                    // Correctly obtain the original UDP payload from the original packet data.
                    uint16_t udpPayload;
                    memcpy(&udpPayload, orig_packet + ip_header_length + sizeof(struct udphdr), sizeof(udpPayload));
                    printf("Original UDP payload: %u\n", ntohs(udpPayload));
                }
            }
            if (bytesReceived >= (ssize_t)RESPONSE_SIZE_MIN)
            {
                Probe receivedProbe = parseProbe(iov.iov_base, bytesReceived);
                Probe *probePointer = probePointerBySeq(probes, receivedProbe.seq);
                if (probePointer)
                {
                    probePointer->timeReceived = receivedProbe.timeReceived;
                    probePointer->final = receivedProbe.final;
                    probePointer->destination.sin_addr.s_addr = receivedProbe.destination.sin_addr.s_addr;
                    snprintf(probePointer->errorString, ERROR_STRING_SIZE_MAX + 1, "%s", receivedProbe.errorString);
                }
            }
        }
        else if (numberOfReadableSockets == 0)
        {
            break;
        }
        else if (numberOfReadableSockets < 0)
        {
            if (errno == EINTR)
                continue;
            else
                error("select error");
        }
    }
}

static int prepareSocketOrExitFailure(const int protocol)
{
    const int sd = socket(AF_INET, protocol == IPPROTO_ICMP ? SOCK_RAW : SOCK_DGRAM, protocol);
    if (protocol == IPPROTO_UDP)
        setRecverrOrExitFailure(sd);
    if (sd < 0)
        error("socket");
    return sd;
}

static void traceRoute(const struct sockaddr_in destination)
{
    Probe probes[DEFAULT_PROBES_NUMBER];
    initializeProbes(probes, DEFAULT_PROBES_NUMBER, destination);
    const int outboundSd = prepareSocketOrExitFailure(IPPROTO_UDP);
    // const int inboundSd = prepareSocketOrExitFailure(IPPROTO_ICMP);

    while (!isDone(probes))
    {
        struct timeval nextTimeToProcessProbes = (struct timeval){0, 0};
        // Send Probes
        for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
        {
            if (!isTimeNonZero(probes[i].timeSent))
                sendProbe(&probes[i], outboundSd);
        }
        // Timeout Management
        for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
        {
            if (!isTimeNonZero(probes[i].timeReceived) && isTimeNonZero(probes[i].timeSent))
            {
                const struct timeval expirationTime = timeSum(probes[i].timeSent, DEFAULT_EXPIRATION_TIME);
                if (isTimeInOrder(timeOfDay(), expirationTime))
                {
                    /* Normal scenario: future expiry time */
                    if (isTimeNonZero(nextTimeToProcessProbes))
                        nextTimeToProcessProbes = isTimeNonZero(nextTimeToProcessProbes)
                                                      ? timeMin(nextTimeToProcessProbes, expirationTime)
                                                      : expirationTime;
                }
                else
                {
                    /* Else scenario: expired now */
                    probes[i].expired = true;
                }
            }
        }
        // Print Probe
        for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
        {
            if (isTimeNonZero(probes[i].timeReceived) || probes[i].expired)
            {
                if (isPrintableProbe(&probes[i]) && hasAllPreviousProbesPrinted(&probes[i], probes))
                    printProbe(&probes[i]);
            }
        }
        receiveProbeResponses(probes, nextTimeToProcessProbes, outboundSd);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        Args args = parseArgs(argc, argv);

        if (args.help)
        {
            printHelp();
            return 0;
        }

        if (args.host == NULL)
        {
            fprintf(stderr, "Specify \"host\" missing argument.\n");
            printHelp();
            return 1;
        }
        struct sockaddr_in destination = parseAddrOrExitFailure(args.host);
        traceRoute(destination);
    }
    else
    {
        printHelp();
    }
    return 0;
}
