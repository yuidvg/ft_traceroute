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
            char buffer[RESPONSE_SIZE_MAX];
            const ssize_t bytesReceived = recvfrom(sd, buffer, sizeof(buffer), 0, NULL, NULL);
            if (bytesReceived >= (ssize_t)RESPONSE_SIZE_MIN)
            {
                Probe receivedProbe = parseProbe(buffer, bytesReceived);
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
    // if (protocol == IPPROTO_ICMP)
    //     setRecverrOrExitFailure(sd);
    if (sd < 0)
        error("socket");
    return sd;
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
    const int inboundSd = prepareSocketOrExitFailure(IPPROTO_ICMP);

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
                if (isPrintableProbe(&probes[i]) && hasAllPreviousProbesPrinted(&probes[i], probes) && !hasFinalProbePrinted(probes))
                    printProbe(&probes[i]);
            }
        }
        receiveProbeResponses(probes, nextTimeToProcessProbes, inboundSd);
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
