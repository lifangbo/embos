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
#include "semaphore.h"
#include "scheduler.h"
#include "ready.h"
#include "mailbox.h"
#include "mutex.h"
#include "timer_callrte.h"
#include "target/driver/timer_driver.h"


/*!
* schedule_rr()のコールバックルーチン
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void schedule_rr_callrte(void *argv)
{

	/* タスクを回転させる(入れ替える) */
	getcurrent(); /* 実行中のタスクは単一のレディーキュー先頭なので，先頭を抜き取る */
	putcurrent(); /* 抜き取ったタスクを単一レディーキューの最尾へつなぐ */
	
	mg_schdul_info.entry->un.rr_schdul.tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
}


/*!
* schedule_rps()のコールバックルーチン
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void schedule_rps_callrte(void *argv)
{
	/* タスクを回転させる(入れ替える) */
	getcurrent(); /* 実行中のタスクは優先度レベルのレディーキュー先頭なので，先頭を抜き取る */
	putcurrent(); /* 抜き取ったタスクを優先度レベルのレディーキューの最尾へつなぐ */
	
	mg_schdul_info.entry->un.rps_schdul.tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
}


/*!
* schedule_mfq()のコールバックルーチン
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void schedule_mfq_callrte(void *argv)
{
	getcurrent(); /* 実行中タスクをレディーから抜く */
	
	/* 最低優先度になるまで優先度を下げる */
	if (current->priority != PRIORITY_NUM - 1) {
		current->priority++; /* タイムアウトしたので優先度を一つ下げる */
	}
		
	putcurrent(); /* レディーへ戻す */

	mg_schdul_info.entry->un.mfq_schdul.tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
}


/*!
* schedule_odrone()のコールバックルーチン
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void schedule_odrone_callrte(void *argv)
{
	/* タスクをactivキューからexpiredキューに移動させる */
	getcurrent(); /* 実行中のタスクは優先度レベルのactivレディーキュー先頭なので，先頭を抜き取る */
	put_current_tmpri_expiredque(); /* 抜き取ったタスクを優先度レベルのexpiredレディーキューの最尾へつなぐ */
	
	mg_schdul_info.entry->un.odrone_schdul.tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
}


/*!
* schedule_fr()のコールバックルーチン
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void schedule_fr_callrte(void *argv)
{
	getcurrent(); /* 実行中タスクをレディーから抜く */
	
	/* 実行時間の加算(CPUバウンドかI/Oバウンドの公平性実現) */
	current->schdul_info->un.fr_schdul.rel_exetim += mg_schdul_info.entry->un.fr_schdul.tmout;
	
	putcurrent(); /* 再度レディーへ */
	
	mg_schdul_info.entry->un.fr_schdul.tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
}


/*!
* schedule_pfr()のコールバックルーチン
* schedule_fr()のコールバックルーチンと同じだが,わかりやすくするため分けた
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void schedule_pfr_callrte(void *argv)
{
	getcurrent(); /* 実行中タスクをレディーから抜く */
	
	/* 実行時間の加算(CPUバウンドかI/Oバウンドの公平性実現) */
	current->schdul_info->un.fr_schdul.rel_exetim += mg_schdul_info.entry->un.pfr_schdul.tmout;
	
	putcurrent(); /* 再度レディーへ */
	
	mg_schdul_info.entry->un.pfr_schdul.tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
}


/*!
* Earliest Deadline Firstスケジューリングデッドラインミスハンドラ
* EDFの場合デッドライン時刻でタイマをかけて，タイムアウトした場合はこのハンドラでOSをスリープさせる
* (一つのタスクがデッドラインをミスした時点でスリープ．domino effectを防ぐ)
* 一応タイマコールバックルーチンとなるので，scheduler.cではなくtimer_callrte.cに配置した
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void edfschedule_miss_handler(void *argv)
{
	KERNEL_OUTMSG(" Earliest Deadline First Deadline Miss task set\n");
	KERNEL_OUTMSG(" OS freeze... Please push reset button\n");
	freeze_kernel();
}


/*!
* Least Laxity Firstスケジューリングミスハンドラ
* タイマのセットはEDFと同様.
* 余裕時刻をオーバーすれば,デッドラインもミスするので(domino effectする事もある),
* ハンドラを用意した.
* *argv : つねにNULL(タイマ割込みが発生したならば，現在実行中のタスクがcurrentに設定してあるため)
*/
void llfschedule_miss_handler(void *argv)
{
	KERNEL_OUTMSG(" Least Laxity First Float time Miss task set\n");
	KERNEL_OUTMSG(" OS freeze... Please push reset button\n");
	freeze_kernel();
}


