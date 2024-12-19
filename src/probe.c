#include "all.h"

void initializeProbes(Probe *probes, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        probes[i].ttl = i / DEFAULT_PROBES_PER_HOP + 1;
        probes[i].seq = i + 1;
        probes[i].socketFd = -1;
        probes[i].final = false;
        probes[i].expired = false;
        probes[i].printed = false;
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
    if (connect(socketFd, &probe->destination, sizeof(probe->destination)) < 0)
        error(strerror(errno));
    setRecverr(socketFd);
    probe->timeSent = timeOfDay();
    if (sendToAddress(socketFd, NULL, 0, probe->destination) < 0)
    {
        close(socketFd);
        probe->timeSent = (struct timeval){0, 0};
        return;
    }
    probe->socketFd = socketFd;
    return;
}

void printProbe(Probe *probe)
{
    printf("Probe %d: TTL %d, Seq %d, Time Sent %ld, Time Received %ld\n", probe->seq, probe->ttl, probe->timeSent,
           probe->timeReceived);
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
    close(probe->socketFd);
    probe->socketFd = -1;
    probe->expired = true;
}