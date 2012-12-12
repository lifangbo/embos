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
#include "memory.h"
#include "mutex.h"
#include "scheduler.h"
#include "ready.h"
#include "prinvermutex.h"
#include "target/driver/timer_driver.h"
#include "timer_callrte.h"


/*! ミューテックスの初期化(静的型mutexの領域確保と初期化(可変長配列として使用する)) */
static ER static_mtx_init(int mtxids);

/*! ミューテックスの初期化(動的型mutexの領域確保と初期化(freeリストを作成する)) */
static ER dynamic_mtx_init(void);

/*! 動的mutex alocリストからmutexコントロールブロックを抜き取る */
static void get_aloclist(MTXCB *mcb);

/*! mutexロック解除時，mutex待ちタスクを先頭からレディーへ入れる */
static void put_mtx_fifo_ready(MTXCB *mcb);

/*! mutexロック解除時，mutex待ちタスクを優先度が高いものからレディーへ入れる */
static void put_mtx_pri_ready(MTXCB *mcb);



/*!
* 優先度逆転機構の初期化とmutexの初期化
* start_init_tsk()で呼ばれるのみ
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER mtx_priver_init(void)
{
	/* 最初のみの初期化 */
	mg_mtx_info.imhigh_loc_head = NULL;
	mg_mtx_info.dyhigh_loc_head = NULL;
	mg_mtx_info.ceil_pri_head = NULL;
	mg_mtx_info.virtual_mtx = NULL;

	return mtx_init(); /* mutexの初期化 */
}


/*!
* ミューテックスの初期化(mutex ID変換テーブルの領域確保と初期化)
* -mutex IDが足らなくなった場合に呼ばれる
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER mtx_init(void)
{
	int mtxids, i;
	MTXCB *mcb;
	
	mtxids = MUTEX_ID_NUM << mg_mtx_info.power_count; /* 倍の領域を確保する(start_init_tsk()の時はデフォルト通り) */
	mg_mtx_info.id_table = (MTXCB **)get_mpf_isr(sizeof(mcb) * mtxids); /* 変換テーブルの動的メモリ確保 */
	/* mutexID変換テーブルの初期化(メモリにNULLを埋めるのにmemset()は使用できない) */
	for (i = 0; i < mtxids; i++) {
		mg_mtx_info.id_table[i] = NULL;
	}
	/* MTXCBの静的型配列の作成及び初期化とMTXCBの動的型freeリストの作成と初期化 */
	if ((E_NOMEM == static_mtx_init(mtxids)) || (E_NOMEM == dynamic_mtx_init())) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk関数内でOSをスリープさせる */
	}
	
	return E_OK;
}


/*!
* ミューテックスの初期化(静的型mutexの領域確保と初期化(可変長配列として使用する))
* mtxids : 確保する可変長配列の個数
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER static_mtx_init(int mtxids)
{
	MTXCB *mcb;
	int size = sizeof(*mcb) * mtxids; /* 可変長配列のサイズ */
	
	mg_mtx_info.array = (MTXCB *)get_mpf_isr(size); /* 可変長配列確保 */
	if (mg_mtx_info.array == NULL) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	/* 確保した可変長配列を初期化(可変長でも配列はメモリに連続しているのでこれでOK) */
	memset(mg_mtx_info.array, -1, size);
	
	return E_OK;
}

/*!
* ミューテックスの初期化(動的型mutexの領域確保と初期化(freeリストを作成する))
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER dynamic_mtx_init(void)
{
	int mtxids, i;
	MTXCB *mcb;
	
	mg_mtx_info.freehead = mg_mtx_info.alochead = NULL;
	
	/* start_init_tsk()の時以外 */
	if (mg_mtx_info.power_count) {
		mtxids = MUTEX_ID_NUM << (mg_mtx_info.power_count - 1); /* 現在と同じ領域を確保する */
	}
	/* start_init_tsk()の時 */
	else {
		mtxids = MUTEX_ID_NUM << mg_mtx_info.power_count;
	}
	
	for (i = 0; i < mtxids; i++) {
		mcb = (MTXCB *)get_mpf_isr(sizeof(*mcb)); /* ノードのメモリ確保 */
		/*メモリが取得できない*/
  	if(mcb == NULL) {
  		return E_NOMEM;	 /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
  	}
		memset(mcb, -1, sizeof(*mcb)); /* 確保したノードを初期化 */
		/* freeキューの作成 */
		mcb->next = mg_mtx_info.freehead;
		mcb->prev = NULL;
		mg_mtx_info.freehead = mcb->next->prev = mcb;
	}
	
	return E_OK;
}


