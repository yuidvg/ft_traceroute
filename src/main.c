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
        size_t numberOfProbesOnItsWay = 0;
        while (firstProbeToProcessIndex < lastProbeToProcessIndex)
        {
            for (int i = 0; i + firstProbeToProcessIndex < lastProbeToProcessIndex &&
                               numberOfProbesOnItsWay < DEFAULT_SIMALTANIOUS_PROBES;
                 i++)
            {
                int probeIndex = i + firstProbeToProcessIndex;
                if (probes[probeIndex].timeSent.tv_sec == 0)
                {
                    sendProbe(&probes[probeIndex]);
                    numberOfProbesOnItsWay++;
                }
                else if (probes[probeIndex].timeReceived.tv_sec != 0)
                {
                    printProbe(&probes[probeIndex]);
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
