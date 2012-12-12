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


#ifndef _INTR_MANAGE_H_INCLUDE_
#define _INTR_MANAGE_H_INCLUDE_


#include "defines.h"


/*! 割込みハンドラ */
IR_HANDL handlers[SOFTVEC_TYPE_NUM];


/*! システムコールの処理(def_inh():割込みハンドラの定義) */
ER def_inh_isr(SOFTVEC type, IR_HANDL handler);


#endif
