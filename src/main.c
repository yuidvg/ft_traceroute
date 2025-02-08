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
            // char *bufp = buf;
            char control[1024];
            // struct cmsghdr *cm;
            // double recv_time = 0;
            // struct sock_extended_err *ee = NULL;

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

static int prepareSocketOrExitFailure()
{
    const int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    setRecverr(sd);
    if (sd < 0)
        error("socket");
    return sd;
}

static void traceRoute(const struct sockaddr_in destination)
{
    Probe probes[DEFAULT_PROBES_NUMBER];
    initializeProbes(probes, DEFAULT_PROBES_NUMBER, destination);
    int sd = prepareSocketOrExitFailure();

    while (!isDone(probes))
    {
        struct timeval nextTimeToProcessProbes = (struct timeval){0, 0};
        // Send Probes
        for (int i = 0; i < DEFAULT_PROBES_NUMBER; i++)
        {
            if (!isTimeNonZero(probes[i].timeSent))
                sendProbe(&probes[i], sd);
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
        receiveProbeResponses(probes, nextTimeToProcessProbes, sd);
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
