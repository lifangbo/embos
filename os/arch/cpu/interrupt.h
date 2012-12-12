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


#ifndef _INTERRUPT_H_INCLUDE_
#define _INTERRUPT_H_INCLUDE_


#include "kernel/defines.h"

/*! 以下はリンカスクリプトで定義してあるシンボル */
extern char softvec;
#define SOFTVEC_ADDR 	(&softvec)
#define SOFTVECS 			((SOFTVEC_HANDL *)SOFTVEC_ADDR)

#define INTR_ENABLE  	asm volatile("andc.b #0x3f,ccr") 				/*! 割込み有効化 */
#define INTR_DISABLE 	asm volatile("orc.b #0xc0,ccr") 				/*! 割込み無効化 */
#define CHECK_CCR(x) 	asm volatile("stc ccr,%0l" : "=r" (x)) 	/*! CCR(コンディションコードレジスタ)のレジスタ値取得 */

/*! ソフトウエア・割込みベクタの設定 */
void softvec_setintr(SOFTVEC type, SOFTVEC_HANDL handler);


#endif