/*!
* 優先度逆転問題解消プロトコルによってロック処理を切り替える
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER check_loc_mtx_protocol(MTXCB *mcb)
{
	READY_TYPE type = mg_ready_info.type;
	TCB *worktcb;

	for (worktcb = mg_mtx_info.virtual_mtx->waithead; worktcb != NULL; worktcb = worktcb->wait_info.wait_next) {
		DEBUG_OUTMSG("tskid ");
		DEBUG_OUTVLE(worktcb->init.tskid, 0);
		DEBUG_OUTMSG(" \n");
	}

	/* スケジューラによって認めているか */
	if (type == SINGLE_READY_QUEUE) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOSPT;
	}
	/* プロトコルなし */
	else if (mcb->piver_type == TA_VOIDPCL) {
		return loc_voidmtx_isr(mcb);
	}
	/* Delay Highest Lockerプロトコル */
	else if (mcb->piver_type == TA_DYHIGHLOC) {
		return loc_dyhighmtx_isr(mcb);
	}
	/* 優先度継承プロトコル */
	else if (mcb->piver_type == TA_INHERIT) {
		return loc_inhermtx_isr(mcb);
	}
	/* 優先度上限プロトコル */
	else if (mcb->piver_type == TA_CEILING) {
		return loc_ceilmtx_isr(mcb);
	}
	/* Immediate Highest Lockerプロトコル */
	else if (mcb->piver_type == TA_IMHIGHLOC) {
		return loc_imhighmtx_isr(mcb);
	}
	/* virtual priority inheritanceプロトコル */
	else if (mcb->piver_type == TA_VINHERIT) {
		return loc_vinhermtx_isr(mcb);
	}
	/* 以外 */
	else {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOSPT;
	}
}


/*!
* 優先度逆転問題解消プロトコルによってポーリングロック処理を切り替える
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : 正常終了(mutexロック完了，再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_NOSPT : プロトコルありでのpol_sem()は認めない
*/
ER check_ploc_mtx_protocol(MTXCB *mcb)
{
	READY_TYPE type = mg_ready_info.type;

	/* スケジューラによって認めているか */
	if (type == SINGLE_READY_QUEUE) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOSPT;
	}
	/* プロトコルなし */
	else if (mcb->piver_type == TA_VOIDPCL) {
		return ploc_voidmtx_isr(mcb);
	}
	/* 以外 */
	else {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOSPT;
	}
}


/*!
* 優先度逆転問題解消プロトコルによってタイムアウト付きロック処理を切り替える
* *mcb : 対象mutexコントロールブロックへのポインタ
* msec : タイムアウト値
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了(mutexセマフォを取得，mutexセマフォ待ちタスクにつなげる)
* (返却値)E_OK，E_ILUSE : dynamic_multipl_lock()の返却値(再帰ロック完了，多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
* (返却値)E_NOSPT : プロトコルありでのtloc_sem()は認めない
*/
ER check_tloc_mtx_protocol(MTXCB *mcb, int msec)
{
	READY_TYPE type = mg_ready_info.type;

	/* スケジューラによって認めているか */
	if (type == SINGLE_READY_QUEUE) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOSPT;
	}
	/* プロトコルなし */
	else if (mcb->piver_type == TA_VOIDPCL) {
		return ploc_voidmtx_isr(mcb);
	}
	/* 以外 */
	else {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOSPT;
	}
}


