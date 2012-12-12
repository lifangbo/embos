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
#include "c_lib/lib.h"
#include "memory.h"
#include "semaphore.h"
#include "ready.h"
#include "target/driver/timer_driver.h"
#include "timer_callrte.h"


/*! セマフォの初期化(静的型semaphoreの領域確保と初期化(可変長配列として使用する)) */
static ER static_sem_init(int semids);

/*! セマフォの初期化(動的型semaphoreの領域確保と初期化(freeリストを作成する)) */
static ER dynamic_sem_init(void);

/*! 動的semaphore alocリストからセマフォコントロールブロックを抜き取る */
static void get_aloclist(SEMCB *scb);

/*! semaphore待ちからタスクをレディーへ戻す関数の分岐 */
static void put_sem_waittsk(SEMCB *scb);

/*! セマフォロック解除時，セマフォ待ちタスクをFIFO順でレディーへ入れる */
static void put_sem_fifo_ready(SEMCB *scb);

/*! セマフォロック解除時，セマフォ待ちタスクを優先度が高いものからレディーへ入れる */
static void put_sem_pri_ready(SEMCB *scb);


/*!
* セマフォの初期化(semaphore ID変換テーブルの領域確保と初期化)
* start_init_tsk()で呼ばれる場合と，semaphoreIDが足らなくなった場合に呼ばれる
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER sem_init(void)
{
	int semids, i;
	SEMCB *scb;
	
	semids = SEMAPHORE_ID_NUM << mg_sem_info.power_count; /* 倍の領域を確保する(start_init_tsk()の時はデフォルト通り) */
	mg_sem_info.id_table = (SEMCB **)get_mpf_isr(sizeof(scb) * semids); /* 変換テーブルの動的メモリ確保 */
	/* semaphoreID変換テーブルの初期化(メモリにNULLを埋めるのにmemset()は使用できない) */
	for (i = 0; i < semids; i++) {
		mg_sem_info.id_table[i] = NULL;
	}
	/* SEMCBの静的型配列の作成及び初期化とSEMCBの動的型freeリストの作成と初期化 */
	if ((E_NOMEM == static_sem_init(semids)) || (E_NOMEM == dynamic_sem_init())) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	
	return E_OK;
}


/*!
* セマフォの初期化(静的型semaphoreの領域確保と初期化(可変長配列として使用する))
* semids : 確保する可変長配列の個数
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER static_sem_init(int semids)
{
	SEMCB *scb;
	int size = sizeof(*scb) * semids; /* 可変長配列のサイズ */
	
	mg_sem_info.array = (SEMCB *)get_mpf_isr(size); /* 可変長配列確保 */
	if (mg_sem_info.array == NULL) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	/* 確保した可変長配列を初期化(可変長でも配列はメモリに連続しているのでこれでOK) */
	memset(mg_sem_info.array, -1, size);
	
	return E_OK;
}

/*!
* セマフォの初期化(動的型semaphoreの領域確保と初期化(freeリストを作成する))
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER dynamic_sem_init(void)
{
	int semids, i;
	SEMCB *scb;
	
	mg_sem_info.freehead = mg_sem_info.alochead = NULL;
	
	/* start_init_tsk()の時以外 */
	if (mg_sem_info.power_count) {
		semids = SEMAPHORE_ID_NUM << (mg_sem_info.power_count - 1); /* 現在と同じ領域を確保する */
	}
	/* start_init_tsk()の時 */
	else {
		semids = SEMAPHORE_ID_NUM << mg_sem_info.power_count;
	}
	
	for (i = 0; i < semids; i++) {
		scb = (SEMCB *)get_mpf_isr(sizeof(*scb)); /* ノードのメモリ確保 */
		/*メモリが取得できない*/
  	if(scb == NULL) {
  		return E_NOMEM;	 /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
  	}
		memset(scb, -1, sizeof(*scb)); /* 確保したノードを初期化 */
		/* freeキューの作成 */
		scb->next = mg_sem_info.freehead;
		scb->prev = NULL;
		mg_sem_info.freehead = scb->next->prev = scb;
	}
	
	return E_OK;
}


/*!
* 動的semaphore alocリストからセマフォコントロールブロックを抜き取る
* scb : 対象のSEMCB
*/
static void get_aloclist(SEMCB *scb)
{
	/* 先頭から抜きとる */
	if (scb == mg_sem_info.alochead) {
		mg_sem_info.alochead = scb->next;
		mg_sem_info.alochead->prev = NULL;
	}
	/* それ以外から抜き取る */
	else {
		scb->prev->next = scb->next;
		scb->next->prev = scb->prev;
	}
}


/*!
* semaphore待ちからタスクをレディーへ戻す関数の分岐
* scb : 対象SEMCB
*/
static void put_sem_waittsk(SEMCB *scb)
{
	/* ロック解除時，待ちタスクをFIFO順でレディーへ入れる */
	if (scb->atr == SEM_TA_TFIFO) {
		put_sem_fifo_ready(scb);
	}
	 /* ロック解除時，待ちタスクを優先度順でレディーへ入れる */
	else {
		put_sem_pri_ready(scb);
	}
}


/*!
* セマフォロック解除時，セマフォ待ちタスクをFIFO順でレディーへ入れる
* ラウンドロビンスケジューリングをしている時は先頭からレディーへ戻す
* *scb : セマフォ待ちタスクが存在するSEMCB
*/
static void put_sem_fifo_ready(SEMCB *scb)
{
	/* 先頭の待ちタスクをレディーへ */
	current = scb->waithead;
	scb->waithead = current->wait_info.wait_next;
	scb->waithead->wait_info.wait_prev = NULL;
	/*currentが最尾の時tailをNULLにしておく*/
	if (current->wait_info.wait_next == NULL) {
		scb->waittail = NULL;
	}
  current->wait_info.wait_next = current->wait_info.wait_prev = NULL;
  
  current->get_info.flags |= TASK_GET_SEMAPHORE; /* 取得情報をセット(TCBに) */
  current->get_info.gobjp = (GET_OBJP)scb; /* 取得情報をセット(TCBに) */
  current->state &= ~TASK_WAIT_SEMAPHORE; /* 待ち情報のクリア(TCBに) */
  current->wait_info.wobjp = 0; /* 待ち情報のクリア(TCBに) */
  
  putcurrent(); /* セマフォ待ちタスクをレディーへ */
  DEBUG_OUTMSG("wait task ready que for interrupt handler.\n");
}


/*!
* セマフォロック解除時，セマフォ待ちタスクを優先度が高いものからレディーへ入れる
* 優先度スケジューリングをしている時は優先度順でレディーに戻す
* *scb : セマフォ待ちタスクが存在するSEMCB
*/
static void put_sem_pri_ready(SEMCB *scb)
{
	TCB *worktcb, *maxtcb;

	worktcb = maxtcb = scb->waithead;

	/* 待ちタスクの中で最高優先度のものを探す */
	while (worktcb->wait_info.wait_next != NULL) {
		if (worktcb->priority < maxtcb->priority) {
			maxtcb = worktcb;
		}
		worktcb = worktcb->wait_info.wait_next;
	}
	/* 最後の一回分(スリープTCBは循環リストの方が効率がいいかも) */
	if (worktcb->priority < maxtcb->priority) {
			maxtcb = worktcb;
	}

	get_tsk_sem_waitque(scb, maxtcb); /* セマフォ待ちキューからスリープTCBを抜き取る関数 */
		
	/* セマフォ待ちタスクの中で優先度が最高のタスクをレディーへ入れる */
	current = maxtcb;
	current->wait_info.wait_next = current->wait_info.wait_prev = NULL;
	
	current->get_info.flags |= TASK_GET_SEMAPHORE; /* 取得情報をセット(TCBに) */
  current->get_info.gobjp = (GET_OBJP)scb; /* 取得情報をセット(TCBに) */
  current->state &= ~TASK_WAIT_SEMAPHORE; /* 待ち情報のクリア(TCBに) */
  current->wait_info.wobjp = 0; /* 待ち情報のクリア(TCBに) */
  
	putcurrent();
  DEBUG_OUTMSG("wait task ready que for interrupt handler.\n");
}


