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

static ProcessProbeResult processProbe(Probe *probe, const Sds sds) // returns nextTimeToProcess
{
    struct timeval nextTimeToProcess = (struct timeval){0, 0};

    // Expiration Management
    if (!isTimeNonZero(probe->timeReceived) && isTimeNonZero(probe->timeSent))
    {
        const struct timeval expirationTime = timeSum(probe->timeSent, DEFAULT_EXPIRATION_TIME);

        if (isTimeInOrder(timeOfDay(), expirationTime))
        {
            /* Normal scenario: future expiry time */
            nextTimeToProcess = expirationTime;
        }
        else
        {
            /* Else scenario: expired now */
            expireProbe(probe);
        }
    }
    // Send Probe
    const int sentNumber = !isTimeNonZero(probe->timeSent) ? sendProbe(probe, sds), 1 : 0;
    // Print Probe
    if (isTimeNonZero(probe->timeReceived) || probe->expired)
    {
        if (!probe->printed)
            printProbe(probe);
        probe->printed = true;
    }
    return (ProcessProbeResult){nextTimeToProcess, sentNumber};
}

static void receiveProbeResponses(Probe *probes, const struct timeval nextTimeToProcessProbes, const Sds sds)
{
    fd_set watchSds;
    FD_ZERO(&watchSds);
    FD_SET(sds.inBound, &watchSds);

    while (1)
    {
        struct timeval timeout = isTimeNonZero(nextTimeToProcessProbes)
                                     ? timeDifference(timeOfDay(), nextTimeToProcessProbes)
                                     : (struct timeval){0, 0};
        fd_set tempSet = watchSds; // Preserve the original set for each select call
        errno = 0;
        const int numberOfReadableSockets = select(sds.inBound + 1, &tempSet, NULL, NULL, &timeout);
        // printf("%d\n", errno);
        if (numberOfReadableSockets > 0)
        {
            char buffer[RESPONSE_SIZE_MAX];
            const ssize_t bytesReceived = recvfrom(sds.inBound, buffer, sizeof(buffer), 0, NULL, NULL);
            if (bytesReceived >= (ssize_t)RESPONSE_SIZE_MIN)
            {
                Probe receivedProbe = parseProbe(buffer, bytesReceived);
                Probe *probePointer = probePointerBySeq(probes, receivedProbe.seq);
                if (probePointer)
                {
                    probePointer->timeReceived = receivedProbe.timeReceived;
                    probePointer->final = receivedProbe.final;
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

static Sds prepareSockets()
{
    const int outBoundSd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    const int inBoundSd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    setRecverr(outBoundSd);
    setRecverr(inBoundSd);

    return (Sds){outBoundSd, inBoundSd};
}

static void traceRoute(const struct sockaddr_in destination)
{
    Probe probes[DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP];
    initializeProbes(probes, DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP, destination);
    const Sds sds = prepareSockets();

    while (!isDone(probes))
    {
        struct timeval nextTimeToProcessProbes = (struct timeval){0, 0};
        size_t probesSent = 0;
        for (int i = 0; i < DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP && probesSent < DEFAULT_SIMALTANIOUS_PROBES; i++)
        {
            const ProcessProbeResult result = processProbe(&probes[i], sds);
            if (isTimeNonZero(result.nextTimeToProcess))
                nextTimeToProcessProbes = isTimeNonZero(nextTimeToProcessProbes)
                                              ? timeMin(nextTimeToProcessProbes, result.nextTimeToProcess)
                                              : result.nextTimeToProcess;
            probesSent += result.sentNumber;
        }
        receiveProbeResponses(probes, nextTimeToProcessProbes, sds);
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
        printf("Host: %s\n", args.host);
        struct sockaddr_in destination = parseAddrOrExitFailure(args.host);
        printf("traceroute to %s (%s), %d hops max, %d byte packets\n", args.host, inet_ntoa(destination.sin_addr),
               DEFAULT_HOPS_MAX, DEFAULT_PACKET_SIZE_BYTES);
        traceRoute(destination);
    }
    else
    {
        printHelp();
    }
    return 0;
}