/*!
* 優先度逆転問題解消プロトコルによってアンロック処理を切り替える
* *mcb : 対象mutexコントロールブロック
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_OK : mutexセマフォ待ちタスクへ割り当て，mutexセマフォの解放，
* 							再帰ロック解除(not_lock_first_mtx()の返却値)
* (返却値)E_ILUSE : オーナータスクでない場合，mutexセマフォ解放エラー
* 									(すでに解放済み，not_lock_first_mtx()の返却値)
*/
ER check_unl_mtx_protocol(MTXCB *mcb)
{
	READY_TYPE type = mg_ready_info.type;
	TCB *worktcb;

	for (worktcb = mg_mtx_info.virtual_mtx->waithead; worktcb != NULL; worktcb = worktcb->wait_info.wait_next) {
		DEBUG_OUTMSG("tskid ");
		DEBUG_OUTVLE(worktcb->init.tskid, 0);
		DEBUG_OUTMSG(" \n");
	}

	/* スケジューラによって認めているか */
	if (type == SINGLE_READY_QUEUE) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOSPT;
	}
	/* プロトコルなし */
	else if (mcb->piver_type == TA_VOIDPCL) {
		return unl_voidmtx_isr(mcb);
	}
	/* Delay Highest Lockerプロトコル */
	else if (mcb->piver_type == TA_DYHIGHLOC) {
		return unl_dyhighmtx_isr(mcb);
	}
	/* 優先度継承プロトコル */
	else if (mcb->piver_type == TA_INHERIT) {
		return unl_inhermtx_isr(mcb);
	}
	/* 優先度上限プロトコル */
	else if (mcb->piver_type == TA_CEILING) {
		return unl_ceilmtx_isr(mcb);
	}
	/*Immediate Highest Lockerプロトコル */
	else if (mcb->piver_type == TA_IMHIGHLOC) {
		return unl_imhighmtx_isr(mcb);
	}
	/* virtual priority inheritanceプロトコル */
	else if (mcb->piver_type == TA_VINHERIT) {
		return unl_vinhermtx_isr(mcb);
	}
	/* 以外 */
	else {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
		return E_NOSPT;
	}
}


/*!
* mutexを無条件で解放する関数への分岐(プロトコルによって処理を切り替える(ext_tsk(), exd_tsk()からしか呼べれない))
* mcb : 解放するカーネルオブジェクトのポインタ
* (返却値)E_NOSPT : スケジューラが認めていない
* (返却値)E_NOEXS : カーネルオブジェクトが存在しない
* (返却値)E_OK : カーネルオブジェクトへ待ちタスクの割り当て，カーネルオブジェクトのクリア
*/
ER check_mtx_uncondy_protocol(MTXCB *mcb)
{
	READY_TYPE type = mg_ready_info.type;

	if (mcb == NULL) {
		DEBUG_OUTMSG("not agree kernel object pointer.\n");
		return E_NOEXS;
	}
	/* スケジューラによって認めているか */
	else if (type == SINGLE_READY_QUEUE) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_NOSPT;
	}
	/* プロトコルなし */
	else if (mcb->piver_type == TA_VOIDPCL) {
		return unl_voidmtx_condy(mcb);
	}
	/* 優先度逆転機構の自動解放機能は未サポート */
	else {
		return E_NOSPT;
	}
}


/*!
* 動的mutex alocリストからセマフォコントロールブロックを抜き取る
* mcb : 対象のMTXCB
*/
static void get_aloclist(MTXCB *mcb)
{
	/* 先頭から抜きとる */
	if (mcb == mg_mtx_info.alochead) {
		mg_mtx_info.alochead = mcb->next;
		mg_mtx_info.alochead->prev = NULL;
	}
	/* それ以外から抜き取る */
	else {
		mcb->prev->next = mcb->next;
		mcb->next->prev = mcb->prev;
	}
}


