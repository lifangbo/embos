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
#include "arch/cpu/intr.h"
#include "intr_manage.h"
#include "kernel.h"
#include "c_lib/lib.h"
#include "kernel_svc/log_manage.h"
#include "mailbox.h"
#include "memory.h"
#include "mutex.h"
#include "target/driver/nmi.h"
#include "prinvermutex.h"
#include "ready.h"
#include "scheduler.h"
#include "semaphore.h"
#include "syscall.h"
#include "system_manage.h"
#include "task_manage.h"
#include "task_sync.h"
#include "target/driver/timer_driver.h"
#include "time_manage.h"


/*! ディスパッチャ(実体はstartup.sにある) */
void dispatch(UINT32 *context);

/*! タスク生成パラメータチェック関数 */
static ER check_cre_tsk(TSK_FUNC func, int priority, int stacksize, int rate, int rel_exetim, int deadtim, int floatim);

/*! tskid変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_tskid_table(int tskids);

/*! task IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_tskid_table(void);

/*! task IDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_tsk(TCB **tsktbl_tmp, TCB *stsk_tmp, TCB *dtsk_atmp, TCB *dtsk_ftmp);

/*! task IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数 */
static void copy_stsk(TCB *stsk_tmp, int tskids);

/*! task IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dtsk(TCB *dtsk_atmp, TCB *dtsk_ftmp, int tskids);

/*! システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)と起動) */
static ER_ID kernelrte_run_tsk(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int rel_exetim, int deadtim, int floatim, int argc, char *argv[]);

/*! tskid変換テーブル設定処理はいらない(ext_tsk():自タスクの終了) */
static void kernelrte_ext_tsk(void);

/*! tskid変換テーブル設定処理はいらない(exd_tsk():自スレッドの終了と排除) */
static void kernelrte_exd_tsk(void);

/*! tskid変換テーブル設定処理(chg_pri():スレッドの優先度変更) */
static ER kernelrte_chg_pri(ER_ID tskid, int tskpri);

/*! tskid変換テーブル設定処理はいらない(slp_tsk():自タスクの起床待ち) */
static ER kernelrte_slp_tsk(void);

/*! tskid変換テーブル設定処理はいらない(tslp_tsk():自タスクのタイムアウト付き起床待ち) */
static ER kernelrte_tslp_tsk(int msec);

/*! tskid変換テーブル設定処理(wup_tsk():タスクの起床) */
static ER kernelrte_wup_tsk(ER_ID tskid);

/*! tskid変換テーブル設定処理(rel_wai():待ち状態強制解除) */
static ER kernelrte_rel_wai(ER_ID tskid);

/*! tskid変換テーブル設定処理はいらない(dly_tsk():自タスクの遅延) */
static ER kernelrte_dly_tsk(int msec);

/*! semaphore ID変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_semid_table(int semids);

/*! semaphore IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_semid_table(void);

/*! semaphore IDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_sem(SEMCB **semtbl_tmp,SEMCB *ssem_tmp, SEMCB *dsem_atmp, SEMCB *dsem_ftmp);

/*! semaphore IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数 */
static void copy_ssem(SEMCB *ssem_tmp, int semids);

/*! semaphore IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dsem(SEMCB *dsem_atmp, SEMCB *dsem_ftmp, int semids);

/*! semid変換テーブル設定処理(sig_sem():セマフォV操作) */
static ER kernelrte_sig_sem(ER_ID semid);

/*! semid変換テーブル設定処理(wai_sem():セマフォP操作) */
static ER kernelrte_wai_sem(ER_ID semid);

/*! semid変換テーブル設定処理(pol_sem():セマフォP操作(ポーリング)) */
static ER kernelrte_pol_sem(ER_ID semid);

/*! semid変換テーブル設定処理(twai_sem():セマフォP操作(タイムアウト付き)) */
static ER kernelrte_twai_sem(ER_ID semid, int msec);

/*! mailbox ID変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_mbxid_table(int mbxids);

/*! mailbox IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_mbxid_table(void);

/*! mailbox IDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_mbx(MBXCB **mbxtbl_tmp, MBXCB *smbx_tmp, MBXCB *dmbx_atmp, MBXCB *dmbx_ftmp);

/*! mailbox IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数 */
static void copy_smbx(MBXCB *smbx_tmp, int mbxids);

/*! mailbox IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dmbx(MBXCB *dmbx_atmp, MBXCB *dmbx_ftmp, int mbxids);

/*! mbxid変換テーブル設定処理(snd_mbx():メールボックスへ送信) */
static ER kernelrte_snd_mbx(ER_ID mbxid, T_MSG *pk_msg);

/*! mbxid変換テーブル設定処理(rcv_mbx():メールボックスからの受信) */
static ER kernelrte_rcv_mbx(ER_ID mbxid, T_MSG **pk_msg);

/*! mbxid変換テーブル設定処理(prcv_mbx():ポーリング付きメールボックスからの受信) */
static ER kernelrte_prcv_mbx(ER_ID mbxid, T_MSG **pk_msg);

/*! mbxid変換テーブル設定処理(trcv_mbx():タイムアウト付きメールボックスからの受信) */
static ER kernelrte_trcv_mbx(ER_ID mbxid, T_MSG **pk_msg, int tmout);

/*! mutex ID変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_mtxid_table(int mtxids);

/*! mutex IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_mtxid_table(void);

/*! mutex IDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_mtx(MTXCB **mtxtbl_tmp,MTXCB *smtx_tmp, MTXCB *dmtx_atmp, MTXCB *dmtx_ftmp);

/*! mutex IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数 */
static void copy_smtx(MTXCB *smtx_tmp, int mtxids);

/*! mutex IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dmtx(MTXCB *dmtx_atmp, MTXCB *dmtx_ftmp, int mtxids);

/*! mtxid変換テーブル設定処理(loc_mtx():mutexロック操作) */
static ER kernelrte_loc_mtx(ER_ID mtxid);

/*! mtxid変換テーブル設定処理(ploc_mtx():mutexポーリングロック操作) */
static ER kernelrte_ploc_mtx(ER_ID mtxid);

/*! mtxid変換テーブル設定処理(tloc_mtx():mutexタイムアウト付きロック操作) */
static ER kernelrte_tloc_mtx(ER_ID mtxid, int msec);

/*! mtxid変換テーブル設定処理(unl_mtx():mutexアンロック操作) */
static ER kernelrte_unl_mtx(ER_ID mtxid);

/*! システムコール処理(get_mpf():動的メモリ獲得) */
static void* kernelrte_get_mpf(int size);

/*! システムコール処理(rel_mpf():動的メモリ解放) */
static int kernelrte_rel_mpf(char *p);

/*! cycleid変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_cycid_table(int cycids);

/*! cycle handler IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_cycid_table(void);

/*! cycle handlerIDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_cyc(CYCCB **cyctbl_tmp, CYCCB *scyc_tmp, CYCCB *dcyc_atmp, CYCCB *dcyc_ftmp);

/*! cycle handlerIDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数 */
static void copy_scyc(CYCCB *scyc_tmp, int cycids);

/*! cycle handlerIDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dcyc(CYCCB *dcyc_atmp, CYCCB *dcyc_ftmp, int cycids);

/*! alamID変換テーブルの次の割付け可能IDを検索をする関数 */
static ER_ID serch_almid_table(int almids);

/*! alarm handler IDが足らなくなった場合の処理(次の割付可能ID) */
static ER pow_almid_table(void);

/*! alarm handlerIDが足らなくなった時に倍のID変換テーブルへコピーする関数 */
static void copy_all_alm(ALMCB **almtbl_tmp, ALMCB *salm_tmp, ALMCB *dalm_atmp, ALMCB *dalm_ftmp);

/*! alarm handlerIDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数 */
static void copy_salm(ALMCB *salm_tmp, int almids);

/*! alarm handlerIDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数 */
static void copy_dalm(ALMCB *dalm_atmp, ALMCB *dalm_ftmp, int almids);

/*! システムコールタイプを見て各ISRを呼び出す */
static void call_functions(SYSCALL_TYPE type, SYSCALL_PARAMCB *p);

/*! タスクコンテキスト用システムコールの各割込みサービスルーチンを呼び出すための準備関数 */
static void syscall_proc(ISR_TYPE type, SYSCALL_PARAMCB *p);

/*! 非タスクコンテキスト用システムコールの各割込みサービスルーチンを呼び出すための準備関数 */
static void isyscall_proc(ISR_TYPE type, SYSCALL_PARAMCB *p);

/*! システムコール割込みハンドラ */
static void syscall_intr(void);

/*! ソフトウェアエラー(例外)ハンドラ */
static void softerr_intr(void);

/*! タスクコンテキスト情報をTCBへ設定する関数 */
static void set_tsk_context(SOFTVEC type, UINT32 sp);

/*! ディスパッチャの初期化 */
static void dispatch_init(void);

/*! デフォルトのスケジューラを登録する関数 */
static void set_schdul_init_tsk(void);


/*!
* タスク生成パラメータチェック関数
* func : タスクのメイン関数
* priority : タスクの優先度
* stacksize : ユーザスタック(タスクのスタック)のサイズ
* rate : 周期
* rel_exetim : 実行時間(仮想)
* deadtim : デッドライン時刻
* floatim : 余裕時間
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_OK : 生成可能
*/
static ER check_cre_tsk(TSK_FUNC func, int priority, int stacksize, int rate, int rel_exetim, int deadtim, int floatim)
{
	SCHDUL_TYPE schdul_type = mg_schdul_info.type;

	/*
  * 割付可能なIDがない(実行されるのはメモリ不足を起こした時のみ)
  * カウンタの数は静的型task配列の個数，動的型taskリストの個数と一致する
  */
  if (mg_tsk_info.counter < 0) {
  	return E_NOID;
  }
  
	/* システムコールの引数は正しいか(スタックサイズの最低0x48分を使用) */
  else if (!func || priority >= PRIORITY_NUM || stacksize < 0x48) {
  	return E_PAR;
  }
  
  /* Rate Monotonic時のパラメータチェック(initタスクは省く) */
  else if (schdul_type == RM_SCHEDULING && mg_tsk_info.counter > INIT_TASK_ID) {
  	if (rate <= 0 || rel_exetim <= 0 || rate < rel_exetim) {
  		return E_PAR; /* 生成不可 */
  	}
  	else {
  		return E_OK; /* 生成可能 */
  	}
  }
  
  /* Deadline Monotonic時のパラメータチェック(initタスクは省く) */
  else if (schdul_type == DM_SCHEDULING && mg_tsk_info.counter > INIT_TASK_ID) {
  	if (rate <= 0 || rel_exetim <= 0 || deadtim <= 0 || rate < rel_exetim || deadtim < rel_exetim || rate < deadtim) {
  		return E_PAR; /* 生成不可 */
  	}
  	else {
  		return E_OK; /* 生成可能 */
  	}
  }
  
	/* Earliest Deadline First Scheduling時のパラメータチェック(initタスクは省く) */
  else if (schdul_type == EDF_SCHEDULING && mg_tsk_info.counter > INIT_TASK_ID) {
  	if (deadtim <= 0) {
  		return E_PAR; /* 生成不可 */
  	}
  	else {
  		return E_OK; /* 生成可能 */
  	}
  }
  
  /* Least Laxity First Scheduling時のパラメータチェック(initタスクは省く) */
  else if (schdul_type == LLF_SCHEDULING && mg_tsk_info.counter > INIT_TASK_ID) {
  	if (floatim <= 0) {
  		return E_PAR; /* 生成不可 */
  	}
  	else {
  		return E_OK; /* 生成可能 */
  	}
  }
  
  /* RMとDM,EDF,LLF以外の時の優先度チェック */
  else if (priority < -1) {
  		return E_PAR;
  }
	else {
		return E_OK;
	}
}


/*!
* tskid変換テーブル設定処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* この関数からmz_acre_alm()のISRを呼ぶ
* IDの割付は0から順番に行う.最大数までいった時は，0に再度戻り検索する(途中がdeleteされている事があるため)
* type : 静的型か動的型か？
* func : タスクのメイン関数
* *name : タスクの名前
* priority : タスクの優先度
* stacksize : ユーザスタック(タスクのスタック)のサイズ
* rate : 周期
* rel_exetim : 実行時間(仮想)
* deadtim : デッドライン時刻
* floatim : 余裕時間
* argc : タスクのメイン関数の第一引数
* *argv[] : タスクのメイン関数の第二引数
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)tskid : 正常終了
*/
ER_ID kernelrte_acre_tsk(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int rel_exetim, int deadtim, int floatim, int argc, char *argv[])
{
	ER_ID tskid, ercd;
	int tskids;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	tskids = TASK_ID_NUM << mg_tsk_info.power_count; /* 検索範囲の指定 */
	
	/* タスク生成可能状態かチェックルーチン呼び出し */
	ercd = (ER_ID)check_cre_tsk(func, priority, stacksize, rate, rel_exetim, deadtim, floatim);
	/* タスク生成可能状態か */
  if (ercd != E_OK) {
		return ercd;
	}
	/* 生成できる場合 */
	else {
		/* ISRの呼び出し(mz_acre_tsk())とID変換テーブルの設定 */
		mg_tsk_info.id_table[mg_tsk_info.counter] = (TCB *)acre_tsk_isr(type, func, name, 																										priority, stacksize, rate, rel_exetim, deadtim,floatim, argc, argv);
		tskid = (ER_ID)mg_tsk_info.counter; /* システムコールのリターンパラメータ型へ変換 */
		mg_tsk_info.counter++; /* 次の割付けIDへ(mg_tsk_info.id_table[]のインデックス) */
		/* 次の割付けIDがすでに使用されている場合 */
		if ((tskids <= mg_tsk_info.counter) || (mg_tsk_info.id_table[mg_tsk_info.counter] != NULL)) {
			mg_tsk_info.counter = (int)serch_tskid_table(tskids); /* 割付け可能なIDを検索 */
		}
		
		return tskid; /* 割付可能なID返却 */
	}
}


/*!
* tskid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* tskid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOMEM) : メモリ不足
*/
static ER_ID serch_tskid_table(int tskids)
{
	ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* tskid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < tskids; srh_cuntr++) {
		if (mg_tsk_info.id_table[srh_cuntr] == NULL) {
			DEBUG_OUTVLE(srh_cuntr, 0);
			DEBUG_OUTMSG(" aaaaaa\n");
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_tskid_table() != E_OK) { /* tskidが足らなくなった場合 */
		srh_cuntr = -1; /* 次のISR処理でE_NOIDになるようにする */
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = tskids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_OUTVLE(srh_cuntr, 0);
  DEBUG_OUTMSG(" tsk next counter\n");
  
	return srh_cuntr;
}


/*!
* task IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_tskid_table(void)
{
	TCB **tsktbl_tmp, *stsk_tmp, *dtsk_atmp, *dtsk_ftmp;
	
	mg_tsk_info.power_count++;
	
	/* 現在使用していた領域すべてを一時退避 */
	tsktbl_tmp = mg_tsk_info.id_table;
	stsk_tmp = mg_tsk_info.array;
	dtsk_atmp = mg_tsk_info.alochead;
	dtsk_ftmp = mg_tsk_info.freehead; /* この時，必ずmg_tsk_info.freeheadはNULLとはならない(静的型があるため) */
	
	if (tsk_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_tsk(tsktbl_tmp, stsk_tmp, dtsk_atmp, dtsk_ftmp);
	
	/* 退避していた領域を解放(動的型alarm handlerリストは解放しない) */
	rel_mpf_isr(tsktbl_tmp);
	rel_mpf_isr(stsk_tmp);
	
	return E_OK;
}


/*!
* task IDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **tsktbl_tmp : 古いtask ID変換テーブル
* *stsk_tmp : 古い静的型task可変長配列
* *dtsk_atmp : 古い動的型task aloc list head
* *dtsk_ftmp : 古い動的型task free list head
*/
static void copy_all_tsk(TCB **tsktbl_tmp, TCB *stsk_tmp, TCB *dtsk_atmp, TCB *dtsk_ftmp)
{
	int tskids;
	
	tskids = TASK_ID_NUM << (mg_tsk_info.power_count - 1); /* 古いtask ID資源数 */
	memcpy(mg_tsk_info.id_table, tsktbl_tmp, sizeof(&(**tsktbl_tmp)) * tskids); /* 可変長配列なのでコピー */
	
	copy_stsk(stsk_tmp, tskids); /* 静的型task配列のコピーとtask ID変換テーブル更新 */
	copy_dtsk(dtsk_atmp, dtsk_ftmp, tskids); /* 動的型taskリストの連結と更新(コピーはしない) */
}


/*!
* task IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数
* tskids : 古いtask ID資源数
* *stsk_tmp : 古い静的型task可変長配列
*/
static void copy_stsk(TCB *stsk_tmp, int tskids)
{
	int i;
	
	memcpy(mg_tsk_info.array, stsk_tmp, sizeof(*(mg_tsk_info.array)) * tskids); /* 可変長配列なのでコピー */
	
	/* 静的型task配列をコピーしたので，それに応じてtaskID変換テーブルを更新 */
	for (i = 0; i < tskids; i++) {
		if (mg_tsk_info.array[i].init.tskid != -1) {
			mg_tsk_info.id_table[i] = &mg_tsk_info.array[i]; /* 変換テーブルの更新 */
		}
	}
}

/*!
* task IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
* tskids : 古いtask ID資源数
* *dtsk_atmp : 古い動的型task aloc list head
* *dtsk_ftmp : 古い動的型task free list head
*/
static void copy_dtsk(TCB *dtsk_atmp, TCB *dtsk_ftmp, int tskids)
{
	TCB *oldtcb;
	
	/*
	* 動的型taskはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型task freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dtsk_ftmp == NULL) {
		oldtcb = dtsk_atmp;
	}
	else {
		oldtcb = dtsk_ftmp;
	}
	for (; oldtcb->free_next != NULL; oldtcb = oldtcb->free_next) {
		;
	}
	/*
	* old task freeリストと new task freeリストの連結する
	* コピーはしない
	*/
	oldtcb->free_next = mg_tsk_info.freehead;
	mg_tsk_info.freehead->free_prev = dtsk_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dtsk_ftmp != NULL) {
		mg_tsk_info.freehead = dtsk_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dtsk_atmp != NULL) {
		mg_tsk_info.alochead = dtsk_atmp;
	}
}


/*!
* tskid変換テーブル設定処理(del_tsk():スレッドの排除)
* tskid : 排除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクがすでに未登録状態)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクがその他の状態)
*/
ER kernelrte_del_tsk(ER_ID tskid)
{
	ER ercd;
	
	putcurrent(); /* システムコール発行タスクをレディーへ */

	/* 作成であるacre_tsk()でE_NOIDを返していたならばsta_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_tsk_isr(mg_tsk_info.id_table[tskid]);
		/* 排除できたならば，taskID変換テーブルを初期化 */
		if (ercd == E_OK) {
			mg_tsk_info.id_table[tskid] = NULL;
		}
		
		return ercd;
	}
}


/*!
* tskid変換テーブル設定処理(sta_tsk():スレッドの起動)
* tskid : 起動するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS エラー終了(対象タスクが未登録)
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER kernelrte_sta_tsk(ER_ID tskid)
{
	putcurrent(); /* システムコール発行スレッドをレディーへ */

	/*
	* ・ログの出力(ISR呼び出しでcurrentは切り替わる事があるので，ここの位置でログ出力)
	* ・putcurrent()でレディーデータ構造へ戻した状態でログを出力
	*/
	LOG_CONTEXT(current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばsta_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return sta_tsk_isr(mg_tsk_info.id_table[tskid]);
	}
}


/*!
* システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)と起動)
* func : タスクのメイン関数
* *name : タスクの名前
* priority : タスクの優先度
* stacksize : ユーザスタック(タスクのスタック)のサイズ
* rate : 周期
* rel_exetim : 実行時間(仮想)
* deadtim : デッドライン時刻
* floatim : 余裕時間
* argc : タスクのメイン関数の第一引数
* *argv[] : タスクのメイン関数の第二引数
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)rcd : 自動割付したID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(対象タスクが未登録)
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
static ER_ID kernelrte_run_tsk(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int rel_exetim, int deadtim, int floatim, int argc, char *argv[])
{
	/* システムコール発行タスクをレディーへ戻す処理はISRで行う */

	/* 割込みサービスルーチンの呼び出し */
	return run_tsk_isr(type, func, name, priority, stacksize, rate, rel_exetim, deadtim, floatim, argc, argv);
}


/*!
* tskid変換テーブル設定処理はいらない(ext_tsk():自タスクの終了)
* リターンパラメータはあってはならない
* この関数は他のシステムコールと同様に一貫性を保つために追加した
*/
static void kernelrte_ext_tsk(void)
{
	ext_tsk_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* tskid変換テーブル設定処理はいらない(exd_tsk():自スレッドの終了と排除)
* TCBが排除されるので返却値はなしにする
* この関数は他のシステムコールと同様に一貫性を保つために追加した
*/
static void kernelrte_exd_tsk(void)
{
	exd_tsk_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* tskid変換テーブル設定処理(ter_tsk():スレッドの強制終了)
* tskid : 強制終了するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_ILUSE : エラー終了(タスクが実行状態.つまり自タスク)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了(タスクが実行可能状態または待ち状態)
*/
ER kernelrte_ter_tsk(ER_ID tskid)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	/*
	* ・ログの出力(ISR呼び出しでcurrentは切り替わる事があるので，ここの位置でログ出力)
	* ・putcurrent()でレディーデータ構造へ戻した状態でログを出力
	*/
	LOG_CONTEXT(current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばter_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return ter_tsk_isr(mg_tsk_info.id_table[tskid]);
	}
}


/*!
* tskid変換テーブル設定処理(get_pri():スレッドの優先度取得)
* tskid : 優先度を参照するタスクID
* *p_tskpri : 参照優先度を格納するポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER kernelrte_get_pri(ER_ID tskid, int *p_tskpri)
{
	READY_TYPE type = mg_ready_info.type;

	putcurrent(); /* システムコール発行タスクをレディーへ */

	/* 作成であるacre_tsk()でE_NOIDを返していたならばget_pri()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* スケジューラによって認めているか */
	else if (type == SINGLE_READY_QUEUE) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return get_pri_isr(mg_tsk_info.id_table[tskid], p_tskpri);
	}
}


/*!
* tskid変換テーブル設定処理(chg_pri():スレッドの優先度変更)
* tskid : 優先度を変更するタスクID
* tskpri : 変更する優先度
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
static ER kernelrte_chg_pri(ER_ID tskid, int tskpri)
{
	READY_TYPE r_type = mg_ready_info.type;
	SCHDUL_TYPE s_type = mg_schdul_info.type;

	/* 作成であるacre_tsk()でE_NOIDを返していたならばchg_pri()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* スケジューラによって認めているか(スケジュール属性作った方がいいかな～) */
	else if (r_type == SINGLE_READY_QUEUE || s_type >= RM_SCHEDULING) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return chg_pri_isr(mg_tsk_info.id_table[tskid], tskpri);
	}
}


/*!
* tskid変換テーブル設定処理(chg_slt():タスクタイムスライスの変更)
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
ER kernelrte_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice)
{

	putcurrent(); /* システムコール発行タスクをレディーへ */

	/* 作成であるacre_tsk()でE_NOIDを返していたならばget_pri()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* スケジューラによって認めているか(スケジュール属性作った方がいいかな～) */
	else if (type == FCFS_SCHEDULING || type == PRI_SCHEDULING || type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return chg_slt_isr(type, mg_tsk_info.id_table[tskid], slice);
	}
}


/*!
* tskid変換テーブル設定処理(get_slt():タスクタイムスライスの取得)
* type : スケジューラのタイプ
* tskid : 優先度を参照するタスクID
* *p_slice : タイムスライスを格納するパケットへのポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER kernelrte_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice)
{

	putcurrent(); /* システムコール発行タスクをレディーへ */

	/* 作成であるacre_tsk()でE_NOIDを返していたならばget_pri()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* スケジューラによって認めているか(スケジュール属性作った方がいいかな～) */
	else if (type == FCFS_SCHEDULING || type == PRI_SCHEDULING || type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return get_slt_isr(type, mg_tsk_info.id_table[tskid], p_slice);
	}
}


/*!
* tskid変換テーブル設定処理はいらない(slp_tsk():自タスクの起床待ち)
* (返却値)E_NOSPT : 未サポート
* (返却値)E_OK : 正常終了
*/
static ER kernelrte_slp_tsk(void)
{
	SCHDUL_TYPE type = mg_schdul_info.type;
	
	/* スケジューラによって認めているか */
	if (type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチン呼び出し */
	else {
		return slp_tsk_isr(); /* 割込みサービスルーチンの呼び出し */
	}
}


/*!
* tskid変換テーブル設定処理はいらない(tslp_tsk():自タスクのタイムアウト付き起床待ち)
* msec : タイムアウト値
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : エラー終了(パラメータエラー)
* (返却値)E_OK : 正常終了
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_RLWAI(待ち状態の時にrel_wai()が呼ばれた.
*                (これはrel_wai()側の割込みサービスルーチンで返却値E_OKを書き換える))
*/
static ER kernelrte_tslp_tsk(int msec)
{
	SCHDUL_TYPE type = mg_schdul_info.type;

	/* スケジューラによって認めているか */
	if (type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチン呼び出し */
	else {
		return tslp_tsk_isr(msec); /* 割込みサービスルーチンの呼び出し */
	}
}


/*!
* tskid変換テーブル設定処理(wup_tsk():タスクの起床)
* tskid : タスクの起床するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが休止状態
* (返却値)E_ILUSE : システムコール不正使用(要求タスクが実行状態または，何らかの待ち行列につながれている)
* (返却値)E_OK : 正常終了
*/
static ER kernelrte_wup_tsk(ER_ID tskid)
{

	putcurrent(); /* システムコールを呼び出したスレッドをレディーへ */

	/*
	* ・ログの出力(ISR呼び出しでcurrentは切り替わる事があるので，ここの位置でログ出力)
	* ・putcurrent()でレディーデータ構造へ戻した状態でログを出力
	*/
	LOG_CONTEXT(current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばwup_tsk()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return wup_tsk_isr(mg_tsk_info.id_table[tskid]);
	}
}


/*!
* tskid変換テーブル設定処理(rel_wai():待ち状態強制解除)
* tskid : 待ち状態強制解除するタスクID
* (返却値)E_ID : エラー終了(タスクIDが不正)
* (返却値)E_NOEXS : エラー終了(タスクが未登録状態)
* (返却値)E_OBJ : 対象タスクが待ち状態ではない
* (返却値)E_OK : 正常終了
*/
static ER kernelrte_rel_wai(ER_ID tskid)
{

	putcurrent(); /* システムコールを呼び出したスレッドをレディーへ */

	/*
	* ・ログの出力(ISR呼び出しでcurrentは切り替わる事があるので，ここの位置でログ出力)
	* ・putcurrent()でレディーデータ構造へ戻した状態でログを出力
	*/
	LOG_CONTEXT(current);

	/* 作成であるacre_tsk()でE_NOIDを返していたならばrel_wai()ではE_IDを返却 */
	if (tskid == E_NOID || TASK_ID_NUM << mg_tsk_info.power_count <= tskid) {
		return E_ID;
	}
	/* 対象タスクは存在するか?(すでに排除されていないか) */
	else if (mg_tsk_info.id_table[tskid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return rel_wai_isr(mg_tsk_info.id_table[tskid]);
	}
}


/*!
* tskid変換テーブル設定処理はいらない(dly_tsk():自タスクの遅延)
* この関数は他のシステムコールと同様に一貫性を保つために追加した
* msec : タイムアウト値
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : エラー終了(パラメータエラー)
* (返却値)E_OK : 正常終了
* (返却値)E_RLWAI(待ち状態の時にrel_wai()が呼ばれた.
*                (これはrel_wai()側の割込みサービスルーチンで返却値E_OKを書き換える))
*/
static ER kernelrte_dly_tsk(int msec)
{
	SCHDUL_TYPE type = mg_schdul_info.type;

	/* スケジューラによって認めているか */
	if (type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return dly_tsk_isr(msec); /* 割込みサービスルーチンの呼び出し */
	}
}


/*!
* semid変換テーブル設定処理(acre_sem():セマフォコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* atr : タスクをレディーへ戻すアルゴリズム(FIFO順か?優先度順か?)
* semvalue : セマフォ初期値
* maxvalue : セマフォ最大値
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)semid : 正常終了(作成したセマフォID)
*/
ER_ID kernelrte_acre_sem(SEM_TYPE type, SEM_ATR atr, int semvalue, int maxvalue)
{
	ER_ID semid;
	int semids;
	SCHDUL_TYPE s_type = mg_schdul_info.type;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	semids = SEMAPHORE_ID_NUM << mg_sem_info.power_count; /* 検索範囲の指定 */
	
	/* スケジューラによって認めているか */
	if (s_type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* システムコールの引数は正しいか */
  else if (semvalue < 0 || semvalue > maxvalue || maxvalue <= 0) {
  	return E_PAR;
  }
  /*
  * 割付可能なIDがない(実行されるのはメモリ不足を起こした時のみ)
  * カウンタの数は静的型semaphore配列の個数，動的型semaphoreリストの個数と一致する
  */
  else if (mg_sem_info.counter < 0) {
  	return E_NOID;
  }
	/* 生成できる場合 */
	else {
		/* ISRの呼び出し(mz_acre_sem())とID変換テーブルの設定 */
		mg_sem_info.id_table[mg_sem_info.counter] = (SEMCB *)acre_sem_isr(type, atr, semvalue, maxvalue); /* ID変換テーブルへ設定 */
		semid = (ER_ID)mg_sem_info.counter; /* システムコールのリターンパラメータ型へ変換 */
		mg_sem_info.counter++; /* 次の割付けIDへ(id_table[]のインデックス) */
		/* 次の割付けIDがすでに使用されている場合 */
		if ((semids <= mg_sem_info.counter) || (mg_sem_info.id_table[mg_sem_info.counter] != NULL)) {
			mg_sem_info.counter = (int)serch_semid_table(semids); /* 割付け可能なIDを検索 */
		}
		
		return semid; /* 割付可能なID返却 */
	}
}


/*!
* semaphore ID変換テーブルの次の割付け可能IDを検索をする関数
* semid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* semid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOMEM) : メモリ不足
*/
static ER_ID serch_semid_table(int semids)
{
		ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* semid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < semids; srh_cuntr++) {
		if (mg_sem_info.id_table[srh_cuntr] == NULL) {
			DEBUG_OUTVLE(srh_cuntr, 0);
			DEBUG_OUTMSG(" aaaaaa\n");
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_semid_table() != E_OK) { /* semidが足らなくなった場合 */
		srh_cuntr = -1; /* 次のISR処理でE_NOIDになるようにする */
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = semids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_OUTVLE(srh_cuntr, 0);
  DEBUG_OUTMSG(" sem next counter\n");
  
	return srh_cuntr;
}


/*!
* semaphore IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_semid_table(void)
{
	SEMCB **semtbl_tmp, *ssem_tmp, *dsem_atmp, *dsem_ftmp;
	
	mg_sem_info.power_count++;
	
	/* 現在使用していた領域すべてを一時退避 */
	semtbl_tmp = mg_sem_info.id_table;
	ssem_tmp = mg_sem_info.array;
	dsem_atmp = mg_sem_info.alochead;
	dsem_ftmp = mg_sem_info.freehead; /* この時，必ずmg_sem_info.dsem_freeheadはNULLとはならない(静的型があるため) */
	
	if (sem_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_sem(semtbl_tmp, ssem_tmp, dsem_atmp, dsem_ftmp);
	
	/* 退避していた領域を解放(動的型semaphoreリストは解放しない) */
	rel_mpf_isr(semtbl_tmp);
	rel_mpf_isr(ssem_tmp);
	
	return E_OK;
}


/*!
* semaphore IDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **semtbl_tmp : 古いsemaphoreID変換テーブル
* *ssem_tmp : 古い静的型semaphore可変長配列
* *dsem_atmp : 古い動的型semaphore aloc list head
* *dsem_ftmp : 古い動的型semaphore free list head
*/
static void copy_all_sem(SEMCB **semtbl_tmp,SEMCB *ssem_tmp, SEMCB *dsem_atmp, SEMCB *dsem_ftmp)
{
	int semids;
	
	semids = SEMAPHORE_ID_NUM << (mg_sem_info.power_count - 1); /* 古いsemaphore ID資源数 */
	memcpy(mg_sem_info.id_table, semtbl_tmp, sizeof(&(**semtbl_tmp)) * semids); /* 可変長配列なのでコピー */
	
	copy_ssem(ssem_tmp, semids); /* 静的型semaphore配列のコピーとsemaphoreID変換テーブル更新 */
	copy_dsem(dsem_atmp, dsem_ftmp, semids); /* 動的型semaphoreリストの連結と更新(コピーはしない) */
}


/*!
* semaphore IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数
* semids : 古いsemaphore ID資源数
* *ssem_tmp : 古い静的型semaphore可変長配列
*/
static void copy_ssem(SEMCB *ssem_tmp, int semids)
{
	int i;
	
	memcpy(mg_sem_info.array, ssem_tmp, sizeof(*(mg_sem_info.array)) * semids); /* 可変長配列なのでコピー */
	
	/* 静的型semaphore配列をコピーしたので，それに応じてsemaphoreID変換テーブルを更新 */
	for (i = 0; i < semids; i++) {
		if (mg_sem_info.array[i].semid != -1) {
			mg_sem_info.id_table[i] = &mg_sem_info.array[i]; /* 変換テーブルの更新 */
		}
	}
}


/*!
* semaphore IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* * semids : 古いsemaphore ID資源数
* *dsem_atmp : 古い動的型semaphore aloc list head
* *dsem_ftmp : 古い動的型semaphore free list head
*/
static void copy_dsem(SEMCB *dsem_atmp, SEMCB *dsem_ftmp, int semids)
{
	SEMCB *oldscb;
	
	/*
	* 動的型semaphoreはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型semaphore freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dsem_ftmp == NULL) {
		oldscb = dsem_atmp;
	}
	else {
		oldscb = dsem_ftmp;
	}
	for (; oldscb->next != NULL; oldscb = oldscb->next) {
		;
	}
	/*
	* old semaphore freeリストと new semaphore freeリストの連結する
	* コピーはしない
	*/
	oldscb->next = mg_sem_info.freehead;
	mg_sem_info.freehead->prev = dsem_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dsem_ftmp != NULL) {
		mg_sem_info.freehead = dsem_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dsem_atmp != NULL) {
		mg_sem_info.alochead = dsem_atmp;
	}
}


/*!
* semid変換テーブル設定処理(del_sem():セマフォコントロールブロックの排除)
* semid : 排除するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER kernelrte_del_sem(ER_ID semid)
{
	ER ercd;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_sem()でE_NOIDを返していたならばdel_sem()ではE_IDを返却 */
	if (semid == E_NOID || SEMAPHORE_ID_NUM << mg_sem_info.power_count <= semid) {
		return E_ID;
	}
	/* 対象セマフォは存在するか?(すでに排除されていないか) */
	else if (mg_sem_info.id_table[semid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_sem_isr(mg_sem_info.id_table[semid]);
		/* 排除できたならば，semaphoreID変換テーブルを初期化 */
		if (ercd == E_OK) {
			mg_sem_info.id_table[semid] = NULL;
		}
		
		return ercd;
	}
}


/*!
* semid変換テーブル設定処理(sig_sem():セマフォV操作)
* semid : V操作するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(セマフォの解放，待ちタスクへの割り当て)
* (返却値)E_QOVR : キューイングオーバフロー(セマフォ解放エラー(すでに解放済み))，上限値を越えた
*/
static ER kernelrte_sig_sem(ER_ID semid)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */
	
	/* 作成であるacre_alm()でE_NOIDを返していたならばsig_sem()ではE_IDを返却 */
	if (semid == E_NOID || SEMAPHORE_ID_NUM << mg_sem_info.power_count <= semid) {
		return E_ID;
	}
	/* 対象セマフォは存在するか?(すでに排除されていないか) */
	else if (mg_sem_info.id_table[semid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return sig_sem_isr(mg_sem_info.id_table[semid]);
	}
}


/*!
* semid変換テーブル設定処理(wai_sem():セマフォP操作)
* semid : P操作するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
*/
static ER kernelrte_wai_sem(ER_ID semid)
{
	/* ここではシステムコール発行タスクはレディーへ戻さない */

	/* 作成であるacre_sem()でE_NOIDを返していたならばwai_sem()ではE_IDを返却 */
	if (semid == E_NOID || SEMAPHORE_ID_NUM << mg_sem_info.power_count <= semid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象セマフォは存在するか?(すでに排除されていないか) */
	else if (mg_sem_info.id_table[semid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return wai_sem_isr(mg_sem_info.id_table[semid]);
	}
}


/*!
* semid変換テーブル設定処理(pol_sem():セマフォP操作(ポーリング))
* semid : P操作(ポーリング)するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
* (返却値)E_TMOUT : セマフォ待ち失敗(ポーリング)
*/
static ER kernelrte_pol_sem(ER_ID semid)
{
	/* ここではシステムコール発行タスクはレディーへ戻さない */

	/* 作成であるacre_alm()でE_NOIDを返していたならばpol_sem()ではE_IDを返却 */
	if (semid == E_NOID || SEMAPHORE_ID_NUM << mg_sem_info.power_count <= semid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象セマフォは存在するか?(すでに排除されていないか) */
	else if (mg_sem_info.id_table[semid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return pol_sem_isr(mg_sem_info.id_table[semid]);
	}
}


/*!
* semid変換テーブル設定処理(twai_sem():セマフォP操作(タイムアウト付き))
* semid : P操作(タイムアウト付き)するsemid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : セマフォオブジェクト未登録(すでに排除済み)
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
* (返却値)E_TMOUT : タイムアウト
*/
static ER kernelrte_twai_sem(ER_ID semid, int msec)
{
	/* ここではシステムコール発行タスクはレディーへ戻さない */

	/* 作成であるacre_alm()でE_NOIDを返していたならばtwai_sem()ではE_IDを返却 */
	if (semid == E_NOID || SEMAPHORE_ID_NUM << mg_sem_info.power_count <= semid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象セマフォは存在するか?(すでに排除されていないか) */
	else if (mg_sem_info.id_table[semid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return twai_sem_isr(mg_sem_info.id_table[semid], msec);
	}
}


/*!
* mbxid変換テーブル設定処理(acre_mbx():メールボックスコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* atr : タスクをレディーへ戻すアルゴリズム(FIFO順か?優先度順か?)
* max_msgpri : 送信されるメッセージ優先度の最大値
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mbxid : 正常終了(作成したメールボックスID)
*/
ER_ID kernelrte_acre_mbx(MBX_TYPE type, MBX_MATR msg_atr, MBX_WATR wai_atr, int max_msgpri)
{
	ER_ID mbxid;
	int mbxids;
	SCHDUL_TYPE s_type = mg_schdul_info.type;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	mbxids = MAILBOX_ID_NUM << mg_mbx_info.power_count; /* 検索範囲の指定 */
	
	/* スケジューラによって認めているか */
	if (s_type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* システムコールの引数は正しいか */
  else if (0 > max_msgpri || max_msgpri >= MASSAGE_PRIORITY_NUM) {
  	return E_PAR;
  }
  /*
  * 割付可能なIDがない(実行されるのはメモリ不足を起こした時のみ)
  * カウンタの数は静的型mailbox配列の個数，動的型mailboxリストの個数と一致する
  */
  else if (mg_mbx_info.counter < 0) {
  	return E_NOID;
  }
	/* 生成できる場合 */
	else {
		/* ISRの呼び出し(mz_acre_mbx())とID変換テーブルの設定 */
		mg_mbx_info.id_table[mg_mbx_info.counter] = (MBXCB *)acre_mbx_isr(type, msg_atr, wai_atr, max_msgpri);
		mbxid = (ER_ID)mg_mbx_info.counter; /* システムコールのリターンパラメータ型へ変換 */
		mg_mbx_info.counter++; /* 次の割付けIDへ(id_table[]のインデックス) */
		/* 次の割付けIDがすでに使用されている場合 */
		if ((mbxids <= mg_mbx_info.counter) || (mg_mbx_info.id_table[mg_mbx_info.counter] != NULL)) {
			mg_mbx_info.counter = (int)serch_mbxid_table(mbxids); /* 割付け可能なIDを検索 */
		}
		
		return mbxid; /* 割付可能なID返却 */
	}
}


/*!
* mailbox ID変換テーブルの次の割付け可能IDを検索をする関数
* mbxid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* mbxid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOMEM) : メモリ不足
*/
static ER_ID serch_mbxid_table(int mbxids)
{
		ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* mbxid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < mbxids; srh_cuntr++) {
		if (mg_mbx_info.id_table[srh_cuntr] == NULL) {
			DEBUG_OUTVLE(srh_cuntr, 0);
			DEBUG_OUTMSG(" aaaaaa\n");
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_mbxid_table() != E_OK) { /* mbxidが足らなくなった場合 */
		srh_cuntr = -1; /* 次のISR処理でE_NOIDになるようにする */
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = mbxids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_OUTVLE(srh_cuntr, 0);
  DEBUG_OUTMSG(" mbx next counter\n");
  
	return srh_cuntr;
}


/*!
* mailbox IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_mbxid_table(void)
{
	MBXCB **mbxtbl_tmp, *smbx_tmp, *dmbx_atmp, *dmbx_ftmp;
	
	mg_mbx_info.power_count++;
	
	/* 現在使用していた領域すべてを一時退避 */
	mbxtbl_tmp = mg_mbx_info.id_table;
	smbx_tmp = mg_mbx_info.array;
	dmbx_atmp = mg_mbx_info.alochead;
	dmbx_ftmp = mg_mbx_info.freehead; /* この時，必ずmg_mbx_info.freeheadはNULLとはならない(静的型があるため) */
	
	if (mbx_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_mbx(mbxtbl_tmp, smbx_tmp, dmbx_atmp, dmbx_ftmp);
	
	/* 退避していた領域を解放(動的型mailboxリストは解放しない) */
	rel_mpf_isr(mbxtbl_tmp);
	rel_mpf_isr(smbx_tmp);
	
	return E_OK;
}


/*!
* mailbox IDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **mbxtbl_tmp : 古いmailboxID変換テーブル
* *smbx_tmp : 古い静的型mailbox可変長配列
* *dmbx_atmp : 古い動的型mailbox aloc list head
* *dmbx_ftmp : 古い動的型mailbox free list head
*/
static void copy_all_mbx(MBXCB **mbxtbl_tmp, MBXCB *smbx_tmp, MBXCB *dmbx_atmp, MBXCB *dmbx_ftmp)
{
	int mbxids;
	
	mbxids = MAILBOX_ID_NUM << (mg_mbx_info.power_count - 1); /* 古いmailbox ID資源数 */
	memcpy(mg_mbx_info.id_table, mbxtbl_tmp, sizeof(&(**mbxtbl_tmp)) * mbxids); /* 可変長配列なのでコピー */
	
	copy_smbx(smbx_tmp, mbxids); /* 静的型mailbox配列のコピーとmailboxID変換テーブル更新 */
	copy_dmbx(dmbx_atmp, dmbx_ftmp, mbxids); /* 動的型mailboxリストの連結と更新(コピーはしない) */
}


/*!
* mailbox IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数
* mbxids : 古いmailbox ID資源数
* *smbx_tmp : 古い静的型mailbox可変長配列
*/
static void copy_smbx(MBXCB *smbx_tmp, int mbxids)
{
	int i;
	
	memcpy(mg_mbx_info.array, smbx_tmp, sizeof(*(mg_mbx_info.array)) * mbxids); /* 可変長配列なのでコピー */
	
	/* 静的型mailbox配列をコピーしたので，それに応じてmailboxID変換テーブルを更新 */
	for (i = 0; i < mbxids; i++) {
		if (mg_mbx_info.array[i].mbxid != -1) {
			mg_mbx_info.id_table[i] = &mg_mbx_info.array[i]; /* 変換テーブルの更新 */
		}
	}
}


/*!
* mailbox IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* * mbxids : 古いmailbox ID資源数
* *dmbx_atmp : 古い動的型mailbox aloc list head
* *dmbx_ftmp : 古い動的型mailbox free list head
*/
static void copy_dmbx(MBXCB *dmbx_atmp, MBXCB *dmbx_ftmp, int mbxids)
{
	MBXCB *oldmbcb;
	
	/*
	* 動的型mailboxはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型mailbox freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dmbx_ftmp == NULL) {
		oldmbcb = dmbx_atmp;
	}
	else {
		oldmbcb = dmbx_ftmp;
	}
	for (; oldmbcb->next != NULL; oldmbcb = oldmbcb->next) {
		;
	}
	/*
	* old mailbox freeリストと new mailbox freeリストの連結する
	* コピーはしない
	*/
	oldmbcb->next = mg_mbx_info.freehead;
	mg_mbx_info.freehead->prev = dmbx_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dmbx_ftmp != NULL) {
		mg_mbx_info.freehead = dmbx_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dmbx_atmp != NULL) {
		mg_mbx_info.alochead = dmbx_atmp;
	}
}


/*!
* mbxid変換テーブル設定処理(del_mbx():メールボックスコントロールブロックの排除)
* mbxid : 排除するmbxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : メールボックス取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER kernelrte_del_mbx(ER_ID mbxid)
{
	ER ercd;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_mbx()でE_NOIDを返していたならばdel_mbx()ではE_IDを返却 */
	if (mbxid == E_NOID || MAILBOX_ID_NUM << mg_mbx_info.power_count <= mbxid) {
		return E_ID;
	}
	/* 対象メールボックスは存在するか?(すでに排除されていないか) */
	else if (mg_mbx_info.id_table[mbxid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_mbx_isr(mg_mbx_info.id_table[mbxid]);
		/* 排除できたならば，mailboxID変換テーブルを初期化 */
		if (ercd == E_OK) {
			mg_mbx_info.id_table[mbxid] = NULL;
		}
		
		return ercd;
	}
}


/*!
* mbxid変換テーブル設定処理(snd_mbx():メールボックスへ送信)
* mbxid : 送信するmbxid
* *pk_msg : 送信するメッセージパケット先頭番地
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージをキューイング,メッセージをタスクへ送信)
*/
static ER kernelrte_snd_mbx(ER_ID mbxid, T_MSG *pk_msg)
{
	/* 送信側のタスクは待ちにならない */
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* パラメータチェック */
	if (pk_msg == NULL || pk_msg->msgpri < 0 || MASSAGE_PRIORITY_NUM <= pk_msg->msgpri) {
		return E_PAR;
	}
	/* 作成であるacre_mbx()でE_NOIDを返していたならばsnd_mbx()ではE_IDを返却 */
	else if (mbxid == E_NOID || MAILBOX_ID_NUM << mg_mbx_info.power_count <= mbxid) {
		return E_ID;
	}
	/* 対象メールボックスは存在するか?(すでに排除されていないか) */
	else if (mg_mbx_info.id_table[mbxid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return snd_mbx_isr(mg_mbx_info.id_table[mbxid], pk_msg);
	}
}


/*!
* mbxid変換テーブル設定処理(rcv_mbx():メールボックスからの受信)
* mbxid : 受信するmbxid
* **pk_msg : 受信するメッセージパケットの先頭番地
* ・意味がないので，E_PAR(ppk_msgのチェックはしない)
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージがなく待ちタスクとなった，メッセージをタスクに与えた)
*/
static ER kernelrte_rcv_mbx(ER_ID mbxid, T_MSG **pk_msg)
{
	/* ここではシステムコール発行スレッドをレディーへは戻さない */
	
	/* 作成であるacre_mbx()でE_NOIDを返していたならばrcv_mbx()ではE_IDを返却 */
	if (mbxid == E_NOID || MAILBOX_ID_NUM << mg_mbx_info.power_count <= mbxid) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_ID;
	}
	/* 対象メールボックスは存在するか?(すでに排除されていないか) */
	else if (mg_mbx_info.id_table[mbxid] == NULL) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return rcv_mbx_isr(mg_mbx_info.id_table[mbxid], pk_msg);
	}
}


/*!
* mbxid変換テーブル設定処理(prcv_mbx():ポーリング付きメールボックスからの受信)
* mbxid : 受信するmbxid
* **pk_msg : 受信するメッセージパケットの先頭番地
* ・意味がないので，E_PAR(ppk_msgのチェックはしない)
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージをタスクに与えた)
* (返却値)E_TMOUT : メッセージ待ち失敗(ポーリング)
*/
static ER kernelrte_prcv_mbx(ER_ID mbxid, T_MSG **pk_msg)
{

	/* システムコール発行スレッドをレディーへ(ポーリングsystem callはタスクは待ちにならないため，ここで戻す) */
	putcurrent();
	
	/* 作成であるacre_mbx()でE_NOIDを返していたならばprcv_mbx()ではE_IDを返却 */
	if (mbxid == E_NOID || MAILBOX_ID_NUM << mg_mbx_info.power_count <= mbxid) {
		return E_ID;
	}
	/* 対象メールボックスは存在するか?(すでに排除されていないか) */
	else if (mg_mbx_info.id_table[mbxid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return prcv_mbx_isr(mg_mbx_info.id_table[mbxid], pk_msg);
	}
}


/*!
* mbxid変換テーブル設定処理(trcv_mbx():タイムアウト付きメールボックスからの受信)
* mbxid : 受信するmbxid
* **pk_msg : 受信するメッセージパケットの先頭番地
* tmout : タイムアウト値
* (返却値)E_PAR : パラメータエラー
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : メールボックスオブジェクト未登録(すでに排除済み)
* (返却値)E_OK : 正常終了(メッセージがなく待ちタスクとなった，メッセージをタスクに与えた)
* (返却値)E_TMOUT : メッセージ待ち失敗(ポーリング)
*/
static ER kernelrte_trcv_mbx(ER_ID mbxid, T_MSG **pk_msg, int tmout)
{
	/* ここではシステムコール発行スレッドをレディーへは戻さない */
	
	/* パラメータチェック */
	if (tmout < TMO_FEVR) {
		return E_PAR;
	}
	/* 作成であるacre_mbx()でE_NOIDを返していたならばtrcv_mbx()ではE_IDを返却 */
	else if (mbxid == E_NOID || MAILBOX_ID_NUM << mg_mbx_info.power_count <= mbxid) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_ID;
	}
	/* 対象メールボックスは存在するか?(すでに排除されていないか) */
	else if (mg_mbx_info.id_table[mbxid] == NULL) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return trcv_mbx_isr(mg_mbx_info.id_table[mbxid], pk_msg, tmout);
	}
}


/*!
* mtxid変換テーブル設定処理(acre_mtx():mutexコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* atr : タスクをレディーへ戻すアルゴリズム(FIFO順か?優先度順か?)
* piver_mtx : 優先度逆転機構の選択
* maxlocks : 再帰ロックの上限値
* pcl_param : 優先度逆転プロトコル適用時のパラメータ(必要としない優先度逆転プロトコルもある)
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mtxid : 正常終了(作成したミューテックスID)
*/
ER_ID kernelrte_acre_mtx(MTX_TYPE type, MTX_ATR atr, PIVER_TYPE piver_type, int maxlocks, int pcl_param)
{
	ER_ID mtxid;
	int mtxids;
	SCHDUL_TYPE s_type = mg_schdul_info.type;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	mtxids = MUTEX_ID_NUM << mg_mtx_info.power_count; /* 検索範囲の指定 */
	
	/* スケジューラによって認めているか */
	if (s_type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* システムコールの引数は正しいか */
  else if (maxlocks <= 0) {
  	return E_PAR;
  }
  /*
  * 割付可能なIDがない(実行されるのはメモリ不足を起こした時のみ)
  * カウンタの数は静的型mutex配列の個数，動的型mutexリストの個数と一致する
  */
  else if (mg_mtx_info.counter < 0) {
  	return E_NOID;
  }
	/* 生成できる場合 */
	else {
		/* ISRの呼び出し(mz_acre_mtx())とID変換テーブルの設定 */
		mg_mtx_info.id_table[mg_mtx_info.counter] = (MTXCB *)acre_mtx_isr(type, atr, piver_type, maxlocks, pcl_param);
		mtxid = (ER_ID)mg_mtx_info.counter; /* システムコールのリターンパラメータ型へ変換 */
		mg_mtx_info.counter++; /* 次の割付けIDへ(id_table[]のインデックス) */
		/* 次の割付けIDがすでに使用されている場合 */
		if ((mtxids <= mg_mtx_info.counter) || (mg_mtx_info.id_table[mg_mtx_info.counter] != NULL)) {
			mg_mtx_info.counter = (int)serch_mtxid_table(mtxids); /* 割付け可能なIDを検索 */
		}
		
		return mtxid; /* 割付可能なID返却 */
	}
}


/*!
* mutex ID変換テーブルの次の割付け可能IDを検索をする関数
* mtxid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* mtxid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOMEM) : メモリ不足
*/
static ER_ID serch_mtxid_table(int mtxids)
{
		ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* alarmid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < mtxids; srh_cuntr++) {
		if (mg_mtx_info.id_table[srh_cuntr] == NULL) {
			DEBUG_OUTVLE(srh_cuntr, 0);
			DEBUG_OUTMSG(" aaaaaa\n");
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_mtxid_table() != E_OK) { /* mtxidが足らなくなった場合 */
		srh_cuntr = -1; /* 次のISR処理でE_NOIDになるようにする */
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = mtxids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_OUTVLE(srh_cuntr, 0);
  DEBUG_OUTMSG(" mtx next counter\n");
  
	return srh_cuntr;
}


/*!
* mutex IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_mtxid_table(void)
{
	MTXCB **mtxtbl_tmp, *smtx_tmp, *dmtx_atmp, *dmtx_ftmp;
	
	mg_mtx_info.power_count++;
	
	/* 現在使用していた領域すべてを一時退避 */
	mtxtbl_tmp = mg_mtx_info.id_table;
	smtx_tmp = mg_mtx_info.array;
	dmtx_atmp = mg_mtx_info.alochead;
	dmtx_ftmp = mg_mtx_info.freehead; /* この時，必ずmg_mtx_info.freeheadはNULLとはならない(静的型があるため) */
	
	if (mtx_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_mtx(mtxtbl_tmp, smtx_tmp, dmtx_atmp, dmtx_ftmp);
	
	/* 退避していた領域を解放(動的型mutexリストは解放しない) */
	rel_mpf_isr(mtxtbl_tmp);
	rel_mpf_isr(smtx_tmp);
	
	return E_OK;
}


/*!
* mutex IDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **mtxtbl_tmp : 古いmutexID変換テーブル
* *smtx_tmp : 古い静的型mutex可変長配列
* *dmtx_atmp : 古い動的型mutex aloc list head
* *dmtx_ftmp : 古い動的型mutex free list head
*/
static void copy_all_mtx(MTXCB **mtxtbl_tmp, MTXCB *smtx_tmp, MTXCB *dmtx_atmp, MTXCB *dmtx_ftmp)
{
	int mtxids;
	
	mtxids = MUTEX_ID_NUM << (mg_mtx_info.power_count - 1); /* 古いmutex ID資源数 */
	memcpy(mg_mtx_info.id_table, mtxtbl_tmp, sizeof(&(**mtxtbl_tmp)) * mtxids); /* 可変長配列なのでコピー */
	
	copy_smtx(smtx_tmp, mtxids); /* 静的型mutex配列のコピーとmutexID変換テーブル更新 */
	copy_dmtx(dmtx_atmp, dmtx_ftmp, mtxids); /* 動的型mutexリストの連結と更新(コピーはしない) */
}


/*!
* mutex IDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数
* mtxids : 古いmutex ID資源数
* *smtx_tmp : 古い静的型mutex可変長配列
*/
static void copy_smtx(MTXCB *smtx_tmp, int mtxids)
{
	int i;
	
	memcpy(mg_mtx_info.array, smtx_tmp, sizeof(*(mg_mtx_info.array)) * mtxids); /* 可変長配列なのでコピー */
	
	/* 静的型mutex配列をコピーしたので，それに応じてmutex変換テーブルを更新 */
	for (i = 0; i < mtxids; i++) {
		if (mg_mtx_info.array[i].mtxid != -1) {
			mg_mtx_info.id_table[i] = &mg_mtx_info.array[i]; /* 変換テーブルの更新 */
		}
	}
}


/*!
* mutex IDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* * mtxids : 古いmutex ID資源数
* *dmtx_atmp : 古い動的型mutex aloc list head
* *dmtx_ftmp : 古い動的型mutex free list head
*/
static void copy_dmtx(MTXCB *dmtx_atmp, MTXCB *dmtx_ftmp, int mtxids)
{
	MTXCB *oldmcb;
	
	/*
	* 動的型mutexはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型mutex freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dmtx_ftmp == NULL) {
		oldmcb = dmtx_atmp;
	}
	else {
		oldmcb = dmtx_ftmp;
	}
	for (; oldmcb->next != NULL; oldmcb = oldmcb->next) {
		;
	}
	/*
	* old mutex freeリストと new mutex freeリストの連結する
	* コピーはしない
	*/
	oldmcb->next = mg_mtx_info.freehead;
	mg_mtx_info.freehead->prev = dmtx_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dmtx_ftmp != NULL) {
		mg_mtx_info.freehead = dmtx_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dmtx_atmp != NULL) {
		mg_mtx_info.alochead = dmtx_atmp;
	}
}


/*!
* mtxid変換テーブル設定処理(del_mtx():mutexコントロールブロックの排除)
* mtxid : 排除するmutexid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER kernelrte_del_mtx(ER_ID mtxid)
{
	ER ercd;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_mtx()でE_NOIDを返していたならばdel_mtx()ではE_IDを返却 */
	if (mtxid == E_NOID || MUTEX_ID_NUM << mg_mtx_info.power_count <= mtxid) {
		return E_ID;
	}
	/* 対象セマフォは存在するか?(すでに排除されていないか) */
	else if (mg_mtx_info.id_table[mtxid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_mtx_isr(mg_mtx_info.id_table[mtxid]);
		/* 排除できたならば，mutexID変換テーブルを初期化 */
		if (ercd == E_OK) {
			mg_mtx_info.id_table[mtxid] = NULL;
		}
		
		return ercd;
	}
}


/*!
* mtxid変換テーブル設定処理(loc_mtx():mutexロック操作)
* mtxid : ロック操作するmtxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
static ER kernelrte_loc_mtx(ER_ID mtxid)
{
	/* ここではシステムコール発行タスクはレディーへ戻さない */

	/* 作成であるacre_mtx()でE_NOIDを返していたならばloc_mtx()ではE_IDを返却 */
	if (mtxid == E_NOID || MUTEX_ID_NUM << mg_mtx_info.power_count <= mtxid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象mutexは存在するか?(すでに排除されていないか) */
	else if (mg_mtx_info.id_table[mtxid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return check_loc_mtx_protocol(mg_mtx_info.id_table[mtxid]);
	}
}


/*!
* mtxid変換テーブル設定処理(ploc_mtx():mutexポーリングロック操作)
* mtxid : ポーリングロック操作するmtxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_NOSPT : プロトコルありでのpol_mtx()は認めない(check_ploc_mtx_protocol()の返却値)
*/
static ER kernelrte_ploc_mtx(ER_ID mtxid)
{
	/* ここではシステムコール発行タスクはレディーへ戻さない */

	/* 作成であるacre_alm()でE_NOIDを返していたならばploc_mtx()ではE_IDを返却 */
	if (mtxid == E_NOID || MUTEX_ID_NUM << mg_mtx_info.power_count <= mtxid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象mutexは存在するか?(すでに排除されていないか) */
	else if (mg_mtx_info.id_table[mtxid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return check_ploc_mtx_protocol(mg_mtx_info.id_table[mtxid]);
	}
}


/*!
* mtxid変換テーブル設定処理(tloc_mtx():mutexタイムアウト付きロック操作)
* mtxid : ポーリングロック操作するmtxid
* msec : タイムアウト値
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了(mutexセマフォを取得，mutexセマフォ待ちタスクにつなげる)
* (返却値)E_OK，E_ILUSE : dynamic_multipl_lock()の返却値(再帰ロック完了，多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_NOSPT : プロトコルありでのtloc_mtx()は認めない
*/
static ER kernelrte_tloc_mtx(ER_ID mtxid, int msec)
{
	/* ここではシステムコール発行タスクはレディーへ戻さない */

	/* 作成であるacre_alm()でE_NOIDを返していたならばtloc_mtx()ではE_IDを返却 */
	if (mtxid == E_NOID || MUTEX_ID_NUM << mg_mtx_info.power_count <= mtxid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象mutexは存在するか?(すでに排除されていないか) */
	else if (mg_mtx_info.id_table[mtxid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return check_tloc_mtx_protocol(mg_mtx_info.id_table[mtxid], msec);
	}
}


/*!
* mtxid変換テーブル設定処理(unl_mtx():mutexアンロック操作)
* mtxid : アンロック操作するmtxid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : mutexオブジェクト未登録(すでに排除済み)
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
static ER kernelrte_unl_mtx(ER_ID mtxid)
{
	/* ここではシステムコール発行タスクをレディーへ(優先度逆転機構もこの関数を使用するため) */
	
	/* 作成であるacre_alm()でE_NOIDを返していたならばunl_mtx()ではE_IDを返却 */
	if (mtxid == E_NOID || MUTEX_ID_NUM << mg_mtx_info.power_count <= mtxid) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_ID;
	}
	/* 対象mutexは存在するか?(すでに排除されていないか) */
	else if (mg_mtx_info.id_table[mtxid] == NULL) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return check_unl_mtx_protocol(mg_mtx_info.id_table[mtxid]);
	}
}


/* システムコール処理(get_mpf():動的メモリ獲得) */
static void* kernelrte_get_mpf(int size)
{
  putcurrent(); /* システムコール発行タスクをレディーへ */
  return get_mpf_isr(size);
}


/* システムコール処理(rel_mpf():動的メモリ解放) */
static int kernelrte_rel_mpf(char *p)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */
  rel_mpf_isr(p);
  return 0;
}


/*!
* cycid変換テーブル設定処理(acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付))
* この関数からacre_cyc()のISRを呼ぶ
* IDの割付は0から順番に行う.最大数までいった時は，0に再度戻り検索する(途中がdeleteされている事があるため)
* type : 静的型か動的型か？
* *exinf : 周期ハンドラに渡すパラメータ
* cyctim : 起動周期(msec)
* func : 周期ハンドラ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)cycid : 正常終了(作成したアラームハンドラID)
*/
ER_ID kernelrte_acre_cyc(CYC_TYPE type, void *exinf, int cyctim, TMR_CALLRTE func)
{
	ER_ID cycid;
	int cycids;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	cycids = CYCLE_ID_NUM << mg_cyc_info.power_count; /* 検索範囲の指定 */
	
	/*システムコールの引数は正しいか*/
  if (!func || cyctim <= 0) {
  	return E_PAR;
  }
  /*
  * 割付可能なIDがない(実行されるのはメモリ不足を起こした時のみ)
  * カウンタの数は静的型cycle handler配列の個数，動的型cycle handlerリストの個数と一致する
  */
  else if (mg_cyc_info.counter < 0) {
  	return E_NOID;
  }
	/* 生成できる場合 */
	else {
		/* ISRの呼び出し(mz_acre_cyc())とID変換テーブルの設定 */
		mg_cyc_info.id_table[mg_cyc_info.counter] = (CYCCB *)acre_cyc_isr(type, exinf, cyctim, func);
		cycid = (ER_ID)mg_cyc_info.counter; /* システムコールのリターンパラメータ型へ変換 */
		mg_cyc_info.counter++; /* 次の割付けIDへ(mg_cycleid_table[]のインデックス) */
		/* 次の割付けIDがすでに使用されている場合 */
		if ((cycids <= mg_cyc_info.counter) || (mg_cyc_info.id_table[mg_cyc_info.counter] != NULL)) {
			mg_cyc_info.counter = (int)serch_cycid_table(cycids); /* 割付け可能なIDを検索 */
		}
		
		return cycid; /* 割付可能なID返却 */
	}
}


/*!
* cycleid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* cycid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOMEM) : メモリ不足
*/
static ER_ID serch_cycid_table(int cycids)
{
	ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* cycleid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < cycids; srh_cuntr++) {
		if (mg_cyc_info.id_table[srh_cuntr] == NULL) {
			DEBUG_OUTVLE(srh_cuntr, 0);
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_cycid_table() != E_OK) { /* cycidが足らなくなった場合 */
		srh_cuntr = -1; /* 次のISR処理でE_NOIDになるようにする */
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = cycids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_OUTVLE(srh_cuntr, 0);
  DEBUG_OUTMSG(" cyc next counter\n");
  
	return srh_cuntr;
}


/*!
* cycle handler IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_cycid_table(void)
{
	CYCCB **cyctbl_tmp, *scyc_tmp, *dcyc_atmp, *dcyc_ftmp;
	
	mg_cyc_info.power_count++;
	
	/* 現在使用していた領域すべてを一時退避 */
	cyctbl_tmp = mg_cyc_info.id_table;
	scyc_tmp = mg_cyc_info.array;
	dcyc_atmp = mg_cyc_info.alochead;
	dcyc_ftmp = mg_cyc_info.freehead; /* この時，必ずmg_cyc_info.freeheadはNULLとはならない(静的型があるため) */
	
	if (cyc_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_cyc(cyctbl_tmp, scyc_tmp, dcyc_atmp, dcyc_ftmp);
	
	/* 退避していた領域を解放(動的型cycle handlerリストは解放しない) */
	rel_mpf_isr(cyctbl_tmp);
	rel_mpf_isr(scyc_tmp);
	
	return E_OK;
}


/*!
* cycle handlerIDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **cyctbl_tmp : 古いcycle handlerID変換テーブル
* *scyc_tmp : 古い静的型cycle handler可変長配列
* *dcyc_atmp : 古い動的型cycle handler aloc list head
* *cyc_ftmp : 古い動的型cycle handler free list head
*/
static void copy_all_cyc(CYCCB **cyctbl_tmp, CYCCB *scyc_tmp, CYCCB *dcyc_atmp, CYCCB *dcyc_ftmp)
{
	int cycids;
	
	cycids = CYCLE_ID_NUM << (mg_cyc_info.power_count - 1); /* 古いcycle handler ID資源数 */
	memcpy(mg_cyc_info.id_table, cyctbl_tmp, sizeof(&(**cyctbl_tmp)) * cycids); /* 可変長配列なのでコピー */
	
	copy_scyc(scyc_tmp, cycids); /* 静的型cycle handler配列のコピーとcycle handlerID変換テーブル更新 */
	copy_dcyc(dcyc_atmp, dcyc_ftmp, cycids); /* 動的型cycle handlerリストの連結と更新(コピーはしない) */
}


/*!
* cycle handlerIDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数
* cycids : 古いcycle handler ID資源数
* *scyc_tmp : 古い静的型cycle handler可変長配列
*/
static void copy_scyc(CYCCB *scyc_tmp, int cycids)
{
	int i;
	
	memcpy(mg_cyc_info.array, scyc_tmp, sizeof(*(mg_cyc_info.array)) * cycids); /* 可変長配列なのでコピー */
	
	/* 静的型cycle handler配列をコピーしたので，それに応じてcycle handlerID変換テーブルを更新 */
	for (i = 0; i < cycids; i++) {
		if (mg_cyc_info.array[i].cycid != -1) {
			mg_cyc_info.id_table[i] = &mg_cyc_info.array[i]; /* 変換テーブルの更新 */
		}
	}
}

/*!
* cycle handlerIDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* cycids : 古いcycle handler ID資源数
* *dcyc_atmp : 古い動的型cycle handler aloc list head
* *dcyc_ftmp : 古い動的型cycle handler free list head
*/
static void copy_dcyc(CYCCB *dcyc_atmp, CYCCB *dcyc_ftmp, int cycids)
{
	CYCCB *oldcycb;
	
	/*
	* 動的型cycle handlerはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型cycle handler freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dcyc_ftmp == NULL) {
		oldcycb = dcyc_atmp;
	}
	else {
		oldcycb = dcyc_ftmp;
	}
	for (; oldcycb->next != NULL; oldcycb = oldcycb->next) {
		;
	}
	/*
	* old cycle handler freeリストと new cycle handler freeリストの連結する
	* コピーはしない
	*/
	oldcycb->next = mg_cyc_info.freehead;
	mg_cyc_info.freehead->prev = dcyc_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dcyc_ftmp != NULL) {
		mg_cyc_info.freehead = dcyc_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dcyc_atmp != NULL) {
		mg_cyc_info.alochead = dcyc_atmp;
	}
}


/*!
* cycid変換テーブル設定処理(del_cyc():周期ハンドラコントロールブロックの排除)
* cycid : 排除するcycid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : 周期ハンドラオブジェクト未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER kernelrte_del_cyc(ER_ID cycid)
{
	ER ercd;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_cyc()でE_NOIDを返していたならばdel_cyc()ではE_IDを返却 */
	if (cycid == E_NOID || CYCLE_ID_NUM << mg_cyc_info.power_count <= cycid) {
		return E_ID;
	}
	/* 対象周期ハンドラは存在するか?(すでに排除されていないか) */
	else if (mg_cyc_info.id_table[cycid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_cyc_isr(mg_cyc_info.id_table[cycid]);
		/* 排除できたならば，cycle handlerID変換テーブルを初期化 */
		if (ercd == E_OK) {
			mg_cyc_info.id_table[cycid] = NULL;
		}
		
		return ercd;
	}
}


/*!
* cycid変換テーブル設定処理(sta_cyc():周期ハンドラの動作開始)
* cycid : 動作開始する周期ハンドラのID
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : 周期ハンドラオブジェクト未登録
* (返却値)E_OK : 正常終了(起動完了)
*/
ER kernelrte_sta_cyc(ER_ID cycid)
{
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_cyc()でE_NOIDを返していたならばsta_cyc()ではE_IDを返却 */
	if (cycid == E_NOID || CYCLE_ID_NUM << mg_cyc_info.power_count <= cycid) {
		return E_ID;
	}
	/* 対象周期ハンドラは存在するか?(すでに排除されていないか) */
	else if (mg_cyc_info.id_table[cycid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return sta_cyc_isr(mg_cyc_info.id_table[cycid]);
	}
}


/*!
* cycid変換テーブル設定処理(stp_cyc():周期ハンドラの動作停止)
* cycid : 動作停止するcycid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : 周期ハンドラオブジェクト未登録
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER kernelrte_stp_cyc(ER_ID cycid)
{
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_cyc()でE_NOIDを返していたならばstp_cyc()ではE_IDを返却 */
	if (cycid == E_NOID || CYCLE_ID_NUM << mg_cyc_info.power_count <= cycid) {
		return E_ID;
	}
	/* 対象周期ハンドラは存在するか?(すでに排除されていないか) */
	else if (mg_cyc_info.id_table[cycid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return stp_cyc_isr(mg_cyc_info.id_table[cycid]);
	}
}


/*!
* almid変換テーブル設定処理(acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付))
* この関数からacre_alm()のISRを呼ぶ
* IDの割付は0から順番に行う.最大数までいった時は，0に再度戻り検索する(途中がdeleteされている事があるため)
* type : 静的型か動的型か？
* *exinf : アラームハンドラに渡すパラメータ
* func : アラームハンドラ
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)almid : 正常終了(作成したアラームハンドラID)
*/
ER_ID kernelrte_acre_alm(ALM_TYPE type, void *exinf, TMR_CALLRTE func)
{
	ER_ID almid;
	int almids;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	almids = ALARM_ID_NUM << mg_alm_info.power_count; /* 検索範囲の指定 */
	
	/*システムコールの引数は正しいか*/
  if (!func) {
  	return E_PAR;
  }
  /*
  * 割付可能なIDがない(実行されるのはメモリ不足を起こした時のみ)
  * カウンタの数は静的型alarm handler配列の個数，動的型alarm handlerリストの個数と一致する
  */
  else if (mg_alm_info.counter < 0) {
  	return E_NOID;
  }
	/* 生成できる場合 */
	else {
		/* ISRの呼び出し(mz_acre_alm())とID変換テーブルの設定 */
		mg_alm_info.id_table[mg_alm_info.counter] = (ALMCB *)acre_alm_isr(type, exinf, func);
		almid = (ER_ID)mg_alm_info.counter; /* システムコールのリターンパラメータ型へ変換 */
		mg_alm_info.counter++; /* 次の割付けIDへ(mg_alarmid_table[]のインデックス) */
		/* 次の割付けIDがすでに使用されている場合 */
		if ((almids <= mg_alm_info.counter) || (mg_alm_info.id_table[mg_alm_info.counter] != NULL)) {
			mg_alm_info.counter = (int)serch_almid_table(almids); /* 割付け可能なIDを検索 */
		}
		
		return almid; /* 割付可能なID返却 */
	}
}


/*!
* alamid変換テーブルの次の割付け可能IDを検索をする関数
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってくる
* almid : 割付け可能なID変換テーブルのインデックス
* (返却値)srh : 検索した割付可能ID
* (返却値)srh(E_NOMEM) : メモリ不足
*/
static ER_ID serch_almid_table(int almids)
{
	ER_ID srh_cuntr; /* 検索するカウンタ */
	
	/* alarmid変換テーブルを先頭から検索する */
	for (srh_cuntr = 0; srh_cuntr < almids; srh_cuntr++) {
		if (mg_alm_info.id_table[srh_cuntr] == NULL) {
			DEBUG_OUTVLE(srh_cuntr, 0);
			DEBUG_OUTMSG(" aaaaaa\n");
			return srh_cuntr;
		}
	}
	
	/* メモリ不足 */
	if (pow_almid_table() != E_OK) { /* almidが足らなくなった場合 */
		srh_cuntr = -1; /* 次のISR処理でE_NOIDになるようにする */
	}
	/* メモリ確保できた */
	else {
		srh_cuntr = almids; /* 割付可能IDは累乗前(累乗前の要素数のインデックス)の位置にある */
	}
	DEBUG_OUTVLE(srh_cuntr, 0);
  DEBUG_OUTMSG(" alm next counter\n");
  
	return srh_cuntr;
}


/*!
* alarm handler IDが足らなくなった場合の処理(次の割付可能ID)
* IDが不足している(ID変換テーブルが不足)場合は倍のID数をとってきて，変換テーブル，
* 静的型配列をコピーし，動的型リンクドリストを連結させる
* (返却値)E_NOMEM : メモリが確保できない
* (返却値)E_OK : 正常終了
*/
static ER pow_almid_table(void)
{
	ALMCB **almtbl_tmp, *salm_tmp, *dalm_atmp, *dalm_ftmp;
	
	mg_alm_info.power_count++;
	
	/* 現在使用していた領域すべてを一時退避 */
	almtbl_tmp = mg_alm_info.id_table;
	salm_tmp = mg_alm_info.array;
	dalm_atmp = mg_alm_info.alochead;
	dalm_ftmp = mg_alm_info.freehead; /* この時，必ずmg_alm_info.freeheadはNULLとはならない(静的型があるため) */
	
	if (alm_init() != E_OK) { /* 現在使用していた倍の領域の確保及び初期化する */
		return E_NOMEM;
	}
	/* 新たに確保した領域にすべてコピー */
	copy_all_alm(almtbl_tmp, salm_tmp, dalm_atmp, dalm_ftmp);
	
	/* 退避していた領域を解放(動的型alarm handlerリストは解放しない) */
	rel_mpf_isr(almtbl_tmp);
	rel_mpf_isr(salm_tmp);
	
	return E_OK;
}


/*!
* alarm handlerIDが足らなくなった時に倍のID変換テーブルへコピーする関数
* **almtbl_tmp : 古いalarm handlerID変換テーブル
* *salm_tmp : 古い静的型alarm handler可変長配列
* *dalm_atmp : 古い動的型alarm handler aloc list head
* *alm_ftmp : 古い動的型alarm handler free list head
*/
static void copy_all_alm(ALMCB **almtbl_tmp, ALMCB *salm_tmp, ALMCB *dalm_atmp, ALMCB *dalm_ftmp)
{
	int almids;
	
	almids = ALARM_ID_NUM << (mg_alm_info.power_count - 1); /* 古いalarm handler ID資源数 */
	memcpy(mg_alm_info.id_table, almtbl_tmp, sizeof(&(**almtbl_tmp)) * almids); /* 可変長配列なのでコピー */
	
	copy_salm(salm_tmp, almids); /* 静的型alarm handler配列のコピーとalarm handlerID変換テーブル更新 */
	copy_dalm(dalm_atmp, dalm_ftmp, almids); /* 動的型alarm handlerリストの連結と更新(コピーはしない) */
}


/*!
* alarm handlerIDが足らなくなった時に倍の可変長配列へコピーし，ID変換テーブルを更新する関数
* almids : 古いalarm handler ID資源数
* *salm_tmp : 古い静的型alarm handler可変長配列
*/
static void copy_salm(ALMCB *salm_tmp, int almids)
{
	int i;
	
	memcpy(mg_alm_info.array, salm_tmp, sizeof(*(mg_alm_info.array)) * almids); /* 可変長配列なのでコピー */
	
	/* 静的型alarm handler配列をコピーしたので，それに応じてalarm handlerID変換テーブルを更新 */
	for (i = 0; i < almids; i++) {
		if (mg_alm_info.array[i].almid != -1) {
			mg_alm_info.id_table[i] = &mg_alm_info.array[i]; /* 変換テーブルの更新 */
		}
	}
}

/*!
* alarm handlerIDが足らなくなった時に現在と同じID数を連結し，aloc headとfree headを更新する関数
* almids : 古いalarm handler ID資源数
* *dalm_atmp : 古い動的型alarm handler aloc list head
* *dalm_ftmp : 古い動的型alarm handler free list head
*/
static void copy_dalm(ALMCB *dalm_atmp, ALMCB *dalm_ftmp, int almids)
{
	ALMCB *oldacb;
	
	/*
	* 動的型alarm handlerはalocheadにfreeheadをつなげる
	* 倍の個数は取得せずにalocリストと同じ数を取得し，alocリスト終端へつなげる
	*/
	/* 古い動的型alarm handler freeheadの終端の一つ前を検索
	* (freeリストは使い切っているかもしれないのでalocheadから検索する)
	*/
	if (dalm_ftmp == NULL) {
		oldacb = dalm_atmp;
	}
	else {
		oldacb = dalm_ftmp;
	}
	for (; oldacb->next != NULL; oldacb = oldacb->next) {
		;
	}
	/*
	* old alarm handler freeリストと new alarm handler freeリストの連結する
	* コピーはしない
	*/
	oldacb->next = mg_alm_info.freehead;
	mg_alm_info.freehead->prev = dalm_ftmp;
	
	/* リスト切れしていないならば，free headの位置を戻す */
	if (dalm_ftmp != NULL) {
		mg_alm_info.freehead = dalm_ftmp;
	}
	/* リスト切れしているならば，aloc headの位置を戻す */
	if (dalm_atmp != NULL) {
		mg_alm_info.alochead = dalm_atmp;
	}
}


/*!
* almid変換テーブル設定処理(del_alm():アラームハンドラコントロールブロックの排除)
* almid : 排除するalmid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER kernelrte_del_alm(ER_ID almid)
{
	ER ercd;
	
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_alm()でE_NOIDを返していたならばdel_alm()ではE_IDを返却 */
	if (almid == E_NOID || ALARM_ID_NUM << mg_alm_info.power_count <= almid) {
		return E_ID;
	}
	/* 対象アラームハンドラは存在するか?(すでに排除されていないか) */
	else if (mg_alm_info.id_table[almid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		ercd = del_alm_isr(mg_alm_info.id_table[almid]);
		/* 排除できたならば，alarm handlerID変換テーブルを初期化 */
		if (ercd == E_OK) {
			mg_alm_info.id_table[almid] = NULL;
		}
		
		return ercd;
	}
}


/*!
* almid変換テーブル設定処理(sta_alm():アラームハンドラの動作開始)
* almid : 動作開始するアラームハンドラのID
* msec : 動作開始時間
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(起動完了)
*/
ER kernelrte_sta_alm(ER_ID almid, int msec)
{
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_alm()でE_NOIDを返していたならばsta_alm()ではE_IDを返却 */
	if (almid == E_NOID || ALARM_ID_NUM << mg_alm_info.power_count <= almid) {
		return E_ID;
	}
	/* 対象アラームハンドラは存在するか?(すでに排除されていないか) */
	else if (mg_alm_info.id_table[almid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return sta_alm_isr(mg_alm_info.id_table[almid], msec);
	}
}


/*!
* almid変換テーブル設定処理(stp_alm():アラームの動作停止)
* almid : 動作停止するalmid
* (返却値)E_ID : ID不正
* (返却値)E_NOEXS : アラームハンドラオブジェクト未登録
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER kernelrte_stp_alm(ER_ID almid)
{
	putcurrent(); /* システムコール発行スレッドをレディーへ */
	
	/* 作成であるacre_alm()でE_NOIDを返していたならばstp_alm()ではE_IDを返却 */
	if (almid == E_NOID || ALARM_ID_NUM << mg_alm_info.power_count <= almid) {
		return E_ID;
	}
	/* 対象アラームハンドラは存在するか?(すでに排除されていないか) */
	else if (mg_alm_info.id_table[almid] == NULL) {
		return E_NOEXS;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return stp_alm_isr(mg_alm_info.id_table[almid]);
	}
}


/*!
* 変換テーブル設定処理はいらない(def_inh():割込みハンドラの定義)
* type : ソフトウェア割込みベクタ番号
* handler : 登録するハンドラポインタ
* (返却値)E_ILUSE : 不正使用
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 登録完了
*/
ER kernelrte_def_inh(SOFTVEC type, IR_HANDL handler)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	return def_inh_isr(type, handler);
}


/*!
* 変換テーブル設定処理はいらない(rot_rdp():タスクの優先順位回転)
* tskpri : 回転対象の優先度
* (返却値)E_NOSPT : 未サポート
* (返却値)E_PAR : パラメータ不正
* (返却値)E_OK : 正常終了
*/
ER kernelrte_rot_rdq(int tskpri)
{
	READY_TYPE r_type = mg_ready_info.type;
	SCHDUL_TYPE s_type = mg_schdul_info.type;

	putcurrent(); /* システムコール発行タスクをレディーへ */

	/* スケジューラによって認めているか */
	if (r_type == SINGLE_READY_QUEUE || s_type >= RM_SCHEDULING) {
		return E_NOSPT;
	}
	/* 割込みサービスルーチンの呼び出し */
	else {
		return rot_rdq_isr(tskpri);
	}
}


/*!
* 変換テーブル設定処理はいらない(get_tid():実行スレッドID取得)
* (返却値) : スレッドID
*/
ER_ID kernelrte_get_tid(void)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	return get_tid_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 変換テーブル設定処理はいらない(dis_dsp():ディスパッチの禁止)
*/
void kernelrte_dis_dsp(void)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	return dis_dsp_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 変換テーブル設定処理はいらない(ena_dsp():ディスパッチの許可)
*/
void kernelrte_ena_dsp(void)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	return ena_dsp_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 変換テーブル設定処理はいらない(sns_dsp():ディスパッチの状態参照)
*/
BOOL kernelrte_sns_dsp(void)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	return sns_dsp_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 変換テーブル設定処理はいらない(set_pow():省電力モード設定)
*/
void kernelrte_set_pow(void)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */
	
	return set_pow_isr(); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 変換テーブル設定処理はいらない(sel_schdul():スケジューラの切り替え)
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
ER kernelrte_sel_schdul(SCHDUL_TYPE type, long param)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	/*
	* ・ログの出力(ISR呼び出しでcurrentは切り替わる事があるので，ここの位置でログ出力)
	* ・putcurrent()でレディーデータ構造へ戻した状態でログを出力
	*/
	LOG_CONTEXT(current);

	return sel_schdul_isr(type, param); /* 割込みサービスルーチンの呼び出し */
}


/*!
* 変換テーブル設定処理はいらない(rol_sys();initへのロールバック)
* atr : ロールバック属性
* (返却値)E_OK : 正常終了
* (返却値)E_PAR : パラメータエラー
*/
ER kernelrte_rol_sys(ROL_ATR atr)
{
	putcurrent(); /* システムコール発行タスクをレディーへ */

	return rol_sys_isr(atr); /* 割込みサービスルーチンの呼び出し */
}


/*!
* システムコールタイプを見て各ISRを呼び出す
* type : システムコールのタイプ
* *p : システムコール情報パケットへのポインタ
*/
static void call_functions(SYSCALL_TYPE type, SYSCALL_PARAMCB *p)
{
  /*
  * システムコールの実行中にカレントスレッドが変わるので注意
  * また，オブジェクト解放機能を持つシステムコールは待ちタスクをレディーへ戻すアルゴリズムが
  * 選択されているかチェックする.
  */
  switch (type) {
  case ISR_TYPE_ACRE_TSK: /* acre_tsk() */
    p->un.acre_tsk.ret = kernelrte_acre_tsk(p->un.acre_tsk.type, p->un.acre_tsk.func, p->un.acre_tsk.name,
			       p->un.acre_tsk.priority, p->un.acre_tsk.stacksize, p->un.acre_tsk.rate, p->un.acre_tsk.rel_exetim, 
			       p->un.acre_tsk.deadtim, p->un.acre_tsk.floatim, p->un.acre_tsk.argc, p->un.acre_tsk.argv);
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
    break;
  case ISR_TYPE_DEL_TSK: /* del_tsk() */
  	p->un.del_tsk.ret = kernelrte_del_tsk(p->un.del_tsk.tskid);
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
  	break;
  case ISR_TYPE_STA_TSK: /* sta_tsk() */
  	p->un.sta_tsk.ret = kernelrte_sta_tsk(p->un.sta_tsk.tskid);
  	break;
	case ISR_TYPE_RUN_TSK: /* run_tsk() */
    p->un.run_tsk.ret = kernelrte_run_tsk(p->un.run_tsk.type, p->un.run_tsk.func, p->un.run_tsk.name,
			       p->un.run_tsk.priority, p->un.run_tsk.stacksize, p->un.run_tsk.rate, p->un.run_tsk.rel_exetim, 
			       p->un.run_tsk.deadtim, p->un.run_tsk.floatim, p->un.run_tsk.argc, p->un.run_tsk.argv);
    break;
  case ISR_TYPE_EXT_TSK: /* ext_tsk() */
  	/*パラメータ及びリターンパラメータはいらない*/
  	kernelrte_ext_tsk();
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
  	break;
 	case ISR_TYPE_EXD_TSK: /*exd_tsk()*/
  	/*パラメータ及びリターンパラメータはいらない*/
  	kernelrte_exd_tsk();
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
  	break;
  case ISR_TYPE_TER_TSK: /* ter_tsk() */
  	p->un.ter_tsk.ret = kernelrte_ter_tsk(p->un.ter_tsk.tskid);
  	break;
  case ISR_TYPE_GET_PRI: /* get_pri() */
  	p->un.get_pri.ret = kernelrte_get_pri(p->un.get_pri.tskid, p->un.get_pri.p_tskpri);
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
  	break;
  case ISR_TYPE_CHG_PRI: /* chg_pri() */
    p->un.chg_pri.ret = kernelrte_chg_pri(p->un.chg_pri.tskid, p->un.chg_pri.tskpri);
    break;
	case ISR_TYPE_CHG_SLT: /* chg_slt() */
  	p->un.chg_slt.ret = kernelrte_chg_slt(p->un.chg_slt.type, p->un.chg_slt.tskid, p->un.chg_slt.slice);
  	break;
 	case ISR_TYPE_GET_SLT: /* get_slt() */
  	p->un.get_slt.ret = kernelrte_get_slt(p->un.get_slt.type, p->un.get_slt.tskid, p->un.get_slt.p_slice);
  	break;
  case ISR_TYPE_SLP_TSK: /* slp_tsk() */
  	p->un.slp_tsk.ret = kernelrte_slp_tsk();
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
  	break;
  case ISR_TYPE_TSLP_TSK: /* tslp_tsk() */
  	p->un.tslp_tsk.ret = kernelrte_tslp_tsk(p->un.tslp_tsk.msec);
  	break;
  case ISR_TYPE_WUP_TSK: /* wup_tsk() */
  	p->un.wup_tsk.ret = kernelrte_wup_tsk(p->un.wup_tsk.tskid);
  	break;
  case ISR_TYPE_REL_WAI: /* rel_wai() */
  	p->un.rel_wai.ret = kernelrte_rel_wai(p->un.rel_wai.tskid);
  	break;
  case ISR_TYPE_DLY_TSK: /* dly_tsk() */
  	p->un.dly_tsk.ret = kernelrte_dly_tsk(p->un.dly_tsk.msec);
  	break;
  case ISR_TYPE_ACRE_SEM: /* acre_sem() */
    p->un.acre_sem.ret = kernelrte_acre_sem(p->un.acre_sem.type, p->un.acre_sem.atr, 
					     p->un.acre_sem.semvalue, p->un.acre_sem.maxvalue);
    break;
  case ISR_TYPE_DEL_SEM: /* del_sem() */
    p->un.del_sem.ret = kernelrte_del_sem(p->un.del_sem.semid);
    break;
  case ISR_TYPE_SIG_SEM: /* sig_sem() */
    p->un.sig_sem.ret = kernelrte_sig_sem(p->un.sig_sem.semid);
    break;
  case ISR_TYPE_WAI_SEM: /* wai_sem() */
    p->un.wai_sem.ret = kernelrte_wai_sem(p->un.wai_sem.semid);
    break;
	case ISR_TYPE_POL_SEM: /* pol_sem() */
    p->un.pol_sem.ret = kernelrte_pol_sem(p->un.pol_sem.semid);
    break;
  case ISR_TYPE_TWAI_SEM: /* twai_sem() */
    p->un.twai_sem.ret = kernelrte_twai_sem(p->un.twai_sem.semid, p->un.twai_sem.msec);
    break;
	case ISR_TYPE_ACRE_MBX: /* acre_mbx() */
    p->un.acre_mbx.ret = kernelrte_acre_mbx(p->un.acre_mbx.type, p->un.acre_mbx.msg_atr, p->un.acre_mbx.wai_atr, 														p->un.acre_mbx.max_msgpri);
    break;
  case ISR_TYPE_DEL_MBX: /* del_mbx() */
    p->un.del_mbx.ret = kernelrte_del_mbx(p->un.del_mbx.mbxid);
    break;
	case ISR_TYPE_SND_MBX: /* snd_mbx() */
    p->un.snd_mbx.ret = kernelrte_snd_mbx(p->un.snd_mbx.mbxid, p->un.snd_mbx.pk_msg);
    break;
	case ISR_TYPE_RCV_MBX: /* rcv_mbx() */
    p->un.rcv_mbx.ret = kernelrte_rcv_mbx(p->un.rcv_mbx.mbxid, p->un.rcv_mbx.pk_msg);
    break;
	case ISR_TYPE_PRCV_MBX: /* prcv_mbx() */
    p->un.prcv_mbx.ret = kernelrte_prcv_mbx(p->un.prcv_mbx.mbxid, p->un.prcv_mbx.pk_msg);
    break;
	case ISR_TYPE_TRCV_MBX: /* trcv_mbx() */
    p->un.trcv_mbx.ret = kernelrte_trcv_mbx(p->un.trcv_mbx.mbxid, p->un.trcv_mbx.pk_msg, p->un.trcv_mbx.tmout);
    break;
	case ISR_TYPE_ACRE_MTX: /* acre_mtx() */
    p->un.acre_mtx.ret = kernelrte_acre_mtx(p->un.acre_mtx.type, p->un.acre_mtx.atr, p->un.acre_mtx.piver_type, 													p->un.acre_mtx.maxlocks, p->un.acre_mtx.pcl_param);
    break;
	case ISR_TYPE_DEL_MTX: /* del_mtx() */
		p->un.del_mtx.ret = kernelrte_del_mtx(p->un.del_mtx.mtxid);
		break;
	case ISR_TYPE_LOC_MTX: /* loc_mtx() */
    p->un.loc_mtx.ret = kernelrte_loc_mtx(p->un.loc_mtx.mtxid);
    break;
	case ISR_TYPE_PLOC_MTX: /* ploc_mtx() */
    p->un.ploc_mtx.ret = kernelrte_ploc_mtx(p->un.ploc_mtx.mtxid);
    break;
	case ISR_TYPE_TLOC_MTX: /* tloc_mtx() */
    p->un.tloc_mtx.ret = kernelrte_tloc_mtx(p->un.tloc_mtx.mtxid, p->un.tloc_mtx.msec);
    break;
	case ISR_TYPE_UNL_MTX: /* unl_mtx() */
    p->un.unl_mtx.ret = kernelrte_unl_mtx(p->un.unl_mtx.mtxid);
    break;
	case ISR_TYPE_GET_MPF: /* get_mpf() */
    p->un.get_mpf.ret = kernelrte_get_mpf(p->un.get_mpf.size);
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
    break;
  case ISR_TYPE_REL_MPF: /* rel_mpf() */
    p->un.rel_mpf.ret = kernelrte_rel_mpf(p->un.rel_mpf.p);
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
    break;
  case ISR_TYPE_ACRE_CYC: /* acre_cyc() */
    p->un.acre_cyc.ret = kernelrte_acre_cyc(p->un.acre_cyc.type, p->un.acre_cyc.exinf, p->un.acre_cyc.cyctim, p->un.acre_cyc.func);
    break;
  case ISR_TYPE_DEL_CYC: /* del_cyc() */
    p->un.del_cyc.ret = kernelrte_del_cyc(p->un.del_cyc.cycid);
    break;
  case ISR_TYPE_STA_CYC: /* sta_cyc() */
    p->un.sta_cyc.ret = kernelrte_sta_cyc(p->un.sta_cyc.cycid);
    break;
  case ISR_TYPE_STP_CYC: /* stp_cyc() */
    p->un.stp_cyc.ret = kernelrte_stp_cyc(p->un.stp_cyc.cycid);
    break;
  case ISR_TYPE_ACRE_ALM: /* acre_alm() */
    p->un.acre_alm.ret = kernelrte_acre_alm(p->un.acre_alm.type, p->un.acre_alm.exinf, p->un.acre_alm.func);
    break;
  case ISR_TYPE_DEL_ALM: /* del_alm() */
    p->un.del_alm.ret = kernelrte_del_alm(p->un.del_alm.almid);
    break;
  case ISR_TYPE_STA_ALM: /* sta_alm() */
    p->un.sta_alm.ret = kernelrte_sta_alm(p->un.sta_alm.almid, p->un.sta_alm.msec);
    break;
  case ISR_TYPE_STP_ALM: /* stp_alm() */
    p->un.stp_alm.ret = kernelrte_stp_alm(p->un.stp_alm.almid);
    break;
	case ISR_TYPE_DEF_INH: /* def_inh() */
    p->un.def_inh.ret = kernelrte_def_inh(p->un.def_inh.type,
				       p->un.def_inh.handler);
		/* ログの出力(ISR呼び出しでcurrentは切り替わらないため，この位置でログ出力) */
		LOG_CONTEXT(current);
    break;
	case ISR_TYPE_ROT_RDQ: /* rot_rdq() */
		p->un.rot_rdq.ret = kernelrte_rot_rdq(p->un.rot_rdq.tskpri);
		break;
  case ISR_TYPE_GET_TID: /* get_tid() */
  	p->un.get_tid.ret = kernelrte_get_tid();
  	break;
  case ISR_TYPE_DIS_DSP: /* dis_dsp() */
  	kernelrte_dis_dsp();
  	break;
  case ISR_TYPE_ENA_DSP: /* ena_dsp() */
  	kernelrte_ena_dsp();
  	break;
  case ISR_TYPE_SNS_DSP: /* sns_dsp() */
  	p->un.sns_dsp.ret = kernelrte_sns_dsp();
  	break;
  case ISR_TYPE_SET_POW: /* set_pow() */
  	kernelrte_set_pow();
  	break;
	case ISR_TYPE_SEL_SCHDUL: /* sel_schdul */
		p->un.sel_schdul.ret = kernelrte_sel_schdul(p->un.sel_schdul.type, p->un.sel_schdul.schdul_param);
	case ISR_TYPE_ROL_SYS: /* rol_sys() */
		p->un.rol_sys.ret = kernelrte_rol_sys(p->un.rol_sys.atr);
		break;
  default:
    break;
  }
}


/*!
* タスクコンテキスト用システムコールの各割込みサービスルーチンを呼び出すための準備関数
* ディスパッチ禁止状態ならば，システムコールの発行は認めない(システムコールはつねにタスクを切り替えるため)
* ディスパッチ禁止状態で呼べるのはサービスコールのみとなる
*/
static void syscall_proc(ISR_TYPE type, SYSCALL_PARAMCB *p)
{
	ER *ercd;
 
	/*
	* ディスパッチ禁止状態でシステムコール発行された時は返却値を書き換える(各ISRを呼び出さない)
	* ただし，ena_dsp()はISRを呼び出し返却値は書き換えない
	* 非タスクコンテキスト用システムコールと一貫性を保つために，ここでディスパッチ禁止状態のチェック
	*/
	if (dsp_info.flag == FALSE && type != ISR_TYPE_ENA_DSP) {
		ercd = (ER *)current->syscall_info.ret;
		*ercd = E_CTX;
		return;
	}

	/*
	* システムコールを呼び出したタスクはレディーから抜かれてから各ISRを呼ぶ
	* 実装としている．
	* このOSは割込みドリブン型であるため，システムコールが発行されたらつねに
	* タスクが切り替わるようにしている(同優先度レベルでも切り替わる)
	* つまり，タスクを切り替える必要がないシステムコール処理もタスクを切り替えている
	* タスクを切り替えたくない場合は別途用意しているサービスコールを使用する
	*/
	current->syscall_info.flag = MZ_SYSCALL;
  getcurrent(); /* システムコール発行タスクをレディーから抜く */
  call_functions(type, p); /* システムコールのタイプを見て各ISRに分岐 */
}


/*!
* 非タスクコンテキスト用システムコールの各割込みサービスルーチンを呼び出すための準備関数
* ディスパッチ禁止状態ならば，システムコールの発行は認めない(システムコールはつねにタスクを切り替えるため)
* ディスパッチ禁止状態で呼べるのはサービスコールのみとなる
*/
static void isyscall_proc(ISR_TYPE type, SYSCALL_PARAMCB *p)
{
	ER *ercd;

  /*
	* ディスパッチ禁止状態でシステムコール発行された時は返却値を書き換える(各ISRを呼び出さない)
	* ただし，ena_dsp()はISRを呼び出し返却値は書き換えない
	* ディスパッチ禁止状態のチェックはここでしか行えない
	*/
	if (dsp_info.flag == FALSE && type != ISR_TYPE_ENA_DSP) {
		ercd = (ER *)current->syscall_info.ret;
		*ercd = E_CTX;
		return;
	}
	/*
	* システムコールのラッパーでシステムコール発行タスクをレディーへ戻す処理が
	* があるので，不整合が起こる(非タスクコンテキスト用システムコールはタスクを
	* スイッチングしない)ここで，currentのシステムコールフラグを非タスクコンテ
	*キスト用にしておく事によってputcurrent()及びgetcurrent()ではじかれるようになる
	* 非タスクコンテキスト用システムコールは外部割込みなどの延長上(単なる関数呼び出し)
	* で呼ばれるので，外部割込みが終了した時にスケジューラが起動される事となる
	*/
  current->syscall_info.flag = MZ_ISYSCALL;
  call_functions(type, p);
}


/*!
* タスクコンテキスト用システムコール割込みハンドラ
* すべてのハンドラは仮引数，返却値ともにvoid型となるため，この関数を追加した
*/
static void syscall_intr(void)
{
	/* 各割込みサービスルーチンを呼ぶための準備関数呼び出し */
  syscall_proc(current->syscall_info.type, current->syscall_info.param);
}


/*!
* 非タスクコンテキスト用システムコール呼び出しライブラリ関数
* これはタスクコンテキスト用システムコール呼び出しと一貫性を保つため追加した
* トラップの発行は行わない
*/
void isyscall_intr(ISR_TYPE type, SYSCALL_PARAMCB *param)
{
	/* 各割込みサービスルーチンを呼ぶための準備関数呼び出し */
  isyscall_proc(type, param);
}


/*! ソフトウェアエラー(例外)ハンドラ */
static void softerr_intr(void)
{
  DEBUG_OUTMSG(current->init.name);
  DEBUG_OUTMSG(" DOWN.\n");
  getcurrent(); /* エラーとなったタスクをレディーからはずす */
	mz_exd_tsk(); /* システムコール(タスクの終了と排除) */
}


/*!
* タスクコンテキスト情報をTCBへ設定する関数
* type : ソフトウェア割込みベクタのタイプ
* sp : タスクコンテキストのポインタ
*/
static void set_tsk_context(SOFTVEC type, UINT32 sp)
{
	/*! カレントタスクのコンテキストを保存 */
  current->intr_info.sp = sp;

	/* 例外の場合 */
	if (type == SOFTVEC_TYPE_SOFTERR) {
		current->intr_info.type = EXCEPTION;
	}
	/* システムコール割込みの場合 */
	else if (type == SOFTVEC_TYPE_SYSCALL) {
		current->intr_info.type = SYSTEMCALL_INTERRUPT;
	}
	/* シリアル割込みの場合 */
	else if (type == SOFTVEC_TYPE_SERINTR) {
		current->intr_info.type = SERIAL_INTERRUPT;
	}
	/* タイマ割込みの場合 */
	else if (type == SOFTVEC_TYPE_TIMINTR) {
		current->intr_info.type = TIMER_INTERRUPT;
	}
	/* NMI割込みの場合 */
	else if (type == SOFTVEC_TYPE_NMIINTR) {
		current->intr_info.type = NMI_INTERRUPT;
	}
	/* 以外 */
	else {
		/* 処理なし */
	}
}


/*!
* 割込み処理入り口関数(ベクタに登録してある割込みハンドラ)
* このベクタからソフトウェアベクタに登録してある割込みハンドラを呼ぶ2段階となる
* 外部割込み(シリアル割込み，タイマ割込み)もカレントタスクのスタックへコンテキストを保存し，
* スケジューラ→ディスパッチャという順となる
* type : ソフトウェア割込みベクタのタイプ
* sp : タスクコンテキストのポインタ
*/
ER thread_intr(SOFTVEC type, UINT32 sp)
{
	
	set_tsk_context(type, sp); /* タスクコンテキスト情報をTCBへ設定 */

	/* タイムスライス型スケジューリング環境下で割込みが発生しタイマブロックを排除するか検査する関数 */
	check_tmslice_schedul(type);

	/*
	* ソフトウェア割込みベクタに登録してある割込みハンドラを実行する
	* SOFTVEC_TYPE_SYSCALL，SOFTVEC_TYPE_SOFTERR，SOFTVEC_TYPE_TIMINTR
	* の場合はsyscall_intr，softer_intr，tmrdriver_intrが実行される
	* シリアル割込みハンドラは登録式としている
	*/
  if ((*handlers[type])) {

    (*handlers[type])(); /* 割込みハンドラ起動 */

		schedule(); /* スケジューラ呼び出し */
 
		if (type == SOFTVEC_TYPE_SYSCALL) {
			LOG_CONTEXT(current); /* 次に実行されるタスクのログを出力 */
		}
 	 	(*dsp_info.func)(&current->intr_info.sp); /* ディスパッチャの呼び出し */
		/*
		* システムコールコントロールブロックのretをユーザタスク側の返却値とするので，
		* 実質このエラーコードは意味がない(返却されない)
		*/
		return E_OK;
	}
	/*
	* シリアル割込みハンドラが未登録
	* (例外，システムコール割込みハンドラ，タイマ割込みハンドラ，NMI割込みハンドラはOSに組み込む実装としている)
	*/
	else {
		return EV_NORTE;
	}
}


/*!
* トラップ発行(システムコール)
* type : システムコールのタイプ
* *param : システムコールパケットへのポインタ
* ret : システムコール返却値格納ポインタ
*/
void issue_trap_syscall(ISR_TYPE type, SYSCALL_PARAMCB *param, OBJP ret)
{
  current->syscall_info.type  = type;
  current->syscall_info.param = param;
  current->syscall_info.ret = ret;
  asm volatile ("trapa #0"); /*システムコールのトラップ割込み発行*/
}


/*! トラップ発行(タスクを落とす) */
void issue_trap_softerr(void)
{
	asm volatile ("trapa #1");
}


/*! ディスパッチャの初期化 */
static void dispatch_init(void)
{
	dsp_info.flag = TRUE;
	dsp_info.func = dispatch;
}


/*! デフォルトのスケジューラを登録する関数 */
static void set_schdul_init_tsk(void)
{
	/* 割込みサービスルーチンを直接呼ぶ */
	sel_schdul_isr(PRI_SCHEDULING, 0);
	/* rol_sys()のISRは発行しない */
}


/*!
* initタスクの生成と起動をする
* ・スタートスレッドを起動する時にはシステムコールは使用できないため，生成と起動は直接内部関数を呼ぶ
* ・NMIが入るとこの関数へ戻ってくる
* func : タスクのメイン関数
* *name : タスクの名前
* priority : タスクの優先度
* stacksize : ユーザスタック(タスクのスタック)のサイズ
* argc : タスクのメイン関数の第一引数
* argv[] : タスクのメイン関数の第二引数
* (返却値) EV_NSLPAL : スケジューリングアルゴリズムが選択されていない
* (返却値) E_OK : 正常終了だが，ディスパッチ後なので実際は返さない
*/
void process_init_tsk(TSK_FUNC func, char *name, int priority, int stacksize,
	      int argc, char *argv[])
{
	ER_ID tskid;
	ER ercd;
	
	dispatch_init(); /* ディスパッチャの初期化 */
  mem_init(); /* 動的メモリの初期化 */
  tmrdriver_init(); /* タイマドライバの初期化 */
	log_mechanism_init();
	/* スケジューラの初期化 */
	if (schdul_init() != E_OK) {
		down_system();
	}
	/* レディーの初期化 */
	if (ready_init() != E_OK) {
		down_system();
	}
	/* タスク周りの初期化(静的，動的) */
	if (tsk_init() != E_OK) {
		down_system(); /* メモリが取得できない場合はOSをスリープさせる */
	}
	/* セマフォ周りの初期化(静的，動的) */
	if (sem_init() != E_OK) {
		down_system(); /* メモリが取得できない場合はOSをスリープさせる */
	}
	/* メールボックス周りの初期化(静的，動的) */
	if (mbx_init() != E_OK) {
		down_system(); /* メモリが取得できない場合はOSをスリープさせる */
	}
	/* ミューテックス周りの初期化(静的，動的) */
	if (mtx_priver_init() != E_OK) {
		down_system(); /* メモリが取得できない場合はOSをスリープさせる */
	}
	/* アラームハンドラ周りの初期化(静的，動的) */
  if (alm_init() != E_OK) {
  	down_system(); /* メモリが取得できない場合はOSをスリープさせる */
  }
  /* 周期ハンドラ周りの初期化(静的，動的) */
  if (cyc_init() != E_OK) {
  	down_system(); /* メモリが取得できない場合はOSをスリープさせる */
  }

  /*
  * 以降で呼び出すkernel.cの内部関数でcurrentを見ている場合があるので
  * サービスコールフラグ(MV_SRVCALL)を設定し，(getcurretn()，putcurrent()ではじかれるようにする)
	* またkernelrte...しか呼べない．タスク周りの....isr()の関数を直接呼ぶとユーザタスクの最初が
	* ID0として生成されてしまう．ISR直接はよって呼んではならない
	* なぜかはacre_tsk()はID変換テーブルの設定をしないから．
  */
  current->syscall_info.flag = MV_SRVCALL;

	/* 0初期化処理 */
  memset(handlers, 0, sizeof(handlers));
  
  /* 割込みハンドラの登録 */
  ercd = kernelrte_def_inh(SOFTVEC_TYPE_SYSCALL, syscall_intr); /* システムコール */
  if (ercd == E_OK) {
  	puts(" syscall handler ok\n");
  }
  ercd = kernelrte_def_inh(SOFTVEC_TYPE_SOFTERR, softerr_intr); /* ダウン要請 */
  if (ercd == E_OK) {
  	puts(" softerr handler ok\n");
  }
	ercd = kernelrte_def_inh(SOFTVEC_TYPE_TIMINTR, tmrdriver_intr); /* タイマ割込み */
	if (ercd == E_OK) {
		puts(" timer handler handler ok\n");
	}
	ercd = kernelrte_def_inh(SOFTVEC_TYPE_NMIINTR, nmi_intr); /* NMI割込み */
	if (ercd == E_OK) {
		puts(" nmi handler ok\n");
	}
	
  /* システムコール発行不可なので直接内部関数を呼んでタスクを生成する(initに周期，実行時間，デッドライン時刻は設定しない) */
  tskid = kernelrte_acre_tsk(DYNAMIC_TASK, func, name, priority, stacksize, 0, 0, 0, 0, argc, argv);
	ercd = kernelrte_sta_tsk(tskid); /* タスクの起動 */
	
	/* 最初のタスクの起動 */
  (*dsp_info.func)(&current->intr_info.sp); /* ディスパッチャの呼び出し */

  /* ここには返ってこないこない */
}


/*!
* initタスクの生成と起動をする
* func : タスクのメイン関数
* *name : タスクの名前
* priority : タスクの優先度
* stacksize : ユーザスタック(タスクのスタック)のサイズ
* argc : タスクのメイン関数の第一引数
* argv[] : タスクのメイン関数の第二引数
* (返却値) EV_NSLPAL : スケジューリングアルゴリズムが選択されていない
* (返却値) E_OK : 正常終了だが，ディスパッチ後なので実際は返さない
*/
void start_init_tsk(TSK_FUNC func, char *name, int priority, int stacksize,
	      int argc, char *argv[])
{
	set_schdul_init_tsk(); /* デフォルトでのスケジューラ設定 */
	
	process_init_tsk(func, name, priority, stacksize, argc, argv); /* initタスク生成 */

  /* ここには返ってこないこない */
}


/*! OSの致命的エラー時 */
void down_system(void)
{
  KERNEL_OUTMSG("system error!\n");
  /* システムをとめる */
  while (1) {
    ;
	}
}


/*! 処理待ち(NMI) */
void freeze_kernel(void)
{
	KERNEL_OUTMSG("kernel freeze.\n");
	/* システムをとめる */
	while (1) {
		;
	}
}


/*!
* カーネルオブジェクトを取得しているか検査し，取得しているならば各解放関数(無条件関数)を呼ぶ.
* ext_tskやexd_tskの割込みサービスルーチンの延長上で呼ばれる
* この関数はレディーから抜かれ休止状態でない場合に呼ぶ.つまり，flagsの下3ビットは0となっている状態
* tskid : 検査するタスクコントロールブロックへのポインタ
* (返却値)E_NSLPAL : レディーへ戻すアルゴリズムを選択していない
* (返却値)E_OK : 正常終了
* (返却値)E_NOSPT : まだ実装していない
*/
ER release_object(TCB *checktcb)
{
	/*
	* TCBの取得情報を見て処理を分岐する.なお，取得情報のみしか管理していないので(待ち要因管理とは異なる)
	* マスクをかけて判断する必要はない.リバースそれぞれの無条件関数のコールスタック上で行われる.
	*/
	switch (checktcb->get_info.flags) {
		case TASK_GET_SEMAPHORE:
			sig_sem_condy((SEMCB *)checktcb->get_info.gobjp); /* セマフォの無条件解放 */
			return E_OK;
		case TASK_GET_MUTEX:
			check_mtx_uncondy_protocol((MTXCB *)checktcb->get_info.gobjp); /* mutexの無条件解放(プロトコルも調べる) */
			return E_OK;
		default:
			DEBUG_OUTMSG("not get kernel object for task dormant or non exsitent.\n");
			return E_OK;
	}
}


/*!
* 仮引数でもらったworktcb->flagsをみてどの待ち行列からTCBを抜き取るか分岐する
* ter_tskやrel_waiの割込みサービスルーチンの延長上で呼ばれる
* *wortcb : 対象TCB
* flags : 待ち要因のみ記録してあるflag(下位3ビットは抜き取ってある)
* (返却値)E_OK : 待ち行列につながれている(つながれているものは抜き取られる)
* (返却値)E_NG : 待ち行列につながれていない
* (返却値)E_NOSPT : まだ実装していない
*/
ER get_tsk_waitque(TCB *worktcb, UINT16 flags)
{

	DEBUG_OUTMSG("task check waitqueue.\n");
	DEBUG_OUTVLE(worktcb->state, 0);
	DEBUG_OUTMSG("\n");

	/*
	* 待ち要因を確認し，処理を分岐する.
	* なお，待ち要因となっている待ち行列からTCBを抜き取った後は必ず，待ち要因フラグだけリバースマスクをかける
	* 待ち要因の他にもタイマブロックの抜き取りはここからは呼び出さない
	* よって，tslp_tsk()やdly_tsk()はフラグを落とすだけになるので注意
	*/
	switch (flags) {
		case TASK_WAIT_TIME_SLEEP:
			worktcb->state &= ~TASK_WAIT_TIME_SLEEP;
			/* wobjpはここでは0になっている */
			return E_OK;
		case TASK_WAIT_TIME_DELAY:
			worktcb->state &= ~TASK_WAIT_TIME_DELAY;
			/* wobjpはここでは0になっている */
			return E_OK;
		case TASK_WAIT_SEMAPHORE:
			/* semaphoreキューからTCBを抜き取る(関数はsemaphore.cにある) */
			get_tsk_sem_waitque(((SEMCB *)worktcb->wait_info.wobjp), worktcb);
			worktcb->state &= ~TASK_WAIT_SEMAPHORE; /* 待ち要因分だけリバースマスク */
			worktcb->wait_info.wobjp = 0;
			return E_OK;
		case TASK_WAIT_MUTEX:
			/* mutexキューからTCBを抜き取る(関数はmutex.cにある) */
			get_tsk_mtx_waitque(((MTXCB *)worktcb->wait_info.wobjp), worktcb);
			worktcb->state &= ~TASK_WAIT_MUTEX; /* 待ち要因分だけリバースマスク */
			worktcb->wait_info.wobjp = 0;
			return E_OK;
		case TASK_WAIT_VIRTUAL_MUTEX:
			/* virtual mutexキューからTCBを抜き取る(関数はmutex.cにある) */
			get_tsk_virtual_mtx_waitque(((MTXCB *)worktcb->wait_info.wobjp), worktcb);
			worktcb->state &= ~TASK_WAIT_VIRTUAL_MUTEX; /* 待ち要因分だけリバースマスク */
			worktcb->wait_info.wobjp = 0;
			return E_OK;
		case TASK_WAIT_MAILBOX:
			return E_NOSPT;
		default:
			DEBUG_OUTMSG("task not wait queue(for ter_tsk() rel_wai() calls).\n");
			return E_NG;
	}
}
