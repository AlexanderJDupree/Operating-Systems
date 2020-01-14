#include "types.h"
#include "x86.h"

static const char DIGITS[] = "0123456789abcdef";

void*
memset(void *dst, int c, uint n)
{
  if ((int)dst%4 == 0 && n%4 == 0){
    c &= 0xFF;
    stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
  } else
    stosb(dst, c, n);
  return dst;
}

int
memcmp(const void *v1, const void *v2, uint n)
{
  const uchar *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void*
memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void*
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

int
strncmp(const char *p, const char *q, uint n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}

char*
strncpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  while(n-- > 0 && (*s++ = *t++) != 0)
    ;
  while(n-- > 0)
    *s++ = 0;
  return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char*
safestrcpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  if(n <= 0)
    return os;
  while(--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}

int
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

char*
reverse(char* s, int n)
{
  int j = n - 1, temp; 
  for(int i = 0; i < j; ++i)
  {
    temp = s[i]; 
    s[i] = s[j]; 
    s[j] = temp; 
    --j;
  }
  return s;
}

char*
itoa(int val, char* buf, int base)
{
  int i = 0; 
  int sign = 1; 

  if (val < 0 && base == 10) 
  { 
      sign = -1; 
      val *= -1; 
  } 

  // Extract digits
  do {
    buf[i++] = DIGITS[val % base];
  } while((val /= base) != 0);

  if(sign < 0) { buf[i++] = '-'; }

  buf[i] = '\0'; // Append string terminator

  // Reverse and return
  return reverse(buf, i); 
}

char* 
dtoa(double val, char* buf, int digits)
{
  int int_part = (int) val;
  double mantissa = val - (double) int_part;

  int i = strlen(itoa(int_part, buf, 10));

  if(digits > 0)
  {
    buf[i++] = '.';

    double d = 1.0f;
    while((mantissa * 10) < d && digits-- > 0)
    {
      // Add leading zeroes
      mantissa *= 10;
      buf[i++] = '0';
    }

    for(int i = 0; i < digits; ++i)
    {
      d *= 10;
    }

    itoa((int) (mantissa *= d), buf + i, 10);
  }

  return buf;
}