/*!
* システムコール処理(acre_mtx():mutexコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* atr : タスクをレディーへ戻すアルゴリズム(FIFO順か?優先度順か?)
* piver_mtx : 優先度逆転機構の選択
* maxlocks : 再帰ロックの上限値
* pcl_param : 優先度逆転プロトコル適用時のパラメータ(必要としない優先度逆転プロトコルもある)
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mcb : 正常終了(作成したmutexコントロールブロックへのポインタ)
*/
OBJP acre_mtx_isr(MTX_TYPE type, MTX_ATR atr, PIVER_TYPE piver_type, int maxlocks, int pcl_param)
{
  MTXCB *mcb, *vmcb;
  
  /* 静的型mutexの場合 */
  if (type == STATIC_MUTEX) {
  	mcb = &mg_mtx_info.array[mg_mtx_info.counter];
  }
  /* 動的型mutexの場合 */
  else {
  	mcb = mg_mtx_info.freehead; /* free headからノードを与える(抜き取るわけではない) */
  	mg_mtx_info.freehead = mcb->next; /* free headを一つ進める */
  	/* alocheadが空の場合 */
  	if (mg_mtx_info.alochead == NULL) {
  		mg_mtx_info.alochead = mcb;
  	}
  }
  mcb->mtxid = mg_mtx_info.counter; /* カウンタ(ID変換テーブル(可変長配列のインデックス)) */
	mcb->mtx_type = type; /* 静的か動的かを記録 */
	mcb->atr = atr; /* 属性 */
	mcb->piver_type = piver_type; /* 優先度逆転機構 */
  mcb->mtxvalue = 0; /* mutexの値，mutex作成時(初期値)は0 */
  mcb->locks = 0; /* 再帰ロックの回数，mutex作成時(初期値)は0 */
  mcb->maxlocks = maxlocks; /* 再帰ロックの上限値 */
  mcb->ownerid = -1; /* 作成時，オーナーシップは指定しない */
	mcb->pcl.pcl_next = mcb->pcl.pcl_prev = NULL;
  mcb->waithead = mcb->waittail = NULL;

		
	/* virtual priority inheritanceの場合，acre_mtx()の初回のみvirtual mtxを作成 */
	if (piver_type == TA_VINHERIT && mg_mtx_info.virtual_mtx == NULL) {
		/* virtual mutexの作成 */
		vmcb = (MTXCB *)get_mpf_isr(sizeof(*vmcb));
		/* メモリ取得できなかった場合 */
		if (vmcb == NULL) {
			down_system();
		}
		/* メモリ取得できた場合，virtual mutexを初期化 */
		else {
			/* mtxidは使用しない */
			/* mutex typeは指定しない */
			vmcb->atr = atr; /* 属性 */
			/* 優先度逆転機構は指定しない */
  		vmcb->mtxvalue = 0; /* mutexの値，mutex作成時(初期値)は0 */
  		vmcb->locks = 0; /* 再帰ロックの回数，mutex作成時(初期値)は0 */
  		/* 再帰ロックの上限値は指定しない */
  		vmcb->ownerid = -1; /* 作成時，オーナーシップは指定しない */
			/* 優先度逆転機構ポインタは使用しない */
  		vmcb->waithead = vmcb->waittail = NULL;

			mg_mtx_info.virtual_mtx = vmcb; /* グローバルエリアへ繋げておく */
		}
	}
	/* 優先度逆転機構エリアが必要ない場合 */
	else if (piver_type == TA_VOIDPCL || piver_type == TA_INHERIT || piver_type == TA_STACK) {
		mcb->pcl.pcl_param = -1;
	}
	/* 優先度逆転機構エリアが必要な場合 */
	else {
		mcb->pcl.pcl_param = pcl_param;
	}
	
 	DEBUG_OUTMSG("create mutexID for interrput handler\n");
  
  return (OBJP)mcb;
}


