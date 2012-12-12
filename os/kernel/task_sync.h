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


#ifndef _TASK_SYNC_H_INCLUDE_
#define _TASK_SYNC_H_INCLUDE_


#include "defines.h"


/*! システムコールの処理(slp_tsk():自タスクの起床待ち) */
ER slp_tsk_isr(void);

/*! システムコールの処理(tslp_tsk():自タスクのタイムアウト付き起床待ち) */
ER tslp_tsk_isr(int msec);

/*! システムコールの処理(wup_tsk():タスクの起床) */
ER wup_tsk_isr(TCB *tcb);

/*! システムコールの処理(rel_wai():待ち状態強制解除) */
ER rel_wai_isr(TCB *tcb);

/*! システムコールの処理(dly_tsk():自タスクの遅延) */
ER dly_tsk_isr(int msec);


#endif