/*!
* セマフォ待ちキューからスリープTCBを抜き取る関数
* *scb : 対象のSEMCB
* *maxcb : 抜き取るTCB
*/
void get_tsk_sem_waitque(SEMCB *scb, TCB *maxtcb)
{
	/* 先頭から抜きとる */
	if (maxtcb == scb->waithead) {
		scb->waithead = maxtcb->wait_info.wait_next;
		scb->waithead->wait_info.wait_prev = NULL;
		/* セマフォ待ちキューにタスクが一つの場合 */
		if (maxtcb == scb->waittail) {
			scb->waittail = NULL;
		}
	}
	/* 最尾から抜き取りる */
	else if (maxtcb == scb->waittail) {
		scb->waittail = maxtcb->wait_info.wait_prev;
		scb->waittail->wait_info.wait_next = NULL;
	}
	/* 中間から抜き取る */
	else {
		maxtcb->wait_info.wait_prev->wait_info.wait_next = maxtcb->wait_info.wait_next;
		maxtcb->wait_info.wait_next->wait_info.wait_prev = maxtcb->wait_info.wait_prev;
	}
}


/*!
* システムコール処理(acre_sem():セマフォコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* atr : タスクをレディーへ戻すアルゴリズム(FIFO順か?優先度順か?)
* semvalue : セマフォ初期値
* maxvalue : セマフォ最大値
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)scb : 正常終了(作成したセマフォコントロールブロックへのポインタ)
*/
OBJP acre_sem_isr(SEM_TYPE type, SEM_ATR atr, int semvalue, int maxvalue)
{
  SEMCB *scb;
  
  /* 静的型semaphoreの場合 */
  if (type == STATIC_SEMAPHORE) {
  	scb = &mg_sem_info.array[mg_sem_info.counter];
  }
  /* 動的型semaphoreの場合 */
  else {
  	scb = mg_sem_info.freehead; /* free headからノードを与える(抜き取るわけではない) */
  	mg_sem_info.freehead = scb->next; /* free headを一つ進める */
  	/* aloc headが空の場合 */
  	if (mg_sem_info.alochead == NULL) {
  		mg_sem_info.alochead = scb;
  	}
  }
  scb->semid = mg_sem_info.counter;
	scb->type = type;
	scb->atr = atr;
  scb->semvalue = semvalue;
  scb->maxvalue = maxvalue;
  scb->waithead = scb->waittail = NULL;

 	DEBUG_OUTMSG("create general semaphoreID for interrput handler\n");

	return (OBJP)scb;
}


/*!
* システムコール処理(del_sem():セマフォコントロールブロックの排除)
* セマフォ待ちタスクが存在する状態で排除はできないようにしているので，E_DLTエラーはない
* *scb : セマフォコントロールブロックへのポインタ
* (返却値)EV_NDL : セマフォ取得中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER del_sem_isr(SEMCB *scb)
{
  /* セマフォ取得中は排除できない */
  if (scb->waithead != NULL) {
    DEBUG_OUTMSG("not delete busy semaphoreID for interrput handler\n");
    return EV_NDL; /* 排除エラー終了 */
  }
  /* セマフォ排除操作 */
  else {
    memset(scb, -1, sizeof(*scb)); /* ノードを初期化 */
		/* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
		if (scb->type &= DYNAMIC_SEMAPHORE) {
			get_aloclist(scb); /* alocリストから抜き取り */
			/* freeリスト先頭へ挿入 */
			scb->next = mg_sem_info.freehead;
			scb->prev = mg_sem_info.freehead->prev;
			mg_sem_info.freehead = scb->next->prev = scb->prev->next = scb;
		}
		
		DEBUG_OUTMSG("delete semaphore contorol block for interrput handler\n");
		
		return E_OK;
  }
}


