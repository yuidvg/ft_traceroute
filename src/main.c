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

// static bool hasFinalProbePrinted(Probe *probes)
// {
//     for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
//     {
//         if (probes[i].final && probes[i].printed)
//             return true;
//     }
//     return false;
// }

static void traceRoute(const struct sockaddr_in destination)
{
    // Probe probes[DEFAULT_PROBES_NUMBER];
    // initializeProbes(probes, DEFAULT_PROBES_NUMBER, destination);

    int rawSockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (rawSockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int on = 1;
    if (setsockopt(rawSockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
    {
        error("setsockopt IP_HDRINCL");
    }

    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = 0; // Ephemeral port

    if (bind(rawSockfd, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0)
        error("bind");

    const IcmpEchoRequest icmpEchoRequest = constructIcmpEchoRequest(getpid(), 0, "ff", 32, 0);
    const size_t icmpPacketLen = ICMP_ECHO_REQUEST_HEADER_SIZE + icmpEchoRequest.dataLen;
    const size_t ipHeaderLen = sizeof(struct ip);
    const size_t packetLen = ipHeaderLen + icmpPacketLen;

    char packet[packetLen];

    /* Manually construct the IP header */
    struct ip *ipHdr = (struct ip *)packet;
    ipHdr->ip_v = 4;
    ipHdr->ip_hl = ipHeaderLen >> 2; // Header length (typically 5)
    ipHdr->ip_tos = 0;
    ipHdr->ip_len = htons(packetLen);
    ipHdr->ip_id = htons(getpid() & 0xFFFF);
    ipHdr->ip_off = 0;
    ipHdr->ip_ttl = 1; // Set TTL correctly
    ipHdr->ip_p = IPPROTO_ICMP;

    // Fix the source IP: get the actual local IP chosen by the kernel.
    struct sockaddr_in actualLocalAddr;
    socklen_t addrLen = sizeof(actualLocalAddr);
    if (getsockname(rawSockfd, (struct sockaddr *)&actualLocalAddr, &addrLen) < 0)
        error("getsockname");
    ipHdr->ip_src = actualLocalAddr.sin_addr;

    ipHdr->ip_dst = destination.sin_addr;
    ipHdr->ip_sum = 0;

    /* Compute the IP header checksum */
    {
        unsigned long sum = 0;
        unsigned short *ptr = (unsigned short *)ipHdr;
        int nwords = ipHeaderLen / 2;
        for (int i = 0; i < nwords; i++)
            sum += ptr[i];
        while (sum >> 16)
            sum = (sum & 0xFFFF) + (sum >> 16);
        ipHdr->ip_sum = ~sum;
    }

    /* Encode the ICMP echo request right after the IP header */
    char *icmpSection = packet + ipHeaderLen;
    encodeIcmpEchoRequest(icmpEchoRequest, icmpSection);

    sendto(rawSockfd, packet, packetLen, 0,
           (struct sockaddr *)&destination, sizeof(destination));
    // while (!isDone(probes))
    // {
    //     struct timeval nextTimeToProcessProbes = (struct timeval){0, 0};
    //     // Send Probes
    //     for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    //     {
    //         if (!isTimeNonZero(probes[i].timeSent))
    //             sendProbe(&probes[i]);
    //     }
    //     // Timeout Management
    //     for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    //     {
    //         if (!isTimeNonZero(probes[i].timeReceived) && isTimeNonZero(probes[i].timeSent))
    //         {
    //             const struct timeval expirationTime = timeSum(probes[i].timeSent, DEFAULT_EXPIRATION_TIME);
    //             if (isTimeInOrder(timeOfDay(), expirationTime))
    //             {
    //                 /* Normal scenario: future expiry time */
    //                 if (isTimeNonZero(nextTimeToProcessProbes))
    //                     nextTimeToProcessProbes = isTimeNonZero(nextTimeToProcessProbes)
    //                                                   ? timeMin(nextTimeToProcessProbes, expirationTime)
    //                                                   : expirationTime;
    //             }
    //             else
    //             {
    //                 /* Else scenario: expired now */
    //                 probes[i].expired = true;
    //             }
    //         }
    //     }
    //     // Print Probe
    //     for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    //     {
    //         if (isTimeNonZero(probes[i].timeReceived) || probes[i].expired)
    //         {
    //             if (isPrintableProbe(&probes[i]) && hasAllPreviousProbesPrinted(&probes[i], probes) &&
    //                 !hasFinalProbePrinted(probes))
    //                 printProbe(&probes[i]);
    //         }
    //     }
    //     receiveProbeResponses(probes, nextTimeToProcessProbes);
    // }
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
