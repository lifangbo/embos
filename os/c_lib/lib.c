/*
 * <著作権及び免責事項>
 *
 * 　本ソフトウェアはフリーソフトです．自由にご使用下さい．
 *
 * このソフトウェアを使用したことによって生じたすべての障害,損害,不具合等に関しては,
 * 私と私の関係者及び,私の所属する団体とも,一切の責任を負いません．
 * 各自の責任においてご使用下さい
 * 
 * この<著作権及び免責事項>があるソースに関しましては,すべて以下の者が作成しました．
 * 作成者 : mtksum
 * 連絡先 : t-moteki@hykwlab.org
 *
 */


#include "kernel/defines.h"
#include "target/driver/serial_driver.h"
#include "c_lib/lib.h"


/*
* 標準Cライブラリにはオリジナルの返却値型は使用しないものとする
* ここはコメントは最低限にしている
*/


/* ターゲット非依存部 */
/*! メモリセット */
void *memset(void *b, int c, long len)
{
  char *p;
  
  for (p = b; len > 0; len--) {
    *(p++) = c;
  }
  
  return b;
}


/*! メモリコピー */
void *memcpy(void *dst, const void *src, long len)
{
  char *d = dst;
  const char *s = src;
  
  for (; len > 0; len--) {
    *(d++) = *(s++);
  }
  
  return dst;
}


/*! メモリ内容比較 */
int memcmp(const void *b1, const void *b2, long len)
{
  const char *p1 = b1, *p2 = b2;
  
  for (; len > 0; len--) {
    if (*p1 != *p2) {
    	if (*p1 > *p2) {
    		return 1;
    	}
    	else {
    		return -1;
    	}
    }
    p1++;
    p2++;
  }
  
  return 0;
}


/*! 文字列の長さ */
int strlen(const char *s)
{
  int len;
  
  for (len = 0; *s; s++, len++) {
    ;
  }
  
  return len;
}


/*! 文字列のコピー */
char *strcpy(char *dst, const char *src)
{
  char *d = dst;
  
  for (;; dst++, src++) {
    *dst = *src;
    if (!*src) {
    	break;
    }
  }
  
  return d;
}


/*! 文字列の比較 */
int strcmp(const char *s1, const char *s2)
{
  while (*s1 || *s2) {
    if (*s1 != *s2) {
    	if (*s1 > *s2) {
    		return 1;
    	}
    	else {
    		return -1;
    	}
    }
    s1++;
    s2++;
  }
  
  return 0;
}


/*! 文字列の比較(文字数制限付き) */
int strncmp(const char *s1, const char *s2, int len)
{
  while ((*s1 || *s2) && (len > 0)) {
    if (*s1 != *s2) {
    	if (*s1 > *s2) {
    		return 1;
    	}
    	else {
    		return -1;
    	}
    }
    s1++;
    s2++;
    len--;
  }
  
  return 0;
}


int atoi(char str[])
{
	int i, n, sign;

	/* 空白読み飛ばし */
	for(i = 0; str[i] == ' '; i++) {
		;
	}
	sign = (str[i] == '-') ? -1 : 1;
	if (str[i] == '+' || str[i] == '-') {
		i++;
	}
	for (n = 0; 48 <= str[i] && str[i] <= 57; i++) {
		n = 10 * n + (str[i] - '0');
	}
	return sign * n;
}


/* ここからはターゲット依存部 */
/*! １文字送信 */
int putc(unsigned char c)
{
  if (c == '\n') {
    serial_send_byte(SERIAL_DEFAULT_DEVICE, '\r');
  }
  
  return serial_send_byte(SERIAL_DEFAULT_DEVICE, c);
}

/*! １文字受信 */
unsigned char getc(void)
{
  unsigned char c = serial_recv_byte(SERIAL_DEFAULT_DEVICE);
  c = (c == '\r') ? '\n' : c;
  putc(c); /* エコーバック */
  
  return c;
}

/*! 文字列送信 */
int puts(unsigned char *str)
{
  while (*str) {
    putc(*(str++));
  }
  
  return 0;
}

/*! 文字列受信 */
int gets(unsigned char *buf)
{
  int i = 0;
  unsigned char c;
  
  do {
    c = getc();
    if (c == '\n')
      c = '\0';
    buf[i++] = c;
  } while (c);
  
  return i - 1;
}

/*! 数値の16進表示 */
int putxval(unsigned long value, int column)
{
  char buf[9];
  char *p;

  p = buf + sizeof(buf) - 1;
  *(p--) = '\0';

  if (!value && !column) {
    column++;
  }

  while (value || column) {
    *(p--) = "0123456789abcdef"[value & 0xf];
    value >>= 4;
    if (column) {
    	column--;
    }
  }

  puts(p + 1);

  return 0;
}
