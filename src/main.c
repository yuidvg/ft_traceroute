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
        struct sockaddr_in addr = parseAddrOrExitFailure(args.host);

        printf("traceroute to %s (%s), %d hops max, %d byte packets\n", args.host, inet_ntoa(addr.sin_addr),
               DEFAULT_HOPS_MAX, DEFAULT_PACKET_SIZE_BYTES);

        Probe probes[DEFAULT_HOPS_MAX * DEFAULT_PROBES_PER_HOP];
        initializeProbes(probes, NUMBER_OF_ITEMS(probes));
        int firstProbeToProcessIndex = 0;
        int lastProbeToProcessIndex = NUMBER_OF_ITEMS(probes);

        fd_set readfds;
        struct timeval timeout;
        int max_sd;

        while (firstProbeToProcessIndex < lastProbeToProcessIndex)
        {
            size_t numberOfProbesOnItsWay = 0;
            struct timeval nextTimeToProcess;

            FD_ZERO(&readfds);
            max_sd = 0;

            for (int i = 0; i + firstProbeToProcessIndex < lastProbeToProcessIndex &&
                            numberOfProbesOnItsWay < DEFAULT_SIMALTANIOUS_PROBES;
                 i++)
            {
                // Expiracy management Of firstProbeToProcessIndex
                int probeIndex = i + firstProbeToProcessIndex;
                if (probeIndex == firstProbeToProcessIndex && !isTimeExist(probes[probeIndex].timeReceived) &&
                    isTimeExist(probes[probeIndex].timeSent))
                {
                    const struct timeval expirationTime = timeSum(probes[probeIndex].timeSent, DEFAULT_EXPIRATION_TIME);

                    if (isTimeExist(timeDifference(timeOfDay(), expirationTime)))
                    {
                        /* Normal scenario: future expiry time */
                        nextTimeToProcess = expirationTime;
                    }
                    else
                    {
                        /* Else scenario: expired now */
                        expireProbe(&probes[probeIndex]);
                    }
                }
                if (probes[probeIndex].timeSent.tv_sec == 0)
                {
                    sendProbe(&probes[probeIndex]);
                    numberOfProbesOnItsWay++;
                }
                else if (probes[probeIndex].timeReceived.tv_sec != 0)
                {
                    printProbe(&probes[probeIndex]);
                    probes[probeIndex].printed = true;
                }
                else
                {
                    FD_SET(probes[probeIndex].socketFd, &readfds);
                    if (probes[probeIndex].socketFd > max_sd)
                    {
                        max_sd = probes[probeIndex].socketFd;
                    }
                }
                numberOfProbesOnItsWay++;
            }

            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            const int numberOfReadableSokets = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
            if (numberOfReadableSokets < 0 && errno != EINTR)
                error("select error");

            for (int i = 0; i + firstProbeToProcessIndex < lastProbeToProcessIndex; i++)
            {
                int probeIndex = i + firstProbeToProcessIndex;
                if (FD_ISSET(probes[probeIndex].socketFd, &readfds))
                {
                    // Handle the probe response here
                    // For example, you can call a function to process the response
                    // processProbeResponse(&probes[probeIndex]);
                }
            }

            firstProbeToProcessIndex = getFirstProbeToProcessIndex(probes, NUMBER_OF_ITEMS(probes));
        }
    }
    else
    {
        printHelp();
    }
    return 0;
}
