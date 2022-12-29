#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
  int len = 0;
  while (s[len] != '\0')
  {
    len++;
  }
  return len;
}

char *strcpy(char *dst, const char *src)
{
  if (src == NULL || dst == NULL)
    return NULL;
  char *p = dst;
  while ((*dst++ = *src++) != '\0')
    ;
  return p;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  if (src == NULL || dst == NULL)
    return NULL;
  char *p = dst;
  while (n && (*dst++ = *src++) != '\0')
    n--;
  if (n)
  {
    while (--n)
    {
      *dst++ = '\0';
    }
  }
  return p;
}

char *strcat(char *dst, const char *src)
{
  if (dst == NULL || src == NULL)
    return NULL;
  char *p = dst;
  while (*p != '\0')
    p++;
  while ((*p++ = *src++) != '\0')
    ;
  return dst;
}

int strcmp(const char *s1, const char *s2)
{
  assert(s1 != NULL && s2 != NULL);
  int i = 0, j = 0;
  while (s1[i] != '\0' && s2[j] == s1[i])
  {
    i++;
    j++;
  }
  return s1[i] - s2[j];
}

int strncmp(const char *s1, const char *s2, size_t n)
{
  assert(s1 != NULL && s2 != NULL);
  while (n--)
  {
    if (*s1 == '\0' || *s1 != *s2)
      return *s1 - *s2;
    s1++;
    s2++;
  }
  return 0;
}

void *memset(void *s, int c, size_t n)
{
  char *p = s;
  for (int i = 0; i < n; ++i)
  {
    p[i] = c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n)
{
  char *dst1 = dst;
  const char *src1 = src;
  if (src1 >= dst1 || dst1 >= src1 + n)
  {
    *dst1++ = *src1++;
  }
  else
  {
    dst1 = dst1 + n - 1;
    src1 = src1 + n - 1;
    while (n--)
    {
      *dst1-- = *src1--;
    }
  }
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
  assert(dst != NULL && src != NULL);
  char *p = dst;
  const char *q = src;
  while (n--)
  {
    *p++ = *q++;
  }
  return dst;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  assert(s1 != NULL && s2 != NULL);
  unsigned char *c1 = (unsigned char *)s1, *c2 = (unsigned char *)s2;
  while (n--)
  {
    if (*c1 != *c2)
      return *c1 - *c2;
  }
  return 0;
}

#endif
