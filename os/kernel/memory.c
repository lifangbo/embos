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


#include "defines.h"
#include "kernel.h"
#include "c_lib/lib.h"
#include "memory.h"


/*!
 * メモリ・ブロック構造体
 * (獲得された各領域は，先頭に以下の構造体を持っている)
 */
typedef struct _mem_block {
  struct _mem_block *next;
  int size;
} mem_block;

/*! メモリプール */
typedef struct _mem_pool {
  int size;
  int num;
  mem_block *free;
} mem_pool;


/*! メモリプールの定義(個々のサイズと個数) */
static mem_pool pool[] = {
  { 16, 128, NULL }, { 32, 128, NULL }, { 64, 128, NULL }, { 128, 128, NULL }, {256, 128, NULL}, {512, 4, NULL}, {1024, 2, NULL}
};


#define MEMORY_AREA_NUM (sizeof(pool) / sizeof(*pool))


/*!
* メモリプールの初期化
* *p : 指定されたメモリプールポインタ
*/
static void mem_init_pool(mem_pool *p)
{
  int i;
  mem_block *mp;
  mem_block **mpp;
  extern char freearea; /* リンカスクリプトで定義される空き領域 */
  static char *area = &freearea;

  mp = (mem_block *)area;

  /* 個々の領域をすべて解放済みリンクリストに繋ぐ */
  mpp = &p->free;
  for (i = 0; i < p->num; i++) {
    *mpp = mp;
    memset(mp, 0, sizeof(*mp));
    mp->size = p->size;
    mpp = &(mp->next);
    mp = (mem_block *)((char *)mp + p->size);
    area += p->size;
  }
}

/*! 動的メモリの初期化 */
void mem_init(void)
{
  int i;
  
  for (i = 0; i < MEMORY_AREA_NUM; i++) {
    mem_init_pool(&pool[i]); /* 各メモリプールを初期化する */
  }
}

/*!
* 動的メモリの獲得
* size : 要求サイズ
*/
void* get_mpf_isr(int size)
{
  int i;
  mem_block *mp;
  mem_pool *p;

  for (i = 0; i < MEMORY_AREA_NUM; i++) {
    p = &pool[i];
    if (size <= p->size - sizeof(mem_block)) {
      if (p->free == NULL) { /* 解放済み領域が無い(メモリブロック不足) */
				down_system();
				return NULL;
      }
      /* 解放済みリンクリストから領域を取得する */
      mp = p->free;
      p->free = p->free->next;
      mp->next = NULL;

      /*
       * 実際に利用可能な領域は，メモリブロック構造体の直後の領域に
       * なるので，直後のアドレスを返す．
       */
      return mp + 1;
    }
  }

  /* 指定されたサイズの領域を格納できるメモリプールが無い */
  down_system();
  
  return NULL;
}

/*! 
* メモリの解放
* *mem : 解放ブロック先頭ポインタ
*/
void rel_mpf_isr(void *mem)
{
  int i;
  mem_block *mp;
  mem_pool *p;

  /* 領域の直前にある(はずの)メモリ・ブロック構造体を取得 */
  mp = ((mem_block *)mem - 1);

  for (i = 0; i < MEMORY_AREA_NUM; i++) {
    p = &pool[i];
    if (mp->size == p->size) {
      /* 領域を解放済みリンクリストに戻す */
      mp->next = p->free;
      p->free = mp;
      return;
    }
  }

  down_system();
}
