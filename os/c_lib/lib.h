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


#ifndef _LIB_H_INCLUDED_
#define _LIB_H_INCLUDED_


/* ターゲット非依存部 */

/*! メモリセット */
void *memset(void *b, int c, long len);
/*! メモリコピー */
void *memcpy(void *dst, const void *src, long len);

/*! メモリ内容比較 */
int memcmp(const void *b1, const void *b2, long len);

/*! 文字列の長さ */
int strlen(const char *s);

/*! 文字列のコピー */
char *strcpy(char *dst, const char *src);

/*! 文字列の比較 */
int strcmp(const char *s1, const char *s2);

/*! 文字列の比較(文字数制限付き) */
int strncmp(const char *s1, const char *s2, int len);

/* 数値へ変換 */
int atoi(char str[]);


/* ターゲット依存部 */

/*! １文字送信 */
int putc(unsigned char c);

/*! １文字受信 */
unsigned char getc(void);

/*! 文字列送信 */
int puts(unsigned char *str);

/*! 文字列受信 */
int gets(unsigned char *buf);

/*! 数値の16進表示 */
int putxval(unsigned long value, int column);


#endif
