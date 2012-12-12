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
#include "arch/cpu/interrupt.h"
#include "syscall.h"
#include "mailbox.h"


/* システムコール */
/*!
* 割込み出入り口前のパラメータ類の退避(mz_acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* acre_tsk()システムコールはパラメータ数が多いので,とりあえず構造体×共用体でやった(他のもやったほうがいいのかな～).
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : ユーザスタックが確保できない
* (返却値)tskid : 正常終了
*/
volatile ER_ID mz_acre_tsk(SYSCALL_PARAMCB *par)
{
  SYSCALL_PARAMCB param;

	/*
	* パラメータ退避
	* ここでローカル変数に退避させることによって，ユーザ空間の変数にアクセスしない
	*/
	param.un.acre_tsk.type = par->un.acre_tsk.type;
  param.un.acre_tsk.func = par->un.acre_tsk.func;
  param.un.acre_tsk.name = par->un.acre_tsk.name;
  param.un.acre_tsk.priority = par->un.acre_tsk.priority;
  param.un.acre_tsk.stacksize = par->un.acre_tsk.stacksize;
  param.un.acre_tsk.rate = par->un.acre_tsk.rate;
  param.un.acre_tsk.rel_exetim = par->un.acre_tsk.rel_exetim;
	param.un.acre_tsk.deadtim = par->un.acre_tsk.deadtim;
	param.un.acre_tsk.floatim = par->un.acre_tsk.floatim;
  param.un.acre_tsk.argc = par->un.acre_tsk.argc;
  param.un.acre_tsk.argv = par->un.acre_tsk.argv;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_TSK, &param, (OBJP)(&(param.un.acre_tsk.ret)));
	
	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_del_tsk():スレッドの排除)
* tskid : 排除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクがすでに未登録状態)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクがその他の状態)
*/
ER mz_del_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.del_tsk.tskid = tskid;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_DEL_TSK, &param, (OBJP)(&(param.un.del_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.del_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sta_tsk():スレッドの起動)
* tskid : 起動するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS エラー終了(対象タスクが未登録)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER mz_sta_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.sta_tsk.tskid = tskid;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_STA_TSK, &param, (OBJP)(&(param.un.sta_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.sta_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_run_tsk():タスクコントロールブロックの生成(ID自動割付)と起動)
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)rcd : 自動割付したID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(対象タスクが未登録)
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
volatile ER_ID mz_run_tsk(SYSCALL_PARAMCB *par)
{
  SYSCALL_PARAMCB param;

	/*
	* パラメータ退避
	* ここでローカル変数に退避させることによって，ユーザ空間の変数にアクセスしない
	*/
	param.un.run_tsk.type = par->un.run_tsk.type;
  param.un.run_tsk.func = par->un.run_tsk.func;
  param.un.run_tsk.name = par->un.run_tsk.name;
  param.un.run_tsk.priority = par->un.run_tsk.priority;
  param.un.run_tsk.stacksize = par->un.run_tsk.stacksize;
  param.un.run_tsk.rate = par->un.run_tsk.rate;
  param.un.run_tsk.rel_exetim = par->un.run_tsk.rel_exetim;
	param.un.run_tsk.deadtim = par->un.run_tsk.deadtim;
	param.un.run_tsk.floatim = par->un.run_tsk.floatim;
  param.un.run_tsk.argc = par->un.run_tsk.argc;
  param.un.run_tsk.argv = par->un.run_tsk.argv;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_RUN_TSK, &param, (OBJP)(&(param.un.run_tsk.ret)));
	
	/* 割込み復帰後はここへもどってくる */

  return param.un.run_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_ext_tsk():自タスクの終了)
