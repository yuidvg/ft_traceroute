#include "all.h"

void initializeProbes(Probe *probes, size_t count, const struct sockaddr_in destination)
{
    for (size_t i = 0; i < count; i++)
    {
        probes[i].ttl = i / DEFAULT_PROBES_PER_HOP + 1;
        probes[i].destination = destination;
        probes[i].seq = i + 1;
        probes[i].final = false;
        probes[i].expired = false;
        probes[i].printed = false;
        probes[i].timeSent = (struct timeval){0};
        probes[i].timeReceived = (struct timeval){0};
        probes[i].sd = prepareSocketOrExitFailure(IPPROTO_UDP, destination);
        ft_memset(probes[i].errorString, '\0', sizeof(probes[i].errorString));
    }
}

ssize_t sendProbe(Probe *probe, const int sd)
{
    (void)sd;
    // const uint16_t sequenceOnNetwork = htons(probe->seq);
    const uint64_t sequenceOnNetwork = ~0ULL;
    setTtl(probe->sd, probe->ttl);
    errno = 0;
    const ssize_t res = send(probe->sd, &sequenceOnNetwork, sizeof(sequenceOnNetwork), 0);
    if (res != -1)
    {
        probe->timeSent = timeOfDay();
        return res;
    }
    else if (errno == ENOBUFS || errno == EAGAIN)
        return res;
    else if (errno == EMSGSIZE || errno == EHOSTUNREACH)
        return 0;
    else
        error("send");
    return res;
}

void printProbe(Probe *probe)
{
    printf("%2d. ", probe->ttl);

    if (!probe->expired)
    {
        printf(" %s\t%.1f ms", inet_ntoa(probe->destination.sin_addr),
               timeValInMiliseconds(timeDifference(probe->timeSent, probe->timeReceived)));
    }
    else
    {
        printf(" *");
    }
    if (probe->errorString[0] != '\0')
        printf(" %s", probe->errorString);
    printf("\n");
    probe->printed = true;
}

bool isPrintableProbe(Probe *probe)
{
    return probe->printed == false && (probe->expired == true || isTimeNonZero(probe->timeReceived));
}

bool hasAllPreviousProbesPrinted(Probe *probe, Probe *probes)
{
    for (size_t i = 0; i < (size_t)probe->seq - 1; i++)
    {
        if (probes[i].printed == false)
            return false;
    }
    return true;
}