/*!
* システムコール処理(sig_sem():セマフォV操作)
* *scb : セマフォコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(セマフォの解放，待ちタスクへの割り当て)
* (返却値)E_QOVR : キューイングオーバフロー(セマフォ解放エラー(すでに解放済み))，上限値を越えた
*/
ER sig_sem_isr(SEMCB *scb)
{
	/*スリープTCBは一度にすべて戻してはいけない*/
  
  /*セマフォ待ちタスクへの割り当て*/
	if (scb->waithead != NULL) {
		/*取得情報のクリア*/
		current->get_info.flags &= ~TASK_GET_SEMAPHORE;
		current->get_info.gobjp = 0;
		put_sem_waittsk(scb); /* タスクをレディーへ戻す */
    return E_OK;
  }
  /*セマフォの解放*/
  else if (!(scb->semvalue) || (scb->semvalue) < (scb->maxvalue)) {
  	scb->semvalue += 1;
  	/*取得情報クリア*/
  	if (scb->semvalue == scb->maxvalue) {
			current->get_info.flags &= ~TASK_GET_SEMAPHORE;
			current->get_info.gobjp = 0;
  	}
  	DEBUG_OUTMSG("release to general semaphoreID for interrupt handler.\n");
  	return E_OK;
  }
  /*
	セマフォ値が初期設定値を越えた，またはすでに解放済み
	if ((sb->semvalue) >= (sb->maxvalue))のような状態
	*/
  else {
		DEBUG_OUTMSG("general semaphoreID release yet.\n");
  	return E_QOVR;
  }
}


/*!
* システムコール処理(wai_sem():セマフォP操作)
* セマフォ待ちタスクが存在する状態で排除はできないようにしているので，E_DLTエラーはない
* *scb : セマフォコントロールブロックへのポインタ
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
*/
ER wai_sem_isr(SEMCB *scb)
{
	/* P操作 */
  if (scb->semvalue >= 1) {
  	current->get_info.flags |= TASK_GET_SEMAPHORE; /* 取得情報をセット(TCBに) */
  	current->get_info.gobjp = (GET_OBJP)scb; /* 取得情報をセット(TCBに) */
    scb->semvalue -= 1;
    putcurrent(); /* システムコール発行スレッドをレディーへ */
    DEBUG_OUTMSG("get general semaphoreID for interrput handler\n");
    return E_OK;
  }
  /* セマフォ待ちタスクの追加 */
  else {
  	if (scb->waittail) {
			current->wait_info.wait_prev = scb->waittail;
			scb->waittail->wait_info.wait_next = current;
    }
    else {
			scb->waithead = current;
    }
    scb->waittail = current;
    current->state |= TASK_WAIT_SEMAPHORE; /* 待ち情報をセット(TCBに) */
    current->wait_info.wobjp = (WAIT_OBJP)scb; /* 待ち情報をセット(TCBに) */
    DEBUG_OUTMSG("not get general semaphoreID for interrput handler\n");
    return E_OK;
  }
}


/*!
* セマフォP操作(ポーリング)
* *scb : セマフォコントロールブロックへのポインタ
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
* (返却値)E_TMOUT : セマフォ待ち失敗(ポーリング)
*/
ER pol_sem_isr(SEMCB *scb)
{  
	/*P操作*/
  if (scb->semvalue >= 1) {
  	current->get_info.flags |= TASK_GET_SEMAPHORE; /*取得情報をセット(TCBに)*/
  	current->get_info.gobjp = (GET_OBJP)scb; /*取得情報をセット(TCBに)*/
    scb->semvalue -= 1;
    putcurrent(); /*システムコール発行スレッドをレディーへ*/
    DEBUG_OUTMSG("get general semaphoreID for interrput handler\n");
    return E_OK;
  }
  /*取得できない時は，セマフォ待ちタスクの追加はせずレディーへ戻し，E_TMOUTを返す*/
  else {
  	putcurrent();
  	return E_TMOUT;
  }
}


