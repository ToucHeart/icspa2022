#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>
#include <stdio.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...)
{
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...)
{
  int count = 0;
  va_list ap;
  va_start(ap, fmt);
  int num;
  char *cp = NULL;
  while (*fmt != '\0')
  {
    char c = *fmt++;
    if (c == '%')
    {
      switch (*fmt)
      {
      case 'd':
        num = va_arg(ap, int);
        char tmp[20];
        int len = 0;
        while (num != 0)
        {
          tmp[len++] = num % 10 + '0';
          num /= 10;
        }
        --len;
        while (len >= 0)
        {
          out[count++] = tmp[len--];
        }
        break;
      case 's':
        cp = va_arg(ap, char *);
        memcpy(out + count, cp, strlen(cp));
        count+=strlen(cp);
        break;
      default:;
        break;
      }
      fmt++;
    }
    else
      out[count++] = c;
  }
  return count;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
  panic("Not implemented");
}

#endif
