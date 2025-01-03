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
        ft_memset(probes[i].errorString, '\0', sizeof(probes[i].errorString));
    }
}

ssize_t sendProbe(Probe *probe, const Sds sds)
{
    const uint16_t sequenceOnNetwork = htons(probe->seq);
    struct sockaddr_in destination = probe->destination;
    setTtl(sds.outBound, probe->ttl);
    errno = 0;
    const ssize_t res = sendto(sds.outBound, &sequenceOnNetwork, sizeof(uint16_t), 0, (struct sockaddr *)&destination,
                               sizeof(destination));
    // if (errno != 0)
    //     printf("errno: %d\n", errno);
    if (errno == EHOSTUNREACH)
    {
        const ssize_t res = sendto(sds.outBound, &sequenceOnNetwork, sizeof(uint16_t), 0,
                                   (struct sockaddr *)&destination, sizeof(destination));
        if (res != -1)
            probe->timeSent = timeOfDay();
        return res;
    }
    else if (res != -1)
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
        printf(" %s\t\t%ld.%03ld ms", inet_ntoa(probe->destination.sin_addr),
               timeDifference(probe->timeSent, probe->timeReceived).tv_sec,
               timeDifference(probe->timeSent, probe->timeReceived).tv_usec / 1000);
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
            if (bytesReceived >= (ssize_t)(ipHeaderSize + sizeof(struct icmphdr) + originalIpHeaderSize +
                                           sizeof(struct udphdr) + sizeof(uint16_t)))
            {
                const struct udphdr *udpHeader =
                    (struct udphdr *)(buffer + ipHeaderSize + sizeof(struct icmphdr) + originalIpHeaderSize);
                const uint16_t *sequenceOnNetwork = (uint16_t *)(buffer + ipHeaderSize + sizeof(struct icmphdr) +
                                                                 originalIpHeaderSize + sizeof(struct udphdr));
                probe.seq = ntohs(*sequenceOnNetwork);
                probe.timeReceived = timeOfDay();
                probe.destination.sin_addr.s_addr = ipHeader->saddr;
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
                (void)udpHeader;
            }
        }
    }
    return probe;
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
