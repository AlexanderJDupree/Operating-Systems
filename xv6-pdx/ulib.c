#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

static const char DIGITS[] = "0123456789abcdef";

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

#ifdef PDX_XV6
int
atoi(const char *s)
{
  int n, sign;

  n = 0;
  while (*s == ' ') s++;
  sign = (*s == '-') ? -1 : 1;
  if (*s == '+'  || *s == '-')
    s++;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return sign*n;
}

int
atoo(const char *s)
{
  int n, sign;

  n = 0;
  while (*s == ' ') s++;
  sign = (*s == '-') ? -1 : 1;
  if (*s == '+'  || *s == '-')
    s++;
  while('0' <= *s && *s <= '7')
    n = n*8 + *s++ - '0';
  return sign*n;
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
#else
int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}
#endif // PDX_XV6

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
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

int
isdigit(char c)
{
  return ('0' <= c && c <= '9');
}