* リターンパラメータはあってはならない
*/
void mz_ext_tsk(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_EXT_TSK, NULL, WAIT_ERCD_NOCHANGE);

	/* 割込み復帰後はここへもどってこない */

}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_exd_tsk():自スレッドの終了と排除)
* リターンパラメータはあってはならない
*/
void mz_exd_tsk(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_EXD_TSK, NULL, WAIT_ERCD_NOCHANGE);

	/* 割込み復帰後はここへもどってこない */

}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_ter_tsk():スレッドの強制終了)
* tskid : 強制終了するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_ILUSE : エラー終了(タスクが実行状態.つまり自タスク)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了(タスクが実行可能状態または待ち状態)
*/
ER mz_ter_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.ter_tsk.tskid = tskid;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_TER_TSK, &param, (OBJP)(&(param.un.ter_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.ter_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_get_pri():スレッドの優先度取得)
* tskid : 優先度を参照するタスクID
* *p_tskpri : 参照優先度を格納するポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_get_pri(ER_ID tskid, int *p_tskpri)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.get_pri.tskid = tskid;
	param.un.get_pri.p_tskpri = p_tskpri;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_GET_PRI, &param, (OBJP)(&(param.un.get_pri.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.get_pri.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_chg_pri():スレッドの優先度変更)
* tskid : 優先度を変更するタスクID
* tskpri : 変更する優先度
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_chg_pri(ER_ID tskid, int tskpri)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.chg_pri.tskid = tskid;
	param.un.chg_pri.tskpri = tskpri;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_CHG_PRI, &param, (OBJP)(&(param.un.chg_pri.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.chg_pri.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_chg_slt():タスクタイムスライスの変更)
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
ER mz_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.chg_slt.type = type;
	param.un.chg_slt.tskid = tskid;
	param.un.chg_slt.slice = slice;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_CHG_SLT, &param, (OBJP)(&(param.un.chg_slt.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.chg_slt.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_get_slt():タスクタイムスライスの取得)
* type : スケジューラのタイプ
* tskid : 優先度を参照するタスクID
* *p_slice : タイムスライスを格納するパケットへのポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.get_slt.type = type;
	param.un.get_slt.tskid = tskid;
	param.un.get_slt.p_slice = p_slice;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_GET_SLT, &param, (OBJP)(&(param.un.get_slt.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.get_slt.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_slp_tsk():自タスクの起床待ち)
* (返却値)E_NOSPT : 未サポート
* (返却値)E_OK : 正常終了
*/
ER mz_slp_tsk(void)
{
	SYSCALL_PARAMCB param;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_SLP_TSK, &param, (OBJP)(&(param.un.slp_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.slp_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_tslp_tsk():自タスクのタイムアウト付き起床待ち)
* この関数は他のシステムコールと同様に一貫性を保つために追加した
* msec : タイムアウト値
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : エラー終了(パラメータエラー)
* (返却値)E_OK : 正常終了
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_RLWAI(待ち状態の時にrel_wai()が呼ばれた.
*                (これはrel_wai()側の割込みサービスルーチンで返却値E_OKを書き換える))
*/
ER mz_tslp_tsk(int msec)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.tslp_tsk.msec = msec;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_TSLP_TSK, &param, (OBJP)(&(param.un.tslp_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.tslp_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_wup_tsk():タスクの起床)
* tskid : タスクの起床するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
ER mz_wup_tsk(ER_ID tskid)
{
  SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.wup_tsk.tskid = tskid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_WUP_TSK, &param, (OBJP)(&(param.un.wup_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.wup_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_rel_wai():待ち状態強制解除)
* tskid : 待ち状態強制解除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが待ち状態ではない
* (返却値)E_OK : 正常終了
*/
ER mz_rel_wai(ER_ID tskid)
{
  SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.rel_wai.tskid = tskid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_REL_WAI, &param, (OBJP)(&(param.un.rel_wai.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.rel_wai.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_dly_tsk():自タスクの遅延)
* msec : タイムアウト値
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : エラー終了(パラメータエラー)
* (返却値)E_OK : 正常終了
* (返却値)E_RLWAI(待ち状態の時にrel_wai()が呼ばれた.
*                (これはrel_wai()側の割込みサービスルーチンで返却値E_OKを書き換える))
*/
ER mz_dly_tsk(int msec)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.dly_tsk.msec = msec;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_DLY_TSK, &param, (OBJP)(&(param.un.dly_tsk.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.dly_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_acre_sem():セマフォコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)semid : 正常終了(作成したセマフォID)
*/
ER_ID mz_acre_sem(SYSCALL_PARAMCB *par)
{
  SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.acre_sem.type = par->un.acre_sem.type;
  param.un.acre_sem.atr = par->un.acre_sem.atr;
  param.un.acre_sem.semvalue = par->un.acre_sem.semvalue;
  param.un.acre_sem.maxvalue = par->un.acre_sem.maxvalue;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_SEM, &param, (OBJP)(&(param.un.acre_sem.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_sem.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_del_sem():セマフォコントロールブロックの排除)
* semid : 排除するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mz_del_sem(ER_ID semid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.del_sem.semid = semid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEL_SEM, &param, (OBJP)(&(param.un.del_sem.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.del_sem.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sig_sem():セマフォV操作)
* semid : V操作するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(セマフォの解放，待ちタスクへの割り当て)
* (返却値)E_QOVR : キューイングオーバフロー(セマフォ解放エラー(すでに解放済み))，上限値を越えた
*/
ER mz_sig_sem(ER_ID semid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.sig_sem.semid = semid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_SIG_SEM, &param, (OBJP)(&(param.un.sig_sem.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.sig_sem.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_wai_sem():セマフォP操作)
* semid : P操作するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
*/
ER mz_wai_sem(ER_ID semid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.wai_sem.semid = semid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_WAI_SEM, &param, (OBJP)(&(param.un.wai_sem.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.wai_sem.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_pol_sem():セマフォP操作(ポーリング))
* semid : P操作(ポーリング)するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
* (返却値)E_TMOUT : セマフォ待ち失敗(ポーリング)
*/
ER mz_pol_sem(ER_ID semid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.pol_sem.semid = semid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_POL_SEM, &param, (OBJP)(&(param.un.pol_sem.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.wai_sem.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_twai_sem():セマフォP操作(タイムアウト付き))
* semid : P操作(タイムアウト付き)するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
* (返却値)E_TMOUT : タイムアウト
*/
ER mz_twai_sem(ER_ID semid, int msec)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.twai_sem.semid = semid;
	param.un.twai_sem.msec = msec;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_TWAI_SEM, &param, (OBJP)(&(param.un.twai_sem.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.twai_sem.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_acre_mbx():メールボックスコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mbxid : 正常終了(作成したメールボックスID)
*/
ER_ID mz_acre_mbx(SYSCALL_PARAMCB *par)
{
  SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.acre_mbx.type = par->un.acre_mbx.type;
  param.un.acre_mbx.msg_atr = par->un.acre_mbx.msg_atr;
  param.un.acre_mbx.wai_atr = par->un.acre_mbx.wai_atr;
  param.un.acre_mbx.max_msgpri = par->un.acre_mbx.max_msgpri;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_MBX, &param, (OBJP)(&(param.un.acre_mbx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_mbx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_del_mbx():メールボックスコントロールブロックの排除)
* mbxid : 排除するmbxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : メールボックス取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mz_del_mbx(ER_ID mbxid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.del_mbx.mbxid = mbxid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEL_MBX, &param, (OBJP)(&(param.un.del_mbx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.del_mbx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_snd_mbx():メールボックスへ送信)
* mbxid : 送信するmbxid
* *pk_msg : 送信するメッセージパケット先頭番地
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージをキューイング,メッセージをタスクへ送信)
*/
ER mz_snd_mbx(ER_ID mbxid, T_MSG *pk_msg)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.snd_mbx.mbxid = mbxid;
	param.un.snd_mbx.pk_msg = pk_msg;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_SND_MBX, &param, (OBJP)(&(param.un.snd_mbx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.snd_mbx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_rcv_mbx():メールボックスからの受信)
* mbxid : 受信するmbxid
* **pk_msg : 受信するメッセージパケットの先頭番地
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
*/
ER mz_rcv_mbx(ER_ID mbxid, T_MSG **pk_msg)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.rcv_mbx.mbxid = mbxid;
	param.un.rcv_mbx.pk_msg = pk_msg;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_RCV_MBX, &param, (OBJP)(&(param.un.rcv_mbx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.rcv_mbx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_prcv_mbx():ポーリング付きメールボックスからの受信)
* mbxid : 受信するmbxid
* **pk_msg : 受信するメッセージパケットの先頭番地
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージをタスクに与えた)
* (返却値)E_TMOUT : メッセージ待ち失敗(ポーリング)
*/
ER mz_prcv_mbx(ER_ID mbxid, T_MSG **pk_msg)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.prcv_mbx.mbxid = mbxid;
	param.un.prcv_mbx.pk_msg = pk_msg;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_PRCV_MBX, &param, (OBJP)(&(param.un.prcv_mbx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.prcv_mbx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_trcv_mbx():タイムアウト付きメールボックスからの受信)
* mbxid : 受信するmbxid
* **pk_msg : 受信するメッセージパケットの先頭番地
* tmout : タイムアウト値
* (返却値)E_PAR : パラメータエラー
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージがなく待ちタスクとなった，メッセージをタスクに与えた)
* (返却値)E_TMOUT : メッセージ待ち失敗(ポーリング)
*/
ER mz_trcv_mbx(ER_ID mbxid, T_MSG **pk_msg, int tmout)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.trcv_mbx.mbxid = mbxid;
	param.un.trcv_mbx.pk_msg = pk_msg;
	param.un.trcv_mbx.tmout = tmout;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_TRCV_MBX, &param, (OBJP)(&(param.un.trcv_mbx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.trcv_mbx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_acre_mtx():mutexコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mtxid : 正常終了(作成したミューテックスID)
*/
ER_ID mz_acre_mtx(SYSCALL_PARAMCB *par)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.acre_mtx.type = par->un.acre_mtx.type;
	param.un.acre_mtx.atr = par->un.acre_mtx.atr;
	param.un.acre_mtx.piver_type = par->un.acre_mtx.piver_type;
  param.un.acre_mtx.maxlocks = par->un.acre_mtx.maxlocks;
	param.un.acre_mtx.pcl_param = par->un.acre_mtx.pcl_param;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_MTX, &param, (OBJP)(&(param.un.acre_mtx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_mtx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_del_mtx():mutexコントロールブロックの排除)
* mtxid : 排除するmutexid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mz_del_mtx(ER_ID mtxid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.del_mtx.mtxid = mtxid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEL_MTX, &param, (OBJP)(&(param.un.del_mtx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.del_mtx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_loc_mtx():mutexロック操作)
* mtxid : ロック操作するmtxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER mz_loc_mtx(ER_ID mtxid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.loc_mtx.mtxid = mtxid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_LOC_MTX, &param, (OBJP)(&(param.un.loc_mtx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.loc_mtx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_ploc_mtx():mutexポーリングロック操作)
* mtxid : ポーリングロック操作するmtxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_NOSPT : プロトコルありでのpol_sem()は認めない(check_ploc_mtx_protocol()の返却値)
*/
ER mz_ploc_mtx(ER_ID mtxid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.ploc_mtx.mtxid = mtxid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_PLOC_MTX, &param, (OBJP)(&(param.un.ploc_mtx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.ploc_mtx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_tloc_mtx():mutexタイムアウト付きロック操作)
* mtxid : ポーリングロック操作するmtxid
* msec : タイムアウト値
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了(mutexセマフォを取得，mutexセマフォ待ちタスクにつなげる)
* (返却値)E_OK，E_ILUSE : dynamic_multipl_lock()の返却値(再帰ロック完了，多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_NOSPT : プロトコルありでのtloc_sem()は認めない
*/
ER mz_tloc_mtx(ER_ID mtxid, int msec)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.tloc_mtx.mtxid = mtxid;
	param.un.tloc_mtx.msec = msec;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_TLOC_MTX, &param, (OBJP)(&(param.un.tloc_mtx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.tloc_mtx.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_unl_mtx():mutexアンロック操作)
* mtxid : アンロック操作するmtxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER mz_unl_mtx(ER_ID mtxid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.unl_mtx.mtxid = mtxid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_UNL_MTX, &param, (OBJP)(&(param.un.unl_mtx.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.unl_mtx.ret;
}


/* システムコール処理(get_mpf():動的メモリ獲得) */
void* mz_get_mpf(int size)
{
  SYSCALL_PARAMCB param;

  param.un.get_mpf.size = size;
  issue_trap_syscall(ISR_TYPE_GET_MPF, &param, (OBJP)(&(param.un.get_mpf.ret)));

  return param.un.get_mpf.ret;
}


/* システムコール処理(rel_mpf():動的メモリ解放) */
int mz_rel_mpf(void *p)
{
  SYSCALL_PARAMCB param;

  param.un.rel_mpf.p = p;
  issue_trap_syscall(ISR_TYPE_REL_MPF, &param, (OBJP)(&(param.un.rel_mpf.ret)));

  return param.un.kmfree.ret;
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
ER_ID mz_acre_cyc(SYSCALL_PARAMCB *par)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.acre_cyc.type = par->un.acre_cyc.type;
  param.un.acre_cyc.exinf = par->un.acre_cyc.exinf;
  param.un.acre_cyc.cyctim = par->un.acre_cyc.cyctim;
  param.un.acre_cyc.func = par->un.acre_cyc.func;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_CYC, &param, (OBJP)(&(param.un.acre_cyc.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_cyc.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(del_cyc():周期ハンドラコントロールブロックの排除)
* cycid : 排除するcycid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : 周期ハンドラオブジェクト未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mz_del_cyc(ER_ID cycid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.del_cyc.cycid = cycid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEL_CYC, &param, (OBJP)(&(param.un.del_cyc.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.del_cyc.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(sta_cyc():周期ハンドラの動作開始)
* *cycb : 動作開始する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(起動完了)
*/
ER mz_sta_cyc(ER_ID cycid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.sta_cyc.cycid = cycid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_STA_CYC, &param, (OBJP)(&(param.un.sta_cyc.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.sta_cyc.ret;
}


/*!
* システムコール処理(stp_cyc():周期ハンドラの動作停止)
* 周期ハンドラが動作開始していたならば，設定を解除する(排除はしない)
* タイマが動作していない場合は何もしない
* *cycb : 動作停止する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER mz_stp_cyc(ER_ID cycid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.stp_cyc.cycid = cycid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_STP_CYC, &param, (OBJP)(&(param.un.stp_cyc.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.stp_cyc.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付))
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)almid : 正常終了(作成したアラームハンドラID)
*/
ER_ID mz_acre_alm(SYSCALL_PARAMCB *par)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
	param.un.acre_alm.type = par->un.acre_alm.type;
  param.un.acre_alm.exinf = par->un.acre_alm.exinf;
  param.un.acre_alm.func = par->un.acre_alm.func;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_ACRE_ALM, &param, (OBJP)(&(param.un.acre_alm.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.acre_alm.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_del_alm():アラームハンドラコントロールブロックの排除)
* almid : 排除するalmid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER mz_del_alm(ER_ID almid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.del_alm.almid = almid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEL_ALM, &param, (OBJP)(&(param.un.del_alm.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.del_alm.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sta_alm():アラームハンドラの動作開始)
* almid : 動作開始するアラームハンドラのID
* msec : 動作開始時間
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(起動完了)
*/
ER mz_sta_alm(ER_ID almid, int msec)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.sta_alm.almid = almid;
  param.un.sta_alm.msec = msec;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_STA_ALM, &param, (OBJP)(&(param.un.sta_alm.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.sta_alm.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_stp_alm():動的アラームの動作停止)
* almid : 動作停止するalmid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER mz_stp_alm(ER_ID almid)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.stp_alm.almid = almid;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_STP_ALM, &param, (OBJP)(&(param.un.stp_alm.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.stp_alm.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_def_inh():割込みハンドラの定義)
* type : ソフトウェア割込みベクタ番号
* handler : 登録するハンドラポインタ
* (返却値)E_ILUSE : 不正使用
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 登録完了
*/
ER mz_def_inh(SOFTVEC type, IR_HANDL handler)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.def_inh.type = type;
	param.un.def_inh.handler = handler;
	/* トラップ発行 */
  issue_trap_syscall(ISR_TYPE_DEF_INH, &param, (OBJP)(&(param.un.def_inh.ret)));

	/* 割込み復帰後はここへもどってくる */

  return param.un.def_inh.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_rot_rdp():タスクの優先順位回転)
* tskpri : 回転対象の優先度
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : パラメータ不正
* (返却値)E_OK : 正常終了
*/
ER_ID mz_rot_rdq(int tskpri)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.rot_rdq.tskpri = tskpri;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_ROT_RDQ, &param, (OBJP)(&(param.un.rot_rdq.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.rot_rdq.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_get_tid():実行スレッドID取得)
* (返却値) : スレッドID
*/
ER_ID mz_get_tid(void)
{
	SYSCALL_PARAMCB param;

	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_GET_TID, &param, (OBJP)(&(param.un.get_tid.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.get_tid.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_dis_dsp():ディスパッチの禁止)
*/
void mz_dis_dsp(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_DIS_DSP, NULL, WAIT_ERCD_NOCHANGE);

	/* 割込み復帰後はここへもどってくる */
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_ena_dsp():ディスパッチの許可)
*/
void mz_ena_dsp(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_ENA_DSP, NULL, WAIT_ERCD_NOCHANGE);

	/* 割込み復帰後はここへもどってくる */
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_sns_dsp():ディスパッチの状態参照)
*/
BOOL mz_sns_dsp(void)
{
	SYSCALL_PARAMCB param;

	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_SNS_DSP, &param, (OBJP)(&(param.un.sns_dsp.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.sns_dsp.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避いらない(mz_set_pow():省電力モード設定)
*/
void mz_set_pow(void)
{
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_SET_POW, NULL, WAIT_ERCD_NOCHANGE);

	/* 割込み復帰後はここへもどってくる */

}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sel_schdul():スケジューラの切り替え)
* システムコールはタスクをスイッチするため，mz_sel_schdul()は認めていない
* (initロールバックシステムコールを発行しやすくするため)
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_NOSPT : 未サポート
*/
ER mz_sel_schdul(SCHDUL_TYPE type, long param)
{
  return E_NOSPT;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_rol_sys():システムをinitタスク生成ルーチンへロールバック)
* atr : ロールバック属性
* (返却値)E_PAR : パラメータエラー
* 返却値はE_PARだけ(initロールバックの後，E_OKはない．しかし，kernelrteの方はE_OK返る)
*/
ER mz_rol_sys(ROL_ATR atr)
{
	SYSCALL_PARAMCB param;

	/* パラメータ退避 */
  param.un.rol_sys.atr = atr;
	/* トラップ発行 */
	issue_trap_syscall(ISR_TYPE_ROL_SYS, &param, (OBJP)(&(param.un.rol_sys.ret)));

	/* 割込み復帰後はここへもどってくる */

	return param.un.rol_sys.ret;
}


/*
* interrput syscall
* 非タスクコンテキストから呼び出すシステムコール(タスクの切り替えは行わない)
*/
/*
* パラメータ類の退避割込みは使用しない(mz_acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* acre_tsk()システムコールはパラメータ数が多いので,とりあえず構造体×共用体でやった(他のもやったほうがいいのかな～).
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : ユーザスタックが確保できない
* (返却値)tskid : 正常終了
*/
ER mz_iacre_tsk(SYSCALL_PARAMCB *par)
{
	SYSCALL_PARAMCB param;
	/* iacre_tskの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;
	
	/* パラメータ退避 */
	param.un.acre_tsk.type = par->un.acre_tsk.type;
  param.un.acre_tsk.func = par->un.acre_tsk.func;
  param.un.acre_tsk.name = par->un.acre_tsk.name;
  param.un.acre_tsk.priority = par->un.acre_tsk.priority;
  param.un.acre_tsk.stacksize = par->un.acre_tsk.stacksize;
  param.un.acre_tsk.rate = par->un.acre_tsk.rate;
  param.un.acre_tsk.rel_exetim = par->un.acre_tsk.rel_exetim;
	param.un.acre_tsk.deadtim = par->un.acre_tsk.deadtim;
	param.un.acre_tsk.floatim = par->un.acre_tsk.floatim;
  param.un.acre_tsk.argc = par->un.acre_tsk.argc;
  param.un.acre_tsk.argv = par->un.acre_tsk.argv;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_ACRE_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;

	return param.un.acre_tsk.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_sta_tsk():スレッドの起動)
* tskid : 起動するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS エラー終了(対象タスクが未登録)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER mz_ista_tsk(ER_ID tskid)
{
	SYSCALL_PARAMCB param;
	/* ichg_priの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;
	
	/* パラメータ退避 */
	param.un.sta_tsk.tskid = tskid;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_STA_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;

	return param.un.sta_tsk.ret;
}


/*!
* パラメータ類の退避割込みは使用しない(mz_ichg_pri():スレッドの優先度変更)
* tskid : 優先度を変更するタスクID
* tskpri : 変更する優先度
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER mz_ichg_pri(ER_ID tskid, int tskpri)
{
  SYSCALL_PARAMCB param;
	/* ichg_priの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;
 
	/* パラメータ退避 */
  param.un.chg_pri.tskid = tskid;
  param.un.chg_pri.tskpri = tskpri;
  /* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_CHG_PRI, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;
  
  return param.un.chg_pri.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_iwup_tsk():タスクの起床)
* tskid : タスクの起床するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
ER mz_iwup_tsk(ER_ID tskid)
{
  SYSCALL_PARAMCB param;
	/* iwup_tskの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;

	/* パラメータ退避 */
  param.un.wup_tsk.tskid = tskid;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_WUP_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;

  return param.un.wup_tsk.ret;
}


/*!
* パラメータ類の退避割込みは使用しない(acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付))
* この関数からacre_cyc()のISRを呼ぶ
* IDの割付は0から順番に行う.最大数までいった時は，0に再度戻り検索する(途中がdeleteされている事があるため)
* *par : ユーザ側で定義されたシステムコールバッファポインタ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)cycid : 正常終了(作成したアラームハンドラID)
*/
ER mz_iacre_cyc(SYSCALL_PARAMCB *par)
{
	SYSCALL_PARAMCB param;
	/* iacre_cycの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;
	
	/* パラメータ退避 */
	param.un.acre_cyc.type = par->un.acre_cyc.type;
  param.un.acre_cyc.exinf = par->un.acre_cyc.exinf;
  param.un.acre_cyc.cyctim = par->un.acre_cyc.cyctim;
  param.un.acre_cyc.func = par->un.acre_cyc.func;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_ACRE_TSK, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;

	return param.un.acre_cyc.ret;
}


/*!
* パラメータ類の退避割込みは使用しない(sta_cyc():周期ハンドラの動作開始)
* cycid : 動作開始する周期ハンドラID
* (返却値)E_OK : 正常終了(起動完了)
*/
ER mz_ista_cyc(ER_ID cycid)
{
	SYSCALL_PARAMCB param;
	/* ista_cycの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;
	
	/* パラメータ退避 */
	param.un.sta_cyc.cycid = cycid;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_STA_CYC, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;

	return param.un.sta_cyc.ret;
}


/*!
* 割込み出入り口前のパラメータ類の退避(mz_irot_rdp():タスクの優先順位回転)
* tskpri : 回転対象の優先度
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : パラメータ不正
* (返却値)E_OK : 正常終了
*/
ER_ID mz_irot_rdq(int tskpri)
{
	SYSCALL_PARAMCB param;
	/* irot_dtqの延長でget_tsk_readyque()が呼ばれるとcurrentが書き換えられるので一時退避 */
	TCB *tmptcb = current;
	/*
	* システムコール割込みハンドラの延長で非タスクコンテキスト用システムコールが呼ばれた時は，
	* syscall_info.flagが書き換えられるため退避
	*/
	SYSCALL_TYPE tmp_flag = current->syscall_info.flag;

	/* パラメータ退避 */
  param.un.rot_rdq.tskpri = tskpri;
	/* トラップは発行しない(単なる関数呼び出し) */
  isyscall_intr(ISR_TYPE_ROT_RDQ, &param);

	/* 実行状態タスクを前の状態へ戻す */
	current = tmptcb;
	current->syscall_info.flag = tmp_flag;

	return param.un.rot_rdq.ret;
}