/*!
* システムコール処理(del_mtx():mutexコントロールブロックの排除)
* *mcb : mutexコントロールブロックへのポインタ
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER del_mtx_isr(MTXCB *mcb)
{
  /*セマフォ取得中は排除できない*/
  if ((mcb->mtxvalue) || (mcb->locks)) {
    DEBUG_OUTMSG("not delete busy mutexID for interrput handler\n");
    return EV_NDL; /*排除エラー終了*/
  }
  /*セマフォ排除操作*/
  else {
    memset(mcb, -1, sizeof(*mcb)); /* ノードを初期化 */
		/* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
		if (mcb->mtx_type &= DYNAMIC_MUTEX) {
			get_aloclist(mcb); /* alocリストから抜き取り */
			/* freeリスト先頭へ挿入 */
			mcb->next = mg_mtx_info.freehead;
			mcb->prev = mg_mtx_info.freehead->prev;
			mg_mtx_info.freehead = mcb->next->prev = mcb->prev->next = mcb;
		}
			
		DEBUG_OUTMSG("delete mutex contorol block for interrput handler\n");
		
		return E_OK;
  }
}


/*!
* mutex初期ロックをする関数
* tskid : 初期ロックをするタスクID
* *mcb : 初期ロックをするMTXCB
*/
void loc_first_mtx(int tskid, MTXCB *mcb)
{
	/* 初期ロック可能状態の場合 */
	if (mcb->mtxvalue == 0) {
   	mcb->mtxvalue += 1;
   	mcb->locks += 1;
 		/* オーナー情報のセット */
 		mcb->ownerid = tskid;
 		current = mg_tsk_info.id_table[tskid];
 		
		/* 取得情報(TCB)のセット */
 		current->get_info.flags |= TASK_GET_MUTEX;
 		current->get_info.gobjp = (GET_OBJP)mcb;
 		
 		putcurrent(); /* システムコール発行タスクをレディーへ */
 	}
	/* 本来は実行されない(実行される時は再入されたなど) */
 	else {
 		down_system();
 	}
  DEBUG_OUTMSG("get mutex semaphoreID for interrput handler\n");
}


/*!
* mutex再帰ロックをする関数
* *sb : 再帰ロックをするMTXCB
* (返却値)E_ILUSE : 多重にロックされた
* (返却値)E_OK : 正常終了
*/
ER loc_multipl_mtx(MTXCB *mcb)
{
	putcurrent(); /* オーナータスクをレディーへ */

	/* 再帰ロック上限をオーバーした場合 */
  if (mcb->maxlocks <= mcb->locks) {
		DEBUG_OUTMSG("not get multipl mutex semaphoreID for interrput handler\n");
		return E_ILUSE; /* 上限値オーバーエラー終了 */
	}
 	mcb->locks += 1;
 	DEBUG_OUTMSG("get multipl mutex semaphoreID for interrput handler\n");
 	return E_OK;
}


/*!
* mutex待ちタスクの追加
* *mcb : mutex待ちタスクを追加するMTXCB
* msec : タイム値(twai_mtx()以外の呼び出しは0となる)
*/
void wait_mtx_tsk(MTXCB *mcb, int msec)
{
	TMRCB *settbf;

	/* すでに待ちタスクが存在する場合 */
	if (mcb->waittail) {
		current->wait_info.wait_prev = mcb->waittail;
		mcb->waittail->wait_info.wait_next = current; /* システムコールを呼び出したスレッドはcurrentになっている */
 	}
	/* まだ待ちタスクが存在しない場合 */
 	else {
		mcb->waithead = current;
  }
 	mcb->waittail = current;
 	
	/* 待ち情報(TCB)をセット */
 	current->state |= TASK_WAIT_MUTEX;
  current->wait_info.wobjp = (WAIT_OBJP)mcb;
  
  /* タイムアウト値に正の値が設定されていたら，タイムアウト付きとなる */
  if (msec > TMO_POL) {
   	settbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, msec, 0, tloc_mtx_callrte, current); /* 差分のキューのノードを作成 */
		current->wait_info.tobjp = (TMR_OBJP)settbf; /* TCBのwait_infoとTMRCBの接続 */
	}
  
  DEBUG_OUTMSG("not get mutex semaphoreID for interrput handler\n");
}


