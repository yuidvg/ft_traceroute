#include "all.h"

int ft_strcmp(const char *s1, const char *s2)
{
    size_t i = 0;

    while (s1[i] || s2[i])
    {
        if (s1[i] != s2[i])
            return ((unsigned char)s1[i] - (unsigned char)s2[i]);
        i++;
    }
    return (0);
}

int ft_memcmp(const void *s1, const void *s2, size_t n)
{
    size_t i = 0;
    unsigned char *s1c = (unsigned char *)s1;
    unsigned char *s2c = (unsigned char *)s2;

    while (i < n)
    {
        if (s1c[i] != s2c[i])
            return ((int)(s1c[i] - s2c[i]));
        i++;
    }
    return (0);
}

void *ft_memset(void *b, int c, size_t len)
{
    size_t i = 0;

    while (i < len)
    {
        *((unsigned char *)b + i) = (unsigned char)c;
        i++;
    }
    return (b);
}

size_t ft_strlcpy(char *dst, const char *src, size_t dstsize)
{
    size_t i = 0;

    if (dstsize == 0)
        return (ft_strlen(src));
    while (src[i] && i < dstsize - 1)
    {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return (ft_strlen(src));
}

size_t ft_strlen(const char *s)
{
    size_t count = 0;

    if (!s)
        return (0);
    while (s[count])
        count++;
    return (count);
}

void error(const char *str)
{
    // fprintf(stderr, "%s\n", str);
    perror(str);
    exit(EXIT_FAILURE);
}

int max(int a, int b)
{
    return a > b ? a : b;
}

double_t dmax(double_t a, double_t b)
{
    return a > b ? a : b;
}

int min(int a, int b)
{
    return a < b ? a : b;
}
