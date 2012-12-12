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


#ifndef _INTR_H_INCLUDE_
#define _INTR_H_INCLUDE_

/* ソフトウエア割込みベクタの定義 */

#define SOFTVEC_TYPE_NUM			5		/*! ソフトウェア割込みベクタの数 */

#define SOFTVEC_TYPE_SOFTERR	0		/*! ソフトウェアエラーのソフトウェアベクタ */
#define SOFTVEC_TYPE_SYSCALL	1		/*! システムコールのソフトウェアベクタ */
#define SOFTVEC_TYPE_SERINTR	2		/*! シリアル割込みのソフトウェアベクタ */
#define SOFTVEC_TYPE_TIMINTR	3		/*! タイマ割込みソフトウェアベクタ */
#define SOFTVEC_TYPE_NMIINTR	4		/*! NMI割込みソフトウェアベクタ */

#endif
