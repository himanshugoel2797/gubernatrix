/**
 * Copyright (c) 2019 Himanshu Goel
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "stddef.h"
#include "string.h"

void *WEAK memset(void *s, int c, size_t n) {
    unsigned long int c8 = c & 0xFF;
    unsigned long int c32 = (c8 << 24) | (c8 << 16) | (c8 << 8) | (c8);
    unsigned long int c64 = (c32 << 32) | (c32);

    unsigned char *s8 = (unsigned char *)s;
    while (true) {
        bool ptr_aligned = ((uintptr_t)s8 % 8) == 0;
        bool n_aligned = (n % 8) == 0;

        if (ptr_aligned && n_aligned)
            break;
        if (n == 0)
            break;

        *s8 = c8;
        s8++;
        n--;
    }

    unsigned long int *s64 = (unsigned long int *)s8;
    while (n > 0) {
        *s64 = c64;
        s64++;
        n -= 8;
    }

    return s;
}

void *WEAK memcpy(void *restrict dest, const void *restrict src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    unsigned char *s = (unsigned char *)src;

    for (size_t i = 0; i < n; i++)
        d[i] = s[i];

    return dest;
}

int WEAK memcmp(const void *__s1, const void *__s2, size_t __n) {

    const uint8_t *s1 = (const uint8_t *)__s1;
    const uint8_t *s2 = (const uint8_t *)__s2;

    while (*s1 == *s2) {
        s1++;
        s2++;
        if (--__n == 0)
            return 0;
    }

    return *s1 - *s2;
}

void *WEAK memmove(void *restrict dest, const void *restrict src, size_t n) {
    unsigned char *pd = dest;
    const unsigned char *ps = src;

    if ((uintptr_t)ps < (uintptr_t)pd)
        for (pd += n, ps += n; n--;)
            *--pd = *--ps;
    else
        while (n--)
            *pd++ = *ps++;
    return dest;
}

size_t WEAK strlen(const char *s) {
    if (s == NULL)
        return 0;

    size_t n = 0;
    while (s[n] != 0)
        n++;

    return n;
}

size_t WEAK strnlen(const char *string, size_t maxLen) {
    if (string == NULL)
        return 0;

    size_t n = 0;
    while (string[n] != 0 && n < maxLen)
        n++;

    return n;
}

int WEAK strncmp(const char *s1, const char *s2, size_t n) {
    while(n--)
        if(*s1++!=*s2++)
            return *(unsigned char*)(s1 - 1) - *(unsigned char*)(s2 - 1);
    return 0;
}

int WEAK strcmp(const char *s1, const char *s2) {
    while (*s1 == *s2 && *s1 != 0 && *s2 != 0) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

char *WEAK strncpy(char *restrict dest, const char *restrict src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    unsigned char *s = (unsigned char *)src;

    for (size_t i = 0; i < n && s[i] != 0; i++)
        d[i] = s[i];

    return dest;
}

char *WEAK strncat(char *restrict dest, const char *restrict src,
                   size_t count) {
    char *ret = dest;
    while (*dest)
        dest++;

    while (count--)
        if (!(*dest++ = *src++))
            return ret;

    *dest = 0;
    return ret;
}

const char *WEAK strchr(const char *__s, int __c) {
    if (__s == NULL)
        return NULL;

    char c = (char)__c;

    while (*__s != c && *__s != 0)
        __s++;

    if (*__s == c)
        return __s;

    return NULL;
}

const char *WEAK strrchr(const char *__s, int __c) {
    if (__s == NULL)
        return NULL;

    char c = (char)__c;
    const char *prev_s = NULL;

    while (true) {
        __s++;

        if (*__s == 0)
            return prev_s;

        if (*__s == c)
            prev_s = __s;
    }

    return NULL;
}

const char *WEAK strstr(const char *haystack, const char *needle) {

    if (needle == NULL)
        return NULL;

    if (haystack == NULL)
        return NULL;

    if (*needle == 0)
        return haystack;

    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);

    while (true) {
        while (*haystack != 0 && *haystack != *needle) {
            haystack++;
            haystack_len--;
        }

        if (*haystack == 0)
            return NULL;

        if (haystack_len < needle_len)
            return NULL;

        if (strncmp(haystack, needle, needle_len) == 0)
            return haystack;

        haystack++;
        haystack_len--;
    }
}