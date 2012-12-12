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
#include "system_manage.h"
#include "ready.h"
#include "semaphore.h"
#include "memory.h"
#include "mutex.h"
#include "time_manage.h"
#include "target/driver/timer_driver.h"


/*! グローバル変数の初期化 */
static void clear_global(void);

/*! タイマ周りのすべての動的メモリ解放 */
static void delete_all_tmrcb(void);

/*! タスク周りのすべての動的メモリ解放 */
static void delete_all_tcb(void);

/*! セマフォ周りのすべての動的メモリ解放 */
static void delete_all_semcb(void);

/*! mutex周りのすべての動的メモリ解放 */
static void delete_all_mtxcb(void);

/*! 周期ハンドラ周りのすべての動的メモリ解放 */
static void delete_all_cyccb(void);

/*! アラームハンドラ周りのすべての動的メモリ解放 */
static void delete_all_almcb(void);



/*
* dis_dsp()とena_dsp()，sns_dsp()，set_pow()はシステムコールにせずにマクロにした方がいいかも
* (システムコールだと遅い)
* 割込み有効と無効はマクロにした(タスク以外からも呼ばれるため)
*/


/*!
* システムコールの処理(rot_rdp():タスクの優先順位回転)
* 周期的に呼ぶ事によって，ラウンドロビンスケジューリングができる
* tskpri : 回転対象の優先度
* (返却値)E_PAR : パラメータ不正
* (返却値)E_OK : 正常終了
*/
ER rot_rdq_isr(int tskpri)
{
	/* パラメータチェック */
	if (tskpri < 0 || PRIORITY_NUM < tskpri) {
		return E_PAR;
	}
	/* 優先度回転処理 */
	else {
		
		if (current->syscall_info.flag == MZ_SYSCALL) {
			/*
			* 実行中タスクの優先度回転処理をする場合は，何もしない(実行権の放棄)．
			* このOSの実装ではシステムコールを発行するとタスクがスイッチングする．
			* (システムコールの発行でgutcurrent()されて，kernenlrte_rot_rdq()でputcurrent()されるから)
			*/
			if (current->priority == tskpri) {
				return E_OK;
			}
			/* 対象優先度のキューの優先度回転 */
			else {
				current = mg_ready_info.entry->un.pri.ready.que[tskpri].head;
				getcurrent(); /* 一度抜き取り */
				putcurrent(); /* 再度繋げる */
				return E_OK;
			}
		}
		else if (current->syscall_info.flag == MZ_VOID) {
			if (current->priority != tskpri) {
				current = mg_ready_info.entry->un.pri.ready.que[tskpri].head;
			}
			getcurrent(); /* 一度抜き取り */
			putcurrent(); /* 再度繋げる */
			return E_OK;
		}
		else {
			/* 処理なし */
			return E_NG;
		}
	}
}


/*!
* システムコールの処理(get_id():スレッドID取得)
* (返却値) : スレッドID
*/
ER_ID get_tid_isr(void)
{
  return current->init.tskid;
}


/*!
* システムコールの処理(dis_dsp():ディスパッチの禁止)
* タスクから発行する時必ずena_dsp()とセットで使用する
* (そうしないと，ディスパッチできないためタスクが終了した時，先に進まなくなる)
* また，ディスパッチ禁止状態でもサービスコールの使用は認める
* (サービスコール自体タスクが切り替わるものは実装していないため)
* 多重割込みを認めていないので，割込みマスク解除検査はない(保留状態はない)
* 割込みマスク検査がないので返却値はない
*/
void dis_dsp_isr(void)
{
	dsp_info.flag = FALSE;
}


/*!
* システムコールの処理(ena_dsp():ディスパッチの許可)
* 多重割込みを認めていないので，割込みマスク解除検査はない(保留状態はない)
* 割込みマスク検査がないので返却値はない
*/
void ena_dsp_isr(void)
{
	dsp_info.flag = TRUE;
}


/*!
* システムコールの処理(sns_dsp():ディスパッチの状態参照)
* 多重割込みを認めていないので，割込みマスク解除検査はない
* (返却値)FALSE : 割込み許可状態
* (返却値)TRUE : 割込み禁止状態
*/
BOOL sns_dsp_isr(void)
{
	/* ディスパッチ許可状態 */
	if (dsp_info.flag) {
		return FALSE;
	}
	/* ディスパッチ禁止状態 */
	else {
		return TRUE;
	}
}


/*!
* システムコールの処理(set_pow():省電力モード設定)
* これを使用する時は無限ループの中に置いたりする
* システムコール及びサービスコールでこの機能を使用して，省電力モード解除する時には
* 割込みが入らなくてはならない．しかし，システムコール及びサービスコールのラッパー処理時は
* 排他をしているため，なかなかタイミング的に割込みを入れるのが難しい(これは課題だな～)．
*/
void set_pow_isr(void)
{
	asm volatile ("sleep"); /* 省電力モードに移行 */
}


/*!
* システムコールの処理(rol_sys():システムをinitタスク生成ルーチンへロールバック)
* atr : ロールバック属性
* (返却値)E_OK : 正常終了
* (返却値)E_PAR : パラメータエラー
*/
ER rol_sys_isr(ROL_ATR atr)
{
	/* タスク実行中にシステムをロールバック */
	if (atr == TA_EXECHG) {
		DEBUG_OUTMSG("exection change system rollback for interrput handler.\n");
		rollback_system(); /* システムをロールバックさせる */
		return E_OK;
	}
	/* タスクがすべて終了したらシステムをロールバック */
	else if (atr == TA_EXITCHG) {
		mg_sys_info.atr |= TA_EXITCHG;
		DEBUG_OUTMSG("exection change system rollback for interrput handler.\n");
		return E_OK;
	}
	/* TA_VOIDCHGは選択できない */
	else {
		return E_PAR;
	}
}


/*!
* init生成ルーチンへロールバックさせる関数
* nmi割込みハンドラの延長で呼ばれるか，システムコール処理(rol_sys():システムをinit生成ルーチンへロールバック)
* の延長で呼ばれる
*/
void rollback_system(void)
{
	void (*func)(TSK_FUNC func, char *name, int priority, int stacksize,
	      int argc, char *argv[]);
	
	clear_global(); /* グローバル変数の初期化 */
	
	/* 動的メモリ類を解放しておかないと，メモリが枯渇する */
	delete_all_tmrcb(); /* タイマ周りのすべての動的メモリ解放 */
	
	delete_all_tcb(); /* タスク周りのすべての動的メモリ解放 */
	
	delete_all_semcb(); /* セマフォ周りのすべての動的メモリ解放 */
	
	delete_all_mtxcb(); /* mutex周りのすべての動的メモリ解放 */
	
	delete_all_cyccb(); /* 周期ハンドラ周りのすべての動的メモリ解放 */
	
	delete_all_almcb(); /* アラームハンドラ周りのすべての動的メモリ解放 */
	      
	func = process_init_tsk; /* ロールバックする関数を設定 */

	(*func)(start_threads, "idle", 0, 0x100, 0, NULL);
	
	/* ここには戻ってこない */
}


/*!
* グローバル変数の初期化
* ・グローバル変数類は明示的に初期化させていなかったため
*/
static void clear_global(void)
{
	mg_tsk_info.counter = mg_tsk_info.power_count = 0;
	mg_sem_info.counter = mg_sem_info.power_count = 0;
	mg_mtx_info.counter = mg_mtx_info.power_count = 0;
	mg_cyc_info.counter = mg_cyc_info.power_count = 0;
	mg_alm_info.counter = mg_alm_info.power_count = 0;
}


/* スケジューラコントロールブロックの解放処理を入れる */


/*! タイマ周りのすべての動的メモリ解放 */
static void delete_all_tmrcb(void)
{
	TMRCB *deltbf = mg_timerque.tmrhead;
	
	if (deltbf != NULL) {
		/* 先にタイマをとめておく */
		cancel_timer(SOFT_TIMER_DEFAULT_DEVICE);
		/* タイマリストを解放する */
		for (; deltbf != NULL; deltbf = deltbf->next) {
			rel_mpf_isr(deltbf); /* 解放 */
		}
	}
}


/*! タスク周りのすべての動的メモリ解放 */
static void delete_all_tcb(void)
{
	TCB *deltcb;
	
	/* unionの領域も解放する処理を入れる */

	rel_mpf_isr(mg_tsk_info.id_table); /* ID変換テーブルの動的メモリ解放 */
	rel_mpf_isr(mg_tsk_info.array); /* 静的型である可変長配列の動的メモリ解放 */
	if (mg_tsk_info.alochead != NULL) {
		deltcb = mg_tsk_info.alochead;
	}
	else {
		deltcb = mg_tsk_info.freehead;
	}
	/* 動的型リストの解放する */
	for (; deltcb != NULL; deltcb = deltcb->free_next) {
		rel_mpf_isr(deltcb); /* 解放 */
	}
}


/*! セマフォ周りのすべての動的メモリ解放 */
static void delete_all_semcb(void)
{
	SEMCB *delscb;
	
	rel_mpf_isr(mg_sem_info.id_table); /* ID変換テーブルの動的メモリ解放 */
	rel_mpf_isr(mg_sem_info.array); /* 静的型である可変長配列の動的メモリ解放 */
	if (mg_tsk_info.alochead != NULL) {
		delscb = mg_sem_info.alochead;
	}
	else {
		delscb = mg_sem_info.freehead;
	}
	/* 動的型リストの解放する */
	for (; delscb != NULL; delscb = delscb->next) {
		rel_mpf_isr(delscb); /* 解放 */
	}
}


/*! mutex周りのすべての動的メモリ解放 */
static void delete_all_mtxcb(void)
{
	MTXCB *delmcb;

	/* virtual mutex解放処理を入れる */
	
	rel_mpf_isr(mg_mtx_info.id_table); /* ID変換テーブルの動的メモリ解放 */
	rel_mpf_isr(mg_mtx_info.array); /* 静的型である可変長配列の動的メモリ解放 */
	if (mg_mtx_info.alochead != NULL) {
		delmcb = mg_mtx_info.alochead;
	}
	else {
		delmcb = mg_mtx_info.freehead;
	}
	/* 動的型リストの解放する */
	for (; delmcb != NULL; delmcb = delmcb->next) {
		rel_mpf_isr(delmcb); /* 解放 */
	}
}


/*! 周期ハンドラ周りのすべての動的メモリ解放 */
static void delete_all_cyccb(void)
{
	CYCCB *delcycb;
	
	rel_mpf_isr(mg_cyc_info.id_table); /* ID変換テーブルの動的メモリ解放 */
	rel_mpf_isr(mg_cyc_info.array); /* 静的型である可変長配列の動的メモリ解放 */
	if (mg_cyc_info.alochead != NULL) {
		delcycb = mg_cyc_info.alochead;
	}
	else {
		delcycb = mg_cyc_info.freehead;
	}
	/* 動的型リストの解放する */
	for (; delcycb != NULL; delcycb = delcycb->next) {
		rel_mpf_isr(delcycb); /* 解放 */
	}
}


/*! アラームハンドラ周りのすべての動的メモリ解放 */
static void delete_all_almcb(void)
{
	ALMCB *delacb;
	
	rel_mpf_isr(mg_alm_info.id_table); /* ID変換テーブルの動的メモリ解放 */
	rel_mpf_isr(mg_alm_info.array); /* 静的型である可変長配列の動的メモリ解放 */
	if (mg_alm_info.alochead != NULL) {
		delacb = mg_alm_info.alochead;
	}
	else {
		delacb = mg_alm_info.freehead;
	}
	/* 動的型リストの解放する */
	for (; delacb != NULL; delacb = delacb->next) {
		rel_mpf_isr(delacb); /* 解放 */
	}
}