/*!
* mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理
* mcb->locksは1以外となっている
* *mcb : 初期ロック状態(mxvalueが1)以外の操作をするMTXCB
* (返却値) E_OK : 再帰ロック解除
* (返却値) E_ILUSE : すでに解放されている
*/
ER not_lock_first_mtx(MTXCB *mcb)
{
	putcurrent(); /* システムコール発行スレッドをレディーへ戻す */

	/* 再帰ロック解除処理 */
  if (mcb->locks > 1) {
  	mcb->locks -= 1;
  	DEBUG_OUTMSG("release multipl mutex semaphoreID for interrput handler\n");
  	return E_OK;
  }
  /* すでにアンロック状態 */
  else {
  	DEBUG_OUTMSG("semaphoreID release yet.\n");
  	return E_ILUSE; /* すでに解放されてる.エラー終了 */
	}
}


/*!
* mutexオーナーシップのクリア
* *mcb : オーナーシップのクリアをするMTXCB
*/
void clear_mtx_ownership(MTXCB *mcb)
{

	/* オーナーとなっていたタスクの取得情報をクリア(TCBに) */
	current->get_info.flags &= ~TASK_GET_MUTEX;
  current->get_info.gobjp = 0;

	mcb->mtxvalue -= 1;
  mcb->ownerid = -1;
  mcb->locks -= 1;
  DEBUG_OUTMSG("owner ship clear.\n");
}


/*!
* mutex待ちからタスクをレディーへ戻す関数の分岐
* *mcb : 対象MTXCB
*/
void put_mtx_waittsk(MTXCB *mcb)
{
	/* ロック解除時，待ちタスクをFIFO順でレディーへ入れる */
	if (mcb->atr == MTX_TA_TFIFO) {
		put_mtx_fifo_ready(mcb);
	}
	 /* ロック解除時，待ちタスクを優先度順でレディーへ入れる */
	else {
		put_mtx_pri_ready(mcb);
	}
}


/*!
* mutexロック解除時，mutex待ちタスクを先頭からレディーへ入れる
* ラウンドロビンスケジューリングをしている時は先頭からレディーへ戻す
* *mcb : mutex待ちタスクが存在するMTXCB
*/
static void put_mtx_fifo_ready(MTXCB *mcb)
{

	/* 先頭のセマフォ待ちタスクをレディーへ */
	current = mcb->waithead;
	mcb->waithead = current->wait_info.wait_next;
	mcb->waithead->wait_info.wait_prev = NULL;
	
	mcb->ownerid = current->init.tskid; /* オーナーシップの設定 */
	
	/* currentが最尾の時tailをNULLにしておく */
	if (current->wait_info.wait_next == NULL) {
		mcb->waittail = NULL;
	}
  current->wait_info.wait_next = current->wait_info.wait_next = NULL;
  
  mcb->ownerid = current->init.tskid; /* オーナーシップの設定 */
  
  current->get_info.flags |= TASK_GET_MUTEX; /* 取得情報をセット(TCBに) */
  current->get_info.gobjp = (GET_OBJP)mcb; /* 取得情報をセット(TCBに) */
  current->state &= ~TASK_WAIT_MUTEX; /* 待ち情報のクリア(TCBに) */
  current->wait_info.wobjp = 0; /* 待ち情報のクリア(TCBに) */
  
  putcurrent();
  DEBUG_OUTMSG("wait task ready que for interrupt handler.\n");
}


