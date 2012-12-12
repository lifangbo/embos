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


#include "kernel.h"
#include "c_lib/lib.h"
#include "scheduler.h"
#include "task_sync.h"
#include "timer_callrte.h"
#include "target/driver/timer_driver.h"


/*!
* システムコールの処理(slp_tsk():自タスクの起床待ち)
* (返却値)E_OK : 正常終了
*/
ER slp_tsk_isr(void)
{
	/* 実装上はレディーキューからTCBを抜き取るだけである */
  return E_OK;
}


/*!
* システムコールの処理(tslp_tsk():自タスクのタイムアウト付き起床待ち)
* msec : タイムアウト値
* (返却値)E_PAR : エラー終了(パラメータエラー)
* (返却値)E_OK : 正常終了
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_RLWAI(待ち状態の時にrel_wai()が呼ばれた.
*                (これはrel_wai()側の割込みサービスルーチンで返却値E_OKを書き換える))
*/
ER tslp_tsk_isr(int msec)
{
	TMRCB *settbf;
	
	if (msec < TMO_FEVR) {
		return E_PAR;
	}
	/* slp_tsk()と同じ動き */
	else if (msec == TMO_FEVR) {
		return E_OK;
	}
	/* ポーリングタイムアウト */
	else if (msec == TMO_POL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_TMOUT;
	}
	/* ここからタイムアウト処理 */
	else {
		settbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, msec, 0, tslp_callrte, current); /* 差分のキューのノードを作成 */
		current->wait_info.tobjp = (TMR_OBJP)settbf; /* TCBのwait_infoとTMRCBの接続 */
		current->state |= TASK_WAIT_TIME_SLEEP; /* 待ち情報をセット(TCBに) */
		return E_OK;
	}
}


/*!
* システムコールの処理(wup_tsk():タスクの起床)
* 起動要求キューイング機能はない.また任意の待ち要因がある時は起床できない
* E_ILUSEの返却値はμITRON4.0にはないものである.act_tsk()との整合性を考えて自タスクの起床もできる
* とあるが，よくわからんのでE_ILUSEとした.
* また，何らかの待ち要因を持っている時はwup_tsk()はできない
* (ただし，タイムアウトスリープは起床できる.tslp_tsk()の返却値はE_OKとなる)
* tcb : 起床要求のタスクコントロールブロックのポインタ
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
ER wup_tsk_isr(TCB *tcb)
{
	/* 休止状態の場合 */
	if (tcb->state == TASK_DORMANT) {
		return E_OBJ;
	}
	/* 実行状態の場合 */
	else if (current == tcb) {
		return E_ILUSE;
	}
	/* 要求タスクが何らかの待ち行列につながれている時(タイムアウトスリープ以外) */
	else if (tcb->state >= TASK_WAIT_TIME_DELAY) {
		return E_ILUSE;
	}
	/* 要求タスクをレディーへつなぎ起床 */
	else {
  	current = tcb;
  	putcurrent(); /* 要求タスクを起床 */
  	return E_OK;
  }
}


/*!
* システムコールの処理(rel_wai():待ち状態強制解除)
* μITRON4.0返却値としてE_RLWAIをちゃんと返せる
* tcb : 対象タスクコントロールブロックへのポインタ
* (返却値)E_OBJ : 対象タスクが待ち状態ではない
* (返却値)E_OK : 正常終了
*/
ER rel_wai_isr(TCB *tcb)
{
	ER *ercd;
	
	/* 待ち状態 */
	if ((tcb->state & TASK_STATE_INFO) == TASK_WAIT) {
		/* 何らかの待ち行列につながれている時は対象タスクを待ち行列からはずす */
		get_tsk_waitque(tcb, (tcb->state & ~TASK_STATE_INFO));
		/* タイマブロックを持っているものは対象タイマブロックを排除する */
		if (tcb->wait_info.tobjp != 0) {
			delete_tmrcb_diffque((TMRCB *)tcb->wait_info.tobjp);
			tcb->wait_info.tobjp = 0; /* クリアにしておく */
		}
		/* 待ちに入ったシステムコールの返却値をポインタを経由して書き換える */
		ercd = (ER *)tcb->syscall_info.ret;
		*ercd = E_RLWAI;
		current = tcb;
		putcurrent(); /* 待ちとなっているタスクをレディーへ */
		return E_OK;
	}
	/* 対象タスクが待ち状態ではない */
	else {
		return E_OBJ;
	}
}


/*!
* システムコールの処理(dly_tsk():自タスクの遅延)
* μITRON仕様にはtslp_tsk()とdly_tsk()の違いは待ちの間にwup_tsk()が発行できるかできないかで，
* dly_tsk()はできない.また0がmsecに指定された場合はポーリングとならずに，最初のタイムスティックで待ち解除を行う
* これはあんま意味がないので，実装しない.
* msec : タイムアウト値
* (返却値)E_PAR : エラー終了(パラメータエラー)
* (返却値)E_OK : 正常終了
* (返却値)E_RLWAI(待ち状態の時にrel_wai()が呼ばれた.
*                (これはrel_wai()側の割込みサービスルーチンで返却値E_OKを書き換える))
*/
ER dly_tsk_isr(int msec)
{
	TMRCB *settbf;
	
	/* パラメータは正しいか */
	if (msec <= -1) {
		return E_PAR;
	}
	/* dly_tsk()処理 */
	else {
		settbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, msec, 0, dly_callrte, current); /*差分のキューのノードを作成*/
		current->wait_info.tobjp = (TMR_OBJP)settbf; /*TCBのwait_infoとTMRCBの接続*/
		current->state |= TASK_WAIT_TIME_DELAY; /*待ち情報をセット(TCBに)*/
		return E_OK;
	}
}
