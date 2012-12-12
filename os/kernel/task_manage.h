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


#ifndef _TASK_MANAGE_H_INCLUDE_
#define _TASK_MANAGE_H_INCLUDE_


#include "defines.h"


/*! タスクの初期化(task ID変換テーブルの領域確保と初期化) */
ER tsk_init(void);

/*! システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)) */
OBJP acre_tsk_isr(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int exetim, int deadtim, int floatim, int argc, char *argv[]);
				 
/*! システムコール処理(del_tsk():スレッドの排除) */
ER del_tsk_isr(TCB *tcb);

/*! システムコール処理(sta_tsk():スレッドの起動) */
ER sta_tsk_isr(TCB *tcb);

/*! システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)と起動) */
ER_ID run_tsk_isr(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int rel_exetim, int deadtim, int floatim, int argc, char *argv[]);

/*! システムコールの処理(ext_tsk():自タスクの終了) */
void ext_tsk_isr(void);

/*! システムコールの処理(exd_tsk():自スレッドの終了と排除) */
void exd_tsk_isr(void);

/*! システムコール処理(ter_tsk():タスクの強制終了) */
ER ter_tsk_isr(TCB *tcb);

/*! システムコールの処理(get_pri():スレッドの優先度取得) */
ER get_pri_isr(TCB *tcb, int *p_tskpri);

/*! システム・コールの処理(chg_pri():スレッドの優先度変更) */
ER chg_pri_isr(TCB *tcb, int tskpri);

/*! タスクの優先度を変更する関数 */
void chg_pri_tsk(TCB *tcb, int tskpri);

/*! システムコールの処理(get_id():スレッドID取得) */
ER_ID get_tid_isr(void);

/*! システム・コールの処理(chg_slt():タスクタイムスライスの変更) */
ER chg_slt_isr(SCHDUL_TYPE type, TCB *tcb, int slice);

/*! システム・コールの処理(get_slt():タスクタイムスライスの取得) */
ER get_slt_isr(SCHDUL_TYPE type, TCB *tcb, int *p_slice);


#endif
