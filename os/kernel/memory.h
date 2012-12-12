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


#ifndef _MEMORY_H_INCLUDE_
#define _MEMORY_H_INCLUDE_


/*! 動的メモリの初期化 */
void mem_init(void);

/*! 動的メモリの獲得 */
void* get_mpf_isr(int size);

/*! メモリの解放 */
void rel_mpf_isr(void *mem);


#endif