/*!
* tslp_tsk()のコールバックルーチン
* *argv : 引数を格納したポインタ(ここではタスクコントロールブロックへの汎用ポインタ)
*/
void tslp_callrte(void *argv)
{
	ER *er;
	current = (TCB *)argv;
	current->state &= ~TASK_WAIT_TIME_SLEEP; /*タイムアウトしたので，フラグを落としておく*/
	current->wait_info.tobjp = 0; /*タスクとタイマコントロールブロックを未接続にする*/
	/*待ちに入ったシステムコールの返却値をポインタを経由して書き換える*/
	er = (ER *)current->syscall_info.ret;
	*er = E_TMOUT;
	putcurrent(); /*レディーへ戻す*/
}


/*!
* dly_tsk()のコールバックルーチン
* *argv : 引数を格納したポインタ(ここではタスクIDの汎用ポインタ)
*/
void dly_callrte(void *argv)
{
	current = (TCB *)argv;
	current->state &= ~TASK_WAIT_TIME_DELAY; /*時間経過したので，フラグを落としておく*/
	current->wait_info.tobjp = 0; /*タスクとタイマコントロールブロックを未接続にする*/
	/*待ちに入ったシステムコールの返却値は書き換えない*/
	putcurrent(); /*レディーへ戻す*/
}


/*!
* twai_sem()のコールバックルーチン
* *argv : 引数を格納したポインタ(ここではタスクIDの汎用ポインタ)
*/
void twai_sem_callrte(void *argv)
{
	SEMCB *scb;
	ER *er;
	
	current = (TCB *)argv;
	scb = (SEMCB *)current->wait_info.wobjp;
	get_tsk_sem_waitque(scb, current); /* 動的セマフォ待ちタスクキューからTCBを抜き取る(関数はsemaphore.cにある) */
	current->state &= ~TASK_WAIT_SEMAPHORE; /* 待ち要因分だけリバースマスク */
	current->wait_info.wobjp = current->wait_info.tobjp = 0; /* 待ちブロックとタイマブロックを未接続にする */
	/* 待ちに入ったシステムコールの返却値をポインタを経由して書き換える */
	er = (ER *)current->syscall_info.ret;
	*er = E_TMOUT;
	putcurrent(); /* タイムアウトしたタスクをレディーへ */
}


/*!
* tloc_mtx()のコールバックルーチン
* *argv : 引数を格納したポインタ(ここではタスクIDの汎用ポインタ)
*/
void tloc_mtx_callrte(void *argv)
{
	MTXCB *mcb;
	ER *er;
	
	current = (TCB *)argv;
	mcb = (MTXCB *)current->wait_info.wobjp;
	get_tsk_mtx_waitque(mcb, current); /* mutex待ちタスクキューからTCBを抜き取る(関数はmutex.cにある) */
	current->state &= ~TASK_WAIT_MUTEX; /* 待ち要因分だけリバースマスク */
	current->wait_info.wobjp = current->wait_info.tobjp = 0; /* 待ちブロックとタイマブロックを未接続にする */
	/* 待ちに入ったシステムコールの返却値をポインタを経由して書き換える */
	er = (ER *)current->syscall_info.ret;
	*er = E_TMOUT;
	putcurrent(); /*レディーへ戻す*/
}


/*!
* trcv_mbx()のコールバックルーチン
* *argv : 引数を格納したポインタ(ここではタスクIDの汎用ポインタ) 
*/
void twai_mbx_callrte(void *argv)
{
	MBXCB *mbcb;
	ER *er;
	
	current = (TCB *)argv;
	mbcb = (MBXCB *)current->wait_info.wobjp;
	get_tsk_mbx_waitque(mbcb, current); /* mutex待ちタスクキューからTCBを抜き取る(関数はmutex.cにある) */
	current->state &= ~TASK_WAIT_MAILBOX; /* 待ち要因分だけリバースマスク */
	current->wait_info.wobjp = current->wait_info.tobjp = 0; /* 待ちブロックとタイマブロックを未接続にする */
	/* 待ちに入ったシステムコールの返却値をポインタを経由して書き換える */
	er = (ER *)current->syscall_info.ret;
	*er = E_TMOUT;
	putcurrent(); /*レディーへ戻す*/
}
