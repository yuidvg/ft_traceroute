#include "all.h"

void initializeProbes(Probe *probes, size_t count, const struct sockaddr_in destination)
{
    for (size_t i = 0; i < count; i++)
    {
        probes[i].ttl = i / DEFAULT_PROBES_PER_HOP + 1;
        probes[i].destination = destination;
        probes[i].seq = i + 1;
        probes[i].sd = -1;
        probes[i].final = false;
        probes[i].expired = false;
        probes[i].printed = false;
        ft_memset(probes[i].errorString, '\0', sizeof(probes[i].errorString));
    }
}

void sendProbe(Probe *probe)
{
    int af = probe->destination.sin_family;

    const int socketFd = socket(af, SOCK_DGRAM, IPPROTO_UDP);
    if (socketFd < 0)
    {
        const size_t errorStringLen = ft_strlcpy(probe->errorString, strerror(errno), ERROR_STRING_SIZE_MAX);
        probe->errorString[errorStringLen] = '\0';
        return;
    }
    setTtl(socketFd, probe->ttl);
    if (connect(socketFd, (struct sockaddr *)&probe->destination, sizeof(probe->destination)) < 0)
        error(strerror(errno));
    setRecverr(socketFd);
    probe->timeSent = timeOfDay();
    if (sendToAddress(socketFd, probe->destination) < 0)
    {
        close(socketFd);
        probe->timeSent = (struct timeval){0, 0};
        return;
    }
    probe->sd = socketFd;
    return;
}

void printProbe(Probe *probe)
{
    if (probe->errorString[0] == '\0')
    {
        if (!probe->expired)
        {
            printf(" %d  %s (%s)  %ld.%03ld ms\n", probe->seq, inet_ntoa(probe->destination.sin_addr), inet_ntoa(probe->destination.sin_addr), probe->timeReceived.tv_sec, probe->timeReceived.tv_usec / 1000);
        }
        else
        {
            printf("*\n");
        }
    }
    else
        printf("Error: %s\n", probe->errorString);
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
    close(probe->sd);
    probe->sd = -1;
    probe->expired = true;
}

void receiveProbe(Probe *probe)
{
    char buffer[sizeof(struct iphdr) + sizeof(struct icmphdr)];
    const ssize_t bytesReceived = recvfrom(probe->sd, buffer, sizeof(buffer), 0, NULL, NULL);
    if (bytesReceived > (ssize_t)sizeof(struct icmphdr))
    {
        struct iphdr *ipHeader = (struct iphdr *)buffer;
        (void)ipHeader;
        struct icmphdr *icmpHeader = (struct icmphdr *)(buffer + sizeof(struct iphdr));
        probe->timeReceived = timeOfDay();
        if (icmpHeader->type == ICMP_DEST_UNREACH)
        {
            probe->final = true;
        }
        else if (icmpHeader->type == ICMP_TIME_EXCEEDED)
        {
            probe->final = false;
        }
        else
        {
            snprintf(probe->errorString, ERROR_STRING_SIZE_MAX, "Unexpected ICMP type: %d", icmpHeader->type);
        }
    }
    else
    {
        snprintf(probe->errorString, ERROR_STRING_SIZE_MAX, "No response from %s",
                 inet_ntoa(probe->destination.sin_addr));
    }
}

bool isDone(Probe *probes)
{
    bool isAllProbesPrinted = true;
    for (size_t i = 0; i < DEFAULT_PROBES_NUMBER; i++)
    {
        if (probes[i].final)
        {
            bool isAllProbesFromDestinationPrinted = true;
            for (size_t j = (probes[i].ttl - 1) * DEFAULT_PROBES_PER_HOP; j < (size_t)probes[i].ttl * DEFAULT_PROBES_PER_HOP;
                 j++)
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