/*!
* mutexロック解除時，mutex待ちタスクを優先度が高いものからレディーへ入れる
* (Priority Inheritanceでデッドロックが発生するシナリオはこちらでやらなくてはならない)
* *mcb : mutex待ちタスクが存在するMTXCB
*/
static void put_mtx_pri_ready(MTXCB *mcb)
{
	TCB *worktcb, *maxtcb;

	worktcb = maxtcb = mcb->waithead;

	/* 待ちタスクの中で最高優先度のものを探す */
	while (worktcb->wait_info.wait_next != NULL) {
		if (worktcb->priority < maxtcb->priority) {
			maxtcb = worktcb;
		}
		worktcb = worktcb->wait_info.wait_next;
	}
	/* 最後の一回分(スリープTCBは双方リストか循環リストの方が効率がいい) */
	if (worktcb->priority < maxtcb->priority) {
			maxtcb = worktcb;
	}

	get_tsk_mtx_waitque(mcb, maxtcb); /* mutex待ちキューからスリープTCBを抜き取る関数 */
		
	/* セマフォ待ちタスクの中で優先度が最高のタスクをレディーへ入れる */
	current = maxtcb;
	current->wait_info.wait_next = current->wait_info.wait_prev = NULL;
	
	mcb->ownerid = current->init.tskid; /* オーナーシップの設定 */
	
	current->get_info.flags |= TASK_GET_MUTEX; /* 取得情報をセット(TCBに) */
  current->get_info.gobjp = (GET_OBJP)mcb; /* 取得情報をセット(TCBに) */
  current->state &= ~TASK_WAIT_MUTEX; /* 待ち情報のクリア(TCBに) */
  current->wait_info.wobjp = 0; /* 待ち情報のクリア(TCBに) */
	
	putcurrent();
  DEBUG_OUTMSG("wait task ready que for interrupt handler.\n");
}


/*!
* mutex待ちキューからスリープTCBを抜き取る関数
* *mcb : 対象のMTXCB
* *maxcb : 抜き取るTCB
*/
void get_tsk_mtx_waitque(MTXCB *mcb, TCB *maxtcb)
{
	/* 先頭から抜きとる */
	if (maxtcb == mcb->waithead) {
		mcb->waithead = maxtcb->wait_info.wait_next;
		mcb->waithead->wait_info.wait_prev = NULL;
		/* mutex待ちキューにタスが一つの場合 */
		if (maxtcb == mcb->waittail) {
			mcb->waittail = NULL;
		}
	}
	/* 最尾から抜き取りる */
	else if (maxtcb == mcb->waittail) {
		mcb->waittail = maxtcb->wait_info.wait_prev;
		mcb->waittail->wait_info.wait_next = NULL;
	}
	/* 中間から抜き取る */
	else {
		maxtcb->wait_info.wait_prev->wait_info.wait_next = maxtcb->wait_info.wait_next;
		maxtcb->wait_info.wait_next->wait_info.wait_prev = maxtcb->wait_info.wait_prev;
	}
}


