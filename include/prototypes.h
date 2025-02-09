#pragma once
#include "external.h"

// Help
void printHelp();

// Utils
int ft_strcmp(const char *s1, const char *s2);
int ft_memcmp(const void *s1, const void *s2, size_t n);
void *ft_memset(void *b, int c, size_t len);
size_t ft_strlcpy(char *dst, const char *src, size_t dstsize);
size_t ft_strlen(const char *s);
void error(const char *str);
int max(int a, int b);
int min(int a, int b);

// Address
struct sockaddr_in parseAddrOrExitFailure(const char *host);

// Probes
void initializeProbes(Probe *probes, size_t numberOfProbes, const struct sockaddr_in destination);
ssize_t sendProbe(Probe *probe, const int sd);
void printProbe(Probe *probe);
void expireProbe(Probe *probe);
Probe *probePointerBySeq(Probe *probes, uint64_t seq);
bool isPrintableProbe(Probe *probe);
bool hasAllPreviousProbesPrinted(Probe *probe, Probe *probes);
/*Probe is with seq, destination.sin_addr.s_addr, timeReceived, final, errorString*/
Probe parseProbe(const char *buffer, ssize_t bytesReceived);
void receiveProbe(Probe *probe, const int sd);
bool isDone(Probe *probes);

// Socket
void setTtl(int socketFd, int ttl);
void setRecverrOrExitFailure(int socketFd);
int sendToAddress(int socketFd, const struct sockaddr_in addr);

// Time
struct timeval timeDifference(const struct timeval start, const struct timeval end);
struct timeval timeSum(const struct timeval time1, const struct timeval time2);
double_t timeValInMiliseconds(const struct timeval timeVal);
struct timeval timeOfDay();
bool isTimeNonZero(const struct timeval time);
struct timeval timeMax(const struct timeval time1, const struct timeval time2);
struct timeval timeMin(const struct timeval time1, const struct timeval time2);
bool isTimeInOrder(const struct timeval time1, const struct timeval time2);