Probe parseProbe(const char *buffer, ssize_t bytesReceived)
{
    Probe probe;
    probe.seq = 0;
    const struct iphdr *ipHeader = (struct iphdr *)buffer;
    const uint32_t ipHeaderSize = ipHeader->ihl * 4;
    if (bytesReceived >= ntohs(ipHeader->tot_len) && ipHeader->protocol == IPPROTO_ICMP)
    {
        const struct icmphdr *icmpHeader = (struct icmphdr *)(buffer + ipHeaderSize);
        if (icmpHeader->type == ICMP_DEST_UNREACH || icmpHeader->type == ICMP_TIME_EXCEEDED)
        {
            const struct iphdr *originalIpHeader = (struct iphdr *)(buffer + ipHeaderSize + sizeof(struct icmphdr));
            const uint32_t originalIpHeaderSize = originalIpHeader->ihl * 4;
            if (bytesReceived >=
                (ssize_t)(ipHeaderSize + sizeof(struct icmphdr) + originalIpHeaderSize + sizeof(struct udphdr)))
            {
                const struct udphdr *udpHeader =
                    (struct udphdr *)(buffer + ipHeaderSize + sizeof(struct icmphdr) + originalIpHeaderSize);
                (void)udpHeader;
                // const uint16_t destPortOnNetwork = ntohs(udpHeader->dest);
                // probe.seq = destPortOnNetwork - DEFAULT_PORT + 1;
                probe.timeReceived = timeOfDay();
                probe.destination.sin_addr.s_addr = ipHeader->saddr;

                // // For Debug, print the contents of the packet received
                // printf("==============================================\n");
                // printf("Packet received:\n");

                // // IP Header details
                // printf("IP Header:\n");
                // printf("  Version: %d\n", ipHeader->version);
                // printf("  IHL: %d words (%d bytes)\n", ipHeader->ihl, ipHeader->ihl * 4);
                // printf("  Type of Service: %d\n", ipHeader->tos);
                // printf("  Total Length: %d\n", ntohs(ipHeader->tot_len));
                // printf("  Identification: %d\n", ntohs(ipHeader->id));
                // printf("  Flags/Fragment Offset: %d\n", ntohs(ipHeader->frag_off));
                // printf("  TTL: %d\n", ipHeader->ttl);
                // printf("  Protocol: %d\n", ipHeader->protocol);
                // printf("  Header Checksum: 0x%x\n", ntohs(ipHeader->check));
                // printf("  Source IP: %s\n", inet_ntoa(*(struct in_addr *)&ipHeader->saddr));
                // printf("  Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ipHeader->daddr));

                // // ICMP Header details
                // printf("ICMP Header:\n");
                // printf("  Type: %d\n", icmpHeader->type);
                // printf("  Code: %d\n", icmpHeader->code);
                // printf("  Checksum: 0x%x\n", ntohs(icmpHeader->checksum));

                // // Original IP Header details
                // printf("Original IP Header:\n");
                // printf("  Version: %d\n", originalIpHeader->version);
                // printf("  IHL: %d words (%d bytes)\n", originalIpHeader->ihl, originalIpHeader->ihl * 4);
                // printf("  Type of Service: %d\n", originalIpHeader->tos);
                // printf("  Total Length: %d\n", ntohs(originalIpHeader->tot_len));
                // printf("  Identification: %d\n", ntohs(originalIpHeader->id));
                // printf("  Flags/Fragment Offset: %d\n", ntohs(originalIpHeader->frag_off));
                // printf("  TTL: %d\n", originalIpHeader->ttl);
                // printf("  Protocol: %d\n", originalIpHeader->protocol);
                // printf("  Header Checksum: 0x%x\n", ntohs(originalIpHeader->check));
                // printf("  Source IP: %s\n", inet_ntoa(*(struct in_addr *)&originalIpHeader->saddr));
                // printf("  Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&originalIpHeader->daddr));

                // // UDP Header details
                // printf("UDP Header:\n");
                // printf("  Source Port: %d\n", ntohs(udpHeader->source));
                // printf("  Destination Port: %d\n", ntohs(udpHeader->dest));
                // printf("  Length: %d\n", ntohs(udpHeader->len));
                // printf("  Checksum: 0x%x\n", ntohs(udpHeader->check));

                // printf("==============================================\n");

                if (icmpHeader->type == ICMP_DEST_UNREACH)
                {
                    probe.final = true;
                    switch (icmpHeader->code)
                    {
                    case ICMP_UNREACH_NET:
                    case ICMP_UNREACH_NET_UNKNOWN:
                    case ICMP_UNREACH_ISOLATED:
                    case ICMP_UNREACH_TOSNET:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!N");
                        break;

                    case ICMP_UNREACH_HOST:
                    case ICMP_UNREACH_HOST_UNKNOWN:
                    case ICMP_UNREACH_TOSHOST:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!H");
                        break;

                    case ICMP_UNREACH_NET_PROHIB:
                    case ICMP_UNREACH_HOST_PROHIB:
                    case ICMP_UNREACH_FILTER_PROHIB:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!X");
                        break;

                    case ICMP_UNREACH_PORT:
                        break;

                    case ICMP_UNREACH_PROTOCOL:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!P");
                        break;

                    case ICMP_UNREACH_NEEDFRAG:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!F-%d", icmpHeader->un.frag.mtu);
                        break;

                    case ICMP_UNREACH_SRCFAIL:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!S");
                        break;

                    case ICMP_UNREACH_HOST_PRECEDENCE:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!V");
                        break;

                    case ICMP_UNREACH_PRECEDENCE_CUTOFF:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!C");
                        break;

                    default:
                        snprintf(probe.errorString, sizeof(probe.errorString), "!<%u>", icmpHeader->code);
                        break;
                    }
                }
                else if (icmpHeader->type == ICMP_TIME_EXCEEDED)
                {
                    probe.final = false;
                }
            }
        }
    }
    return probe;
}