/*!
* システムコール処理(loc_mtx():mutexロック処理(プロトコルなし))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER loc_voidmtx_isr(MTXCB *mcb)
{
  int tskid;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */
	/* ここからロック操作 */
	if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		if (mcb->ownerid != tskid) {
			wait_mtx_tsk(mcb, TMO_POL); /* mutex待ちタスクの追加 */
    	return E_OK;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* 再帰ロックをする関数 */;
  	}
  }
  /* 初期ロック */
  else {
  	loc_first_mtx(tskid, mcb); /* 初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(ploc_mtx():mutexポーリングロック処理(プロトコルなし))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
*/
ER ploc_voidmtx_isr(MTXCB *mcb)
{
  int tskid;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */

	/* ここからロック操作 */
	if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		if (mcb->ownerid != tskid) {
			putcurrent(); /* システムコール発行スレッドを戻す */
    	return E_TMOUT;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* mutex再帰ロックをする関数 */
  	}
  }
  /* 初期ロック */
  else {
  	loc_first_mtx(tskid, mcb); /* mutex初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(tloc_mtx():mutexタイムアウト付きロック処理(プロトコルなし))
* *mcb : 対象mutexコントロールブロックへのポインタ
* msec : タイムアウト値
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了(mutexセマフォを取得，mutexセマフォ待ちタスクにつなげる)
* (返却値)E_OK，E_ILUSE : dynamic_multipl_lock()の返却値(再帰ロック完了，多重再帰ロック)
* (返却値)E_TMOUT : タイムアウト
*/
ER tloc_voidmtx_isr(MTXCB *mcb, int msec)
{
  int tskid;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */

	/* パラメータエラーチェック */
	if (msec < TMO_FEVR) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		return E_PAR;
	}
	/* ここからロック操作 */
	else if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		if (mcb->ownerid != tskid) {
			/* 取得できない場合で，ポーリングが設定された */
 			if (msec == TMO_POL) {
 				putcurrent(); /* システムコール発行スレッドを戻す */
  			return E_TMOUT;
  		}
    	wait_mtx_tsk(mcb, msec); /* mutexセマフォ待ちタスクの追加 */
    	return E_OK;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* mutex再帰ロックをする関数 */
  	}
  }
  /* 初期ロック */
  else {
  	loc_first_mtx(tskid, mcb); /* mutex初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(unl_mtx():mutexアンロック処理(プロトコルなし))
* *mcb : 対象mutexコントロールブロック
* (返却値)E_OK : mutexセマフォ待ちタスクへ割り当て，mutexセマフォの解放，
* 							再帰ロック解除(not_lock_first_mtx()の返却値)
* (返却値)E_ILUSE : オーナータスクでない場合，mutexセマフォ解放エラー
* 									(すでに解放済み，not_lock_first_mtx()の返却値)
*/
ER unl_voidmtx_isr(MTXCB *mcb)
{
	int tskid;

  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */

	/* スリープTCBは一度にすべて戻してはいけない */

  /* ここからアンロック操作 */
  if (mcb->ownerid == tskid) { /* オーナータスクか */
  	/* 再帰ロック解除状態の場合 */
  	if (mcb->locks == 1) {
  		/* 待ちタスクが存在する場合 */
			if (mcb->waithead != NULL) {
				/* オーナータスクをレディーへ */
				current = mg_tsk_info.id_table[tskid];
				/* 取得情報クリア */
				current->get_info.flags &= ~TASK_GET_MUTEX;
				current->get_info.gobjp = 0;
				putcurrent();
				put_mtx_waittsk(mcb); /* 待ちタスクをレディーへ戻す */
    		return E_OK;
  		}
  		/* 待ちタスクが存在しなければ，オーナーシップのクリア */
  		else {
  			current = mg_tsk_info.id_table[tskid];
				putcurrent(); /* システムコール発行スレッドをレディーへ */
				clear_mtx_ownership(mcb); /* オーナーシップのクリアをする関数 */
  			return E_OK;
  		}
  	}
		return not_lock_first_mtx(mcb); /* 初期ロック状態(mtxvalueが1)以外の場合の場合の処理 */
	}
  /* オーナータスクでない場合 */
  else {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("not release mutexID for interrput handler\n");
  	return E_ILUSE;
  }
}


/*!
* mutex(プロトコルなし)を無条件解放する(ext_tsk()またはexd_tsk()の延長で呼ばれる関数)
* mutexの場合の無条件解放はオーナーシップのクリアとなる(再帰ロックカウント一つではない)
* *mcb : *worktcbが保持している動的mutexのコントロールブロックのポインタ
* (返却値)E_OK : mutex待ちタスクへ割り当て，オーナーシップクリア
*/
ER unl_voidmtx_condy(MTXCB *mcb)
{
	/* スリープTCBは一度にすべて戻してはいけない */
	
	/* 待ちタスクが存在する場合 */
	if (mcb->waithead != NULL) {
		/* ext_tsk()などから呼ばれるので，オーナータスクは戻さない */
		/* 取得情報クリア */
		current->get_info.flags &= ~TASK_GET_MUTEX;
		current->get_info.gobjp = 0;
		put_mtx_waittsk(mcb); /* タスクをレディーへ戻す */
		mcb->locks = 1; /* 無条件解放(取得分の解放処理がないため)のためlocks(再帰ロック数)を待ちタスクが獲得した状態にしておく */
    return E_OK;
  }
  /* 待ちタスクが存在しなければ，オーナーシップのクリア */
  else {
  	/* ext_tsk()などから呼ばれるので，オーナータスクは戻さない */
		clear_mtx_ownership(mcb); /* オーナーシップのクリアをする関数 */
  	return E_OK;
  }
}
