#pragma once
#include "external.h"

// Help
void printHelp();

// Utils
int ft_strcmp(const char *s1, const char *s2);
int ft_memcmp(const void *s1, const void *s2, size_t n);
void *ft_memset(void *b, int c, size_t len);

// Address
struct sockaddr_in parseAddrOrExitFailure(const char *host);
