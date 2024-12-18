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

// Address
struct sockaddr_in parseAddrOrExitFailure(const char *host);

// Probes
void initializeProbes(Probe *probes, size_t numberOfProbes);
void sendProbe(Probe *probe);
void printProbe(Probe *probe);

// Socket
void setTtl(int socketFd, int ttl);
void setRecverr(int socketFd);
int sendToAddress(int socketFd, const void *data, size_t len, const struct sockaddr_in addr);

// Time
struct timeval timeDifference(const struct timeval start, const struct timeval end);
struct timeval timeSum(const struct timeval time1, const struct timeval time2);
double_t timeValInMiliseconds(const struct timeval timeVal);
struct timeval timeOfDay();
