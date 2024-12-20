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

static ProcessProbeResult processProbe(Probe *probe) // returns nextTimeToProcess
{
    struct timeval nextTimeToProcess;

    // Expiration Management
    if (!isTimeNonZero(probe->timeReceived) && isTimeNonZero(probe->timeSent))
    {
        const struct timeval expirationTime = timeSum(probe->timeSent, DEFAULT_EXPIRATION_TIME);

        if (isTimeNonZero(timeDifference(timeOfDay(), expirationTime)))
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
    const int sentNumber = !isTimeNonZero(probe->timeSent) ? sendProbe(probe), 1 : 0;
    // Print Probe
    if (isTimeNonZero(probe->timeReceived))
    {
        printProbe(probe);
        probe->printed = true;
    }
    return (ProcessProbeResult){nextTimeToProcess, sentNumber};
}

static void receiveProbeResponses(Probe *probes, const struct timeval nextTimeToProcessProbes)
{
    fd_set watchSds;
    int maxSd;
    FD_ZERO(&watchSds);
    for (int i = 0; i < DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP; i++)
    {
        if (probes[i].sd != -1)
        {
            maxSd = max(maxSd, probes[i].sd);
            FD_SET(probes[i].sd, &watchSds);
        }
    }
    struct timeval timeout = timeDifference(timeOfDay(), nextTimeToProcessProbes);
    const int numberOfReadableSokets = select(maxSd + 1, &watchSds, NULL, NULL, &timeout);
    if (numberOfReadableSokets < 0 && errno != EINTR)
        error("select error");

    for (int i = 0; i < DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP; i++)
    {
        if (FD_ISSET(probes[i].sd, &watchSds))
            receiveProbe(&probes[i]);
    }
}

static void traceRoute(const struct sockaddr_in destination)
{
    Probe probes[DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP];
    initializeProbes(probes, DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP, destination);

    while (!isDone(probes))
    {
        struct timeval nextTimeToProcessProbes = (struct timeval){0, 0};
        size_t probesSent = 0;
        for (int i = 0; i < DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP && probesSent < DEFAULT_SIMALTANIOUS_PROBES; i++)
        {
            const ProcessProbeResult result = processProbe(&probes[i]);
            if (isTimeNonZero(result.nextTimeToProcess))
                nextTimeToProcessProbes = isTimeNonZero(nextTimeToProcessProbes)
                                              ? timeMin(nextTimeToProcessProbes, result.nextTimeToProcess)
                                              : result.nextTimeToProcess;
            probesSent += result.sentNumber;
        }
        receiveProbeResponses(probes, nextTimeToProcessProbes);
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
