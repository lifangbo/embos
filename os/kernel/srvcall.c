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
#include "arch/cpu/interrupt.h"
#include "kernel.h"
#include "task_manage.h"
#include "semaphore.h"
#include "mutex.h"
#include "time_manage.h"


/* 
* サービスコールはカーネル内排他をする．システムコールの場合はトラップ発行をすると
* 自動的に割込み無効モードとして走るので排他は必要ないが，サービスコールの場合は，
* トラップ発行をしないので排他が必要となる
*/


/* サービスコール */
/*!
* 割込み出入り口前のパラメータ類の退避(mv_acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : ユーザスタックが確保できない
* (返却値)tskid : 正常終了
*/
ER_ID mv_acre_tsk(SYSCALL_PARAMCB *par)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
	ercd = kernelrte_acre_tsk(par->un.acre_tsk.type, par->un.acre_tsk.func, par->un.acre_tsk.name, 																					par->un.acre_tsk.priority, par->un.acre_tsk.stacksize, 																						par->un.acre_tsk.rate, par->un.acre_tsk.rel_exetim, 																						par->un.acre_tsk.deadtim, par->un.acre_tsk.floatim, 																							par->un.acre_tsk.argc, par->un.acre_tsk.argv);	
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_del_tsk():スレッドの排除)
* tskid : 排除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクがすでに未登録状態)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクがその他の状態)
*/
ER mv_del_tsk(ER_ID tskid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_del_tsk(tskid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_ter_tsk():スレッドの強制終了)
* tskid : 強制終了するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_ILUSE : エラー終了(タスクが実行状態.つまり自タスク)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了(タスクが実行可能状態または待ち状態)
*/
ER mv_ter_tsk(ER_ID tskid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
	ercd = kernelrte_ter_tsk(tskid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_get_pri():スレッドの優先度取得)
* tskid : 優先度を参照するタスクID
* *p_tskpri : 参照優先度を格納するポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mv_get_pri(ER_ID tskid, int *p_tskpri)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
	ercd = kernelrte_get_pri(tskid, p_tskpri);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_chg_slt():タスクタイムスライスの変更)
* type : スケジューラのタイプ
* tskid : 優先度を参照するタスクID
* slice : 変更するタスクスライスタイム
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_PAR : エラー終了(tm_sliceが不正)
* (返却値)E_OK : 正常終了
*/
ER mv_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
	ercd = kernelrte_chg_slt(type, tskid, slice);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_get_slt():タスクタイムスライスの取得)
* type : スケジューラのタイプ
* tskid : 優先度を参照するタスクID
* *p_slice : タイムスライスを格納するパケットへのポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mv_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
	ercd = kernelrte_get_slt(type, tskid, p_slice);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_acre_sem():セマフォコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)semid : 正常終了(作成したセマフォID)