/*!
* セマフォP操作(タイムアウト付き)
* タイムアウト時のエラーコードのE_TMOUTはコールバックルーチンの方で呼ぶ
* *scb : セマフォコントロールブロックへのポインタ
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : セマフォ取得，セマフォ待ちタスクの追加
* (返却値)E_TMOUT : タイムアウト
*/
ER twai_sem_isr(SEMCB *scb, int msec)
{
	TMRCB *settbf;

	/* パラメータ(msec)は正しいか? */
  if (msec < TMO_FEVR) {
  	return E_PAR;
  }
	/* ここでP操作 */
  else if (scb->semvalue >= 1) {
  	current->get_info.flags |= TASK_GET_SEMAPHORE; /* 取得情報をセット(TCBに) */
  	current->get_info.gobjp = (GET_OBJP)scb; /* 取得情報をセット(TCBに) */
    scb->semvalue -= 1;
    putcurrent(); /* システムコール発行スレッドをレディーへ */
    DEBUG_OUTMSG("get general semaphoreID for interrput handler\n");
    return E_OK;
  }
  /* 取得できない場合で，ポーリングが設定された */
  else if (msec == TMO_POL) {
  	return E_TMOUT;
  }
  /* セマフォ待ちタスクの追加 */
  else {
  	if (scb->waittail) {
			current->wait_info.wait_prev = scb->waittail;
			scb->waittail->wait_info.wait_next = current;
    }
    else {
			scb->waithead = current;
    }
    scb->waittail = current;
    current->state |= TASK_WAIT_SEMAPHORE; /* 待ち情報をセット(TCBに) */
    current->wait_info.wobjp = (WAIT_OBJP)scb; /* 待ち情報をセット(TCBに) */

    /* タイムアウト値に正の値が設定されていたら，タイムアウト付きとなる */
    if (msec > TMO_POL) {
    	settbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, msec, 0, twai_sem_callrte, current); /* 差分のキューのノードを作成 */
			current->wait_info.tobjp = (TMR_OBJP)settbf; /* TCBのwait_infoとTMRCBの接続 */
		}
    DEBUG_OUTMSG("not get general semaphoreID for interrput handler\n");
    return E_OK;
  }
}


/*!
* セマフォを無条件解放する(ext_tsk()またはexd_tsk()の延長などで呼ばれる関数)
* セマフォの場合の無条件解放は一つのみ解放
* gobjp : 解放するカーネルオブジェクトのポインタ
* (返却値)E_OBJ : カーネルオブジェクトポインタが登録されていない
* (返却値)E_OK : セマフォ無条件解放，セマフォ無条件待ちタスクの追加
*/
ER sig_sem_condy(SEMCB *scb)
{
	/* 本来ここは実行されない */
	if (scb == NULL) {
		DEBUG_OUTMSG("not agree kernel object pointer.\n");
		return E_OBJ;
	}
	
	/* スリープTCBは一度にすべて戻してはいけない */
	
	/* ext_tsk()などから呼ばれるので，オーナータスクは戻さない */
	/* 取得情報クリア */
	current->get_info.flags &= ~TASK_GET_SEMAPHORE;
	current->get_info.gobjp = 0;
	/* セマフォ待ちタスクへの割り当て */
	if (scb->waithead != NULL) {
		put_sem_waittsk(scb); /* タスクをレディーへ戻す */
    return E_OK;
  }
  /* セマフォの解放 */
  else {
    scb->semvalue += 1;
    DEBUG_OUTMSG("release to general semaphoreID for interrupt handler.\n");
  	return E_OK;
  }
}