void receiveProbeResponses(Probe *probes, const struct timeval nextTimeToProcessProbes, const int sd)
{
    (void)sd;
    fd_set watchSds;
    FD_ZERO(&watchSds);
    for (size_t i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    {
        FD_SET(probes[i].sd, &watchSds);
    }

    while (1)
    {
        struct timeval timeout = isTimeNonZero(nextTimeToProcessProbes)
                                     ? timeDifference(timeOfDay(), nextTimeToProcessProbes)
                                     : (struct timeval){0, 0};
        fd_set tempSet = watchSds; // Preserve the original set for each select call
        errno = 0;
        const int numberOfReadableSockets = select(probes[DEFAULT_PROBES_NUMBER - 1].sd + 1, &tempSet, NULL, NULL, &timeout);
        if (numberOfReadableSockets > 0)
        {
            for (size_t i = 0; i < DEFAULT_PROBES_NUMBER; i++)
            {
                int currentSd = probes[i].sd;
                if (FD_ISSET(currentSd, &tempSet))
                {

                    char buffer[RESPONSE_SIZE_MAX];
                    struct iovec iov;
                    iov.iov_base = buffer;
                    iov.iov_len = sizeof(buffer);

                    struct msghdr msg;
                    memset(&msg, 0, sizeof(msg));

                    struct sockaddr_in srcAddr;
                    msg.msg_name = &srcAddr;
                    msg.msg_namelen = sizeof(srcAddr);

                    msg.msg_iov = &iov;
                    msg.msg_iovlen = 1;

                    char control[1024];
                    memset(control, 0, sizeof(control));
                    msg.msg_control = control;
                    msg.msg_controllen = sizeof(control);

                    const ssize_t bytesReceived = recvmsg(currentSd, &msg, MSG_ERRQUEUE);
                    if (bytesReceived >= 0)
                    {
                        Probe *probePointer = probePointerBySd(probes, currentSd);
                        if (probePointer)
                        {
                            probePointer->timeReceived = timeOfDay();
                            parseOffender(msg, probePointer);
                        }
                    }
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

int getFirstProbeToProcessIndex(Probe *probes, size_t numberOfProbes)
{
    for (size_t i = 0; i < numberOfProbes; i++)
    {
        if (probes[i].timeSent.tv_sec == 0)
            return i;
    }
    return -1;
}

Probe *probePointerBySeq(Probe *probes, uint64_t seq)
{
    for (size_t i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    {
        if (probes[i].seq == (int)seq)
            return &probes[i];
    }
    return NULL;
}

Probe *probePointerBySd(Probe *probes, int sd)
{
    for (size_t i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    {
        if (probes[i].sd == sd)
            return &probes[i];
    }
    return NULL;
}

bool isDone(Probe *probes)
{
    bool isAllProbesPrinted = true;
    for (size_t i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    {
        if (probes[i].final)
        {
            bool isAllProbesFromDestinationPrinted = true;
            for (size_t j = (probes[i].ttl - 1) * DEFAULT_PROBES_PER_HOP;
                 j < (size_t)probes[i].ttl * DEFAULT_PROBES_PER_HOP; j++)
            {
                if (!probes[j].printed)
                    isAllProbesFromDestinationPrinted = false;
            }
            if (isAllProbesFromDestinationPrinted)
                return true;
        }
        if (!probes[i].printed)
            isAllProbesPrinted = false;
    }
    return isAllProbesPrinted;
}
