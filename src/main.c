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

static bool hasFinalProbePrinted(Probe *probes)
{
    for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    {
        if (probes[i].final && probes[i].printed)
            return true;
    }
    return false;
}

static void traceRoute(const struct sockaddr_in destination)
{
    Probe probes[DEFAULT_PROBES_NUMBER];
    initializeProbes(probes, DEFAULT_PROBES_NUMBER, destination);
    const int outboundSd = prepareSocketOrExitFailure(IPPROTO_UDP);

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
                if (isPrintableProbe(&probes[i]) && hasAllPreviousProbesPrinted(&probes[i], probes) &&
                    !hasFinalProbePrinted(probes))
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