*/
ER_ID mv_acre_sem(SYSCALL_PARAMCB *par)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_acre_sem(par->un.acre_sem.type, par->un.acre_sem.atr, par->un.acre_sem.semvalue, par->un.acre_sem.maxvalue);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_del_sem():セマフォコントロールブロックの排除)
* semid : 排除するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER_ID mv_del_sem(ER_ID semid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_del_sem(semid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_acre_mbx():メールボックスコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mbxid : 正常終了(作成したメールボックスID)
*/
ER_ID mv_acre_mbx(SYSCALL_PARAMCB *par)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_acre_mbx(par->un.acre_mbx.type, par->un.acre_mbx.msg_atr, par->un.acre_mbx.wai_atr, par->un.acre_mbx.max_msgpri);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_del_mbx():メールボックスコントロールブロックの排除)
* mbxid : 排除するmbxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : メールボックス取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER_ID mv_del_mbx(ER_ID mbxid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_del_mbx(mbxid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_acre_mtx():mutexコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mtxid : 正常終了(作成したミューテックスID)
*/
ER_ID mv_acre_mtx(SYSCALL_PARAMCB *par)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_acre_mtx(par->un.acre_mtx.type, par->un.acre_mtx.atr, par->un.acre_mtx.piver_type, 																		par->un.acre_mtx.maxlocks, par->un.acre_mtx.pcl_param);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_del_mtx():mutexコントロールブロックの排除)
* mtxid : 排除するmutexid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER_ID mv_del_mtx(ER_ID mtxid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_del_mtx(mtxid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付))
* この関数からacre_cyc()のISRを呼ぶ
* IDの割付は0から順番に行う.最大数までいった時は，0に再度戻り検索する(途中がdeleteされている事があるため)
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)cycid : 正常終了(作成したアラームハンドラID)
*/
ER_ID mv_acre_cyc(SYSCALL_PARAMCB *par)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_acre_cyc(par->un.acre_cyc.type, par->un.acre_cyc.exinf, par->un.acre_cyc.cyctim, par->un.acre_cyc.func);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(del_cyc():周期ハンドラコントロールブロックの排除)
* cycid : 排除するcycid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : 周期ハンドラオブジェクト未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mv_del_cyc(ER_ID cycid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_del_cyc(cycid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(sta_cyc():周期ハンドラの動作開始)
* *cycb : 動作開始する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(起動完了)
*/
ER mv_sta_cyc(ER_ID cycid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_sta_cyc(cycid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* システムコール処理(stp_cyc():周期ハンドラの動作停止)
* 周期ハンドラが動作開始していたならば，設定を解除する(排除はしない)
* タイマが動作していない場合は何もしない
* *cycb : 動作停止する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER mv_stp_cyc(ER_ID cycid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_stp_cyc(cycid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)almid : 正常終了(作成したアラームハンドラID)
*/
ER_ID mv_acre_alm(SYSCALL_PARAMCB *par)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_acre_alm(par->un.acre_alm.type, par->un.acre_alm.exinf, par->un.acre_alm.func);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_del_alm():アラームハンドラコントロールブロックの排除)
* almid : 排除するalmid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mv_del_alm(ER_ID almid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_del_alm(almid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_sta_alm():アラームハンドラの動作開始)
* almid : 動作開始するアラームハンドラのID
* msec : 動作開始時間
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)E_NOEXS : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(起動完了)
*/
ER mv_sta_alm(ER_ID almid, int msec)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_sta_alm(almid, msec);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_stp_alm():動的アラームの動作停止)
* almid : 動作停止するalmid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)E_NOEXS : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER mv_stp_alm(ER_ID almid)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
	current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_stp_alm(almid);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mv_def_inh():割込みハンドラの定義)
* type : ソフトウェア割込みベクタ番号
* handler : 登録するハンドラポインタ
* (返却値)E_ILUSE : 不正使用
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 登録完了
*/
ER_ID mv_def_inh(SOFTVEC type, IR_HANDL handler)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_def_inh(type, handler);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_rot_rdp():タスクの優先順位回転)
* tskpri : 回転対象の優先度
* (返却値)E_PAR : パラメータ不正
* (返却値)E_OK : 正常終了
*/
ER_ID mv_rot_rdq(int tskpri)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MZ_VOID; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_rot_rdq(tskpri);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_get_tid():実行スレッドID取得)
* (返却値) : スレッドID
*/
ER_ID mv_get_tid(void)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_get_tid();
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_dis_dsp():ディスパッチの禁止)
*/
void mv_dis_dsp(void)
{
	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  kernelrte_dis_dsp();
	INTR_ENABLE; /* 割込み有効にする */
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_ena_dsp():ディスパッチの許可)
*/
void mv_ena_dsp(void)
{
	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
 	kernelrte_ena_dsp();
	INTR_ENABLE; /* 割込み有効にする */
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_sns_dsp():ディスパッチの状態参照)
*/
BOOL mv_sns_dsp(void)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_sns_dsp();
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_set_pow():省電力モード設定)
*/
void mv_set_pow(void)
{
	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  kernelrte_set_pow();
	INTR_ENABLE; /* 割込み有効にする */
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_sel_schdul():スケジューラの切り替え)
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
ER mv_sel_schdul(SCHDUL_TYPE type, long param)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_sel_schdul(type, param);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mv_rol_sys():initへのロールバック)
* atr : ロールバック属性
* (返却値)E_PAR : パラメータエラー
* 返却値はE_PARだけ(initロールバックの後，E_OKはない．しかし，kernelrteの方はE_OK返る)
*/
ER mv_rol_sys(ROL_ATR atr)
{
	ER_ID ercd;

	INTR_DISABLE; /* 割込み無効にする */
  current->syscall_info.flag = MV_SRVCALL; /* サービスコールフラグセット */
	/* ラッパー(ISR)呼び出し */
  ercd = kernelrte_rol_sys(atr);
	INTR_ENABLE; /* 割込み有効にする */

  return ercd;
}
