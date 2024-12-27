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
        ft_memset(probes[i].errorString, '\0', sizeof(probes[i].errorString));
    }
}

void sendProbe(Probe *probe, const Sds sds)
{
    setTtl(sds.outBound, probe->ttl);
    if (sendToAddress(sds.outBound, probe->destination) < 0)
    {
        close(sds.outBound);
        probe->timeSent = (struct timeval){0, 0};
        return;
    }
    probe->timeSent = timeOfDay();
    return;
}

void printProbe(Probe *probe)
{
    if (probe->seq % DEFAULT_PROBES_PER_HOP == 1)
        printf("%2d ", probe->ttl);
    if (probe->errorString[0] == '\0')
    {
        if (!probe->expired)
        {
            if (probe->seq % DEFAULT_PROBES_PER_HOP == 1)
                printf(" %s (%s)  %ld.%03ld ms", inet_ntoa(probe->destination.sin_addr),
                       inet_ntoa(probe->destination.sin_addr), probe->timeReceived.tv_sec,
                       probe->timeReceived.tv_usec / 1000);
            else
                printf(" %ld.%03ld ms", probe->timeReceived.tv_sec, probe->timeReceived.tv_usec / 1000);
        }
        else
        {
            printf(" *");
        }
    }
    else
        printf("[Error: %s]", probe->errorString);
    if (probe->seq % DEFAULT_PROBES_PER_HOP == 0)
        printf("\n");
}

Probe parseProbe(const char *buffer, ssize_t bytesReceived)
{
    Probe probe;
    const struct iphdr *ipHeader = (struct iphdr *)buffer;
    const uint32_t ipHeaderSize = ipHeader->ihl * 4;
    if (bytesReceived < (ssize_t)(ipHeaderSize + sizeof(struct icmphdr) + sizeof(uint64_t)))
    {
        snprintf(probe.errorString, ERROR_STRING_SIZE_MAX, "Invalid packet size. Too small: %ld", bytesReceived);
        return probe;
    }
    if (ipHeader->protocol == IPPROTO_ICMP)
    {
        const struct icmphdr *icmpHeader = (struct icmphdr *)(buffer + ipHeaderSize);
        const struct iphdr *originalIpHeader = (struct iphdr *)(buffer + ipHeaderSize + sizeof(struct icmphdr));
        const uint32_t originalIpHeaderSize = originalIpHeader->ihl * 4;
        const uint64_t *seq = (uint64_t *)(buffer + ipHeaderSize + sizeof(struct icmphdr) + originalIpHeaderSize);
        probe.seq = *seq;
        if (icmpHeader->type == ICMP_DEST_UNREACH || icmpHeader->type == ICMP_TIME_EXCEEDED)
        {
            probe.timeReceived = timeOfDay();
            if (icmpHeader->type == ICMP_DEST_UNREACH)
            {
                probe.final = true;
            }
            else if (icmpHeader->type == ICMP_TIME_EXCEEDED)
            {
                probe.final = false;
            }
        }
        else
        {
            snprintf(probe.errorString, ERROR_STRING_SIZE_MAX, "Unexpected ICMP type: %d", icmpHeader->type);
        }
        probe.timeReceived = timeOfDay();
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

void expireProbe(Probe *probe)
{
    probe->expired = true;
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
