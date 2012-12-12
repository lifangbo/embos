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


#include "mailbox.h"
#include "memory.h"
#include "c_lib/lib.h"
#include "ready.h"
#include "target/driver/timer_driver.h"
#include "timer_callrte.h"


/*! メールボックスの初期化(静的型mailboxの領域確保と初期化(可変長配列として使用する)) */
static ER static_mbx_init(int mbxids);

/*! メールボックスの初期化(動的型mailboxの領域確保と初期化(freeリストを作成する)) */
static ER dynamic_mbx_init(void);

/*! 動的mailbox alocリストからメールボックスコントロールブロックを抜き取る */
static void get_aloclist(MBXCB *mbcb);

/*! 各メッセージ属性に従って,メッセージを返却する関数へ分岐 */
static void put_mbx_retmsg(MBXCB *mbcb);

/*! メッセージ受信時,メッセージをFIFO順で返却する */
static void put_mbx_msg_fiforet(MBXCB *mbcb);

/*! メッセージ受信時,メッセージを優先度順で返却する */
static void put_mbx_msg_priret(MBXCB *mbcb);

/*! メールボックスのメッセージキューからメッセージを抜き取る関数 */
static void get_msg_mbx_msgque(MBXCB *mbcb, T_MSG *maxmsg);

/*! mailbox待ちからタスクをレディーへ戻す関数の分岐 */
static void put_mbx_waittsk(MBXCB *mbcb);

/*! メールボックス送信時,メッセージ待ちタスクをFIFO順でレディーへ入れる */
static void put_mbx_tsk_fifoready(MBXCB *mbcb);

/*! メールボックス送信時,メールボックス待ちタスクを優先度が高いものからレディーへ入れる */
static void put_mbx_tsk_priready(MBXCB *mbcb);


/*!
* メールボックスの初期化(mailbox ID変換テーブルの領域確保と初期化)
* start_init_tsk()で呼ばれる場合と，mailboxIDが足らなくなった場合に呼ばれる
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER mbx_init(void)
{
	int mbxids, i;
	MBXCB *mbcb;
	
	mbxids = MAILBOX_ID_NUM << mg_mbx_info.power_count; /* 倍の領域を確保する(start_init_tsk()の時はデフォルト通り) */
	mg_mbx_info.id_table = (MBXCB **)get_mpf_isr(sizeof(mbcb) * mbxids); /* 変換テーブルの動的メモリ確保 */
	/* mailboxID変換テーブルの初期化(メモリにNULLを埋めるのにmemset()は使用できない) */
	for (i = 0; i < mbxids; i++) {
		mg_mbx_info.id_table[i] = NULL;
	}
	/* MBXCBの静的型配列の作成及び初期化とMBXCBの動的型freeリストの作成と初期化 */
	if ((E_NOMEM == static_mbx_init(mbxids)) || (E_NOMEM == dynamic_mbx_init())) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	
	return E_OK;
}


/*!
* メールボックスの初期化(静的型mailboxの領域確保と初期化(可変長配列として使用する))
* mbxids : 確保する可変長配列の個数
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER static_mbx_init(int mbxids)
{
	MBXCB *mbcb;
	int size = sizeof(*mbcb) * mbxids; /* 可変長配列のサイズ */
	
	mg_mbx_info.array = (MBXCB *)get_mpf_isr(size); /* 可変長配列確保 */
	if (mg_mbx_info.array == NULL) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	/* 確保した可変長配列を初期化(可変長でも配列はメモリに連続しているのでこれでOK) */
	memset(mg_mbx_info.array, -1, size);
	
	return E_OK;
}

/*!
* メールボックスの初期化(動的型mailboxの領域確保と初期化(freeリストを作成する))
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER dynamic_mbx_init(void)
{
	int mbxids, i;
	MBXCB *mbcb;
	
	mg_mbx_info.freehead = mg_mbx_info.alochead = NULL;
	
	/* start_init_tsk()の時以外 */
	if (mg_mbx_info.power_count) {
		mbxids = MAILBOX_ID_NUM << (mg_mbx_info.power_count - 1); /* 現在と同じ領域を確保する */
	}
	/* start_init_tsk()の時 */
	else {
		mbxids = MAILBOX_ID_NUM << mg_mbx_info.power_count;
	}
	
	for (i = 0; i < mbxids; i++) {
		mbcb = (MBXCB *)get_mpf_isr(sizeof(*mbcb)); /* ノードのメモリ確保 */
		/*メモリが取得できない*/
  	if(mbcb == NULL) {
  		return E_NOMEM;	 /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
  	}
		memset(mbcb, -1, sizeof(*mbcb)); /* 確保したノードを初期化 */
		/* freeキューの作成 */
		mbcb->next = mg_mbx_info.freehead;
		mbcb->prev = NULL;
		mg_mbx_info.freehead = mbcb->next->prev = mbcb;
	}
	
	return E_OK;
}


/*!
* 動的mailbox alocリストからメールボックスコントロールブロックを抜き取る
* mbcb : 対象のMBXCB
*/
static void get_aloclist(MBXCB *mbcb)
{
	/* 先頭から抜きとる */
	if (mbcb == mg_mbx_info.alochead) {
		mg_mbx_info.alochead = mbcb->next;
		mg_mbx_info.alochead->prev = NULL;
	}
	/* それ以外から抜き取る */
	else {
		mbcb->prev->next = mbcb->next;
		mbcb->next->prev = mbcb->prev;
	}
}


/*!
* 各メッセージ属性に従って,メッセージを返却する関数へ分岐
* mbcb : 対象MBXCB
*/
static void put_mbx_retmsg(MBXCB *mbcb)
{
	/* メッセージ送信時，待ちタスクをFIFO順でレディーへ入れる */
	if (mbcb->msg_atr == MSG_TA_MFIFO) {
		put_mbx_msg_fiforet(mbcb);
	}
	/* メッセージ送信時，待ちタスクを優先度順でレディーへ入れる */
	else {
		put_mbx_msg_priret(mbcb);
	}
}


/*!
* メッセージ受信時,メッセージをFIFO順で返却する
* *mbcb : メッセージが存在するメールボックス
*/
static void put_mbx_msg_fiforet(MBXCB *mbcb)
{
	T_MSG *msg;
	
	/* 先頭のメッセージを抜き取る */
	msg = mbcb->mkhead;
	mbcb->mkhead = msg->next;
	mbcb->mkhead->prev = NULL;
	/*currentが最尾の時tailをNULLにしておく*/
	if (msg->next == NULL) {
		mbcb->mktail = NULL;
	}
  msg->next = msg->prev = NULL;
  
  /* メッセージパケットへの先頭番地を受信タスクに渡す */
	*(current->syscall_info.param->un.rcv_mbx.pk_msg) = msg;
}


/*!
* メッセージ受信時,メッセージを優先度順で返却する
* *mbcb : メッセージが存在するメールボックス
*/
static void put_mbx_msg_priret(MBXCB *mbcb)
{

	T_MSG *workmsg, *maxmsg;

	workmsg = maxmsg = mbcb->mkhead;

	/* メッセージで最高優先度のものを探す */
	while (workmsg->next != NULL) {
		if (workmsg->msgpri < maxmsg->msgpri) {
			maxmsg = workmsg;
		}
		workmsg = workmsg->next;
	}
	/* 最後の一回分(メッセージキューは循環リストの方が効率がいいかも) */
	if (workmsg->msgpri < maxmsg->msgpri) {
			maxmsg = workmsg;
	}

	get_msg_mbx_msgque(mbcb, maxmsg); /* メッセージキューからメッセージを抜き取る関数 */
		
	maxmsg->next = maxmsg->prev = NULL;

	/* メッセージパケットへの先頭番地を受信タスクに渡す */
	*(current->syscall_info.param->un.rcv_mbx.pk_msg) = maxmsg;
}


/*!
* メールボックスのメッセージキューからメッセージを抜き取る関数
* *mbcb : 対象のMBXCB
* *maxmsg : 抜き取るメッセージ
*/
static void get_msg_mbx_msgque(MBXCB *mbcb, T_MSG *maxmsg)
{

	/* 先頭から抜きとる */
	if (maxmsg == mbcb->mkhead) {
		mbcb->mkhead = maxmsg->next;
		mbcb->mkhead->prev = NULL;
		/* メッセージキューにメッセージが一つの場合 */
		if (maxmsg == mbcb->mktail) {
			mbcb->mktail = NULL;
		}
	}
	/* 最尾から抜き取りる */
	else if (maxmsg == mbcb->mktail) {
		mbcb->mktail = maxmsg->prev;
		mbcb->mktail->next = NULL;
	}
	/* 中間から抜き取る */
	else {
		maxmsg->prev->next = maxmsg->next;
		maxmsg->next->prev = maxmsg->prev;
	}
}


/*!
* mailbox待ちからタスクをレディーへ戻す関数の分岐
* mbcb : 対象MBXCB
*/
void put_mbx_waittsk(MBXCB *mbcb)
{
	/* メッセージ送信時，待ちタスクをFIFO順でレディーへ入れる */
	if (mbcb->wai_atr == MBX_TA_TFIFO) {
		put_mbx_tsk_fifoready(mbcb);
	}
	/* メッセージ送信時，待ちタスクを優先度順でレディーへ入れる */
	else {
		put_mbx_tsk_priready(mbcb);
	}
}


/*!
* メールボックス送信時,メッセージ待ちタスクをFIFO順でレディーへ入れる
* ラウンドロビンスケジューリングをしている時は先頭からレディーへ戻す
* *mbcb : メッセージ待ちタスクが存在するMBXCB
*/
static void put_mbx_tsk_fifoready(MBXCB *mbcb)
{
	/* 先頭の待ちタスクをレディーへ */
	current = mbcb->waithead;
	mbcb->waithead = current->wait_info.wait_next;
	mbcb->waithead->wait_info.wait_prev = NULL;
	/*currentが最尾の時tailをNULLにしておく*/
	if (current->wait_info.wait_next == NULL) {
		mbcb->waittail = NULL;
	}
  current->wait_info.wait_next = current->wait_info.wait_prev = NULL;
  
  current->state &= ~TASK_WAIT_MAILBOX; /* 待ち情報のクリア(TCBに) */
  current->wait_info.wobjp = 0; /* 待ち情報のクリア(TCBに) */
  
  putcurrent(); /* メールボックス待ちタスクをレディーへ */
  DEBUG_OUTMSG("wait task ready que for interrupt handler.\n");
}


/*!
* メールボックス送信時,メールボックス待ちタスクを優先度が高いものからレディーへ入れる
* 優先度スケジューリングをしている時は優先度順でレディーに戻す
* *mbcb : メールボックス待ちタスクが存在するMBXCB
*/
static void put_mbx_tsk_priready(MBXCB *mbcb)
{
	TCB *worktcb, *maxtcb;

	worktcb = maxtcb = mbcb->waithead;

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

	get_tsk_mbx_waitque(mbcb, maxtcb); /* メールボックス待ちキューからスリープTCBを抜き取る関数 */
		
	/* メールボックス待ちタスクの中で優先度が最高のタスクをレディーへ入れる */
	current = maxtcb;
	current->wait_info.wait_next = current->wait_info.wait_prev = NULL;
	
  current->state &= ~TASK_WAIT_MAILBOX; /* 待ち情報のクリア(TCBに) */
  current->wait_info.wobjp = 0; /* 待ち情報のクリア(TCBに) */
  
	putcurrent();
  DEBUG_OUTMSG("wait task ready que for interrupt handler.\n");
}


/*!
* メールボックス待ちキューからスリープTCBを抜き取る関数
* *mbcb : 対象のMBXCB
* *maxtcb : 抜き取るTCB
*/
void get_tsk_mbx_waitque(MBXCB *mbcb, TCB *maxtcb)
{
	/* 先頭から抜きとる */
	if (maxtcb == mbcb->waithead) {
		mbcb->waithead = maxtcb->wait_info.wait_next;
		mbcb->waithead->wait_info.wait_prev = NULL;
		/* メールボックス待ちキューにタスクが一つの場合 */
		if (maxtcb == mbcb->waittail) {
			mbcb->waittail = NULL;
		}
	}
	/* 最尾から抜き取りる */
	else if (maxtcb == mbcb->waittail) {
		mbcb->waittail = maxtcb->wait_info.wait_prev;
		mbcb->waittail->wait_info.wait_next = NULL;
	}
	/* 中間から抜き取る */
	else {
		maxtcb->wait_info.wait_prev->wait_info.wait_next = maxtcb->wait_info.wait_next;
		maxtcb->wait_info.wait_next->wait_info.wait_prev = maxtcb->wait_info.wait_prev;
	}
}


/*!
* システムコール処理(acre_mbx():メールボックスコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* atr : タスクをレディーへ戻すアルゴリズム(FIFO順か?優先度順か?)
* max_msgpri : 送信されるメッセージ優先度の最大値
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)mbxid : 正常終了(作成したメールボックスID)
*/
OBJP acre_mbx_isr(MBX_TYPE type, MBX_MATR msg_atr, MBX_WATR wai_atr, int max_msgpri)
{
	MBXCB *mbcb;
  
  /* ここから生成(MBXCBの初期設定) */
  /* 静的型mailboxの場合 */
  if (type == STATIC_MAILBOX) {
  	mbcb = &mg_mbx_info.array[mg_mbx_info.counter];
  }
  /* 動的型mailboxの場合 */
  else {
  	mbcb = mg_mbx_info.freehead; /* free headからノードを与える(抜き取るわけではない) */
  	mg_mbx_info.freehead = mbcb->next; /* free headを一つ進める */
  	/* aloc headが空の場合 */
  	if (mg_mbx_info.alochead == NULL) {
  		mg_mbx_info.alochead = mbcb;
  	}
  }
	mbcb->mbxid = mg_mbx_info.counter;
	mbcb->type = type;
	mbcb->msg_atr = msg_atr;
	mbcb->wai_atr = wai_atr;
	mbcb->tobjp = 0;
  mbcb->mkhead = mbcb->mktail = NULL;
	mbcb->waithead = mbcb->waittail = NULL;

 	DEBUG_OUTMSG("create general mailboxID for interrput handler\n");
 
 	return (OBJP)mbcb;
}


/*!
* システムコール処理(del_mbx():メールボックスコントロールブロックの排除)
* メールボックス待ちタスクが存在する状態で排除はできないようにしているので，E_DLTエラーはない
* *mbcb : メールボックスコントロールブロックへのポインタ
* (返却値)EV_NDL : メールボックス使用中は排除できない
* (返却値)E_OK : 正常終了(排除完了)
*/
ER del_mbx_isr(MBXCB *mbcb)
{
	
  /* メールボックス使用中は排除できない */
  if (mbcb->mkhead != NULL || mbcb->waithead) {
    DEBUG_OUTMSG("not delete busy mailboxID for interrput handler\n");
    return EV_NDL; /* 排除エラー終了 */
  }
  /* メールボックス排除操作 */
  else {
    memset(mbcb, -1, sizeof(*mbcb)); /* ノードを初期化 */
		/* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
		if (mbcb->type &= DYNAMIC_MAILBOX) {
			get_aloclist(mbcb); /* alocリストから抜き取り */
			/* freeリスト先頭へ挿入 */
			mbcb->next = mg_mbx_info.freehead;
			mbcb->prev = mg_mbx_info.freehead->prev;
			mg_mbx_info.freehead = mbcb->next->prev = mbcb->prev->next = mbcb;
		}
			
		DEBUG_OUTMSG("delete mailbox contorol block for interrput handler\n");
		
		return E_OK;
  }
}


/*
* システムコール処理(snd_mbx():メールボックスへの送信)
* *mbcb : 送信するメールボックスコントロールブロックへのポインタ
* *pk_msg : 送信するメッセージパケット先頭番地
* (返却値)E_OK : 正常終了(メッセージをキューイング,メッセージをタスクへ送信)
*/
ER snd_mbx_isr(MBXCB *mbcb, T_MSG *pk_msg)
{
	ER *ercd;
	TMR_OBJP *work_objp;

	/* 待ちタスクが存在する場合 */
	if (mbcb->waithead != NULL) {
		put_mbx_waittsk(mbcb); /* タスクをレディーへ戻す */
		
		/*
		* レディーへ戻したタスクがタイマブロックを持っている場合,
		* コールバックルーチンが呼び出されないように(呼ばれると不整合となる)
		* タイマブロックを排除しておく
		*/
		work_objp = &current->wait_info.tobjp;
		if (*work_objp) {
			delete_tmrcb_diffque((TMRCB *)*work_objp); /* タイマコントロールブロックを排除 */
			*work_objp = 0; /* クリアにしておく */
		}
		
		/* 待ちに入ったシステムコールの返却値をポインタを経由して書き換える */
		ercd = (ER *)current->syscall_info.ret;
		*ercd = E_OK;
		
		/* メッセージパケットへの先頭番地を受信タスクに渡す */
		*(current->syscall_info.param->un.rcv_mbx.pk_msg) = pk_msg;
	}
	/* 待ちタスクが存在しない場合(メッセージをキューイング) */
	else {
		if (mbcb->mktail) {
			pk_msg->prev = mbcb->mktail;
			mbcb->mktail->next = pk_msg;
    }
    else {
			mbcb->mkhead = pk_msg;
    }
    mbcb->mktail = pk_msg;
    
    DEBUG_OUTMSG("massage send to not wait task for interruput handler.\n");
	}
	
	return E_OK;

}


/*
* システムコール処理(rcv_mbx():メールボックスから受信)
* *mbcb : 受信するメールボックスコントロールブロックへのポインタ
* **pk_msg : 受信するメッセージパケットの先頭番地
* (返却値)E_OK : 正常終了(メッセージがなく待ちタスクとなった，メッセージをタスクに与えた)
*/
ER rcv_mbx_isr(MBXCB *mbcb, T_MSG **pk_msg)
{
	
	/* メッセージが存在しない場合(メッセージ待ちタスクへキューイング) */
	if (mbcb->mkhead == NULL) {
		if (mbcb->waittail) {
			current->wait_info.wait_prev = mbcb->waittail;
			mbcb->waittail->wait_info.wait_next = current;
    }
    else {
			mbcb->waithead = current;
    }
    mbcb->waittail = current;
    current->state |= TASK_WAIT_MAILBOX; /* 待ち情報をセット(TCBに) */
    current->wait_info.wobjp = (WAIT_OBJP)mbcb; /* 待ち情報をセット(TCBに) */
		
		DEBUG_OUTMSG("massage empty recv to wait task for interruput handler.\n");
	}
	/* メッセージが存在する場合 */
	else {
		put_mbx_retmsg(mbcb); /* メッセージをタスクへ返却 */
  	
  	putcurrent(); /* システムコール発行タスクをレディーへ */
  	DEBUG_OUTMSG("massage recv to task for interruput handler.\n");
	}

	return E_OK;
}


/*
* システムコール処理(prcv_mbx():ポーリング付きメールボックスからの受信)
* *mbcb : 受信するメールボックスコントロールブロックへのポインタ
* **pk_msg : 受信するメッセージパケットの先頭番地
* (返却値)E_OK : 正常終了(メッセージをタスクに与えた)
* (返却値)E_TMOUT : メッセージ待ち失敗(ポーリング)
*/
ER prcv_mbx_isr(MBXCB *mbcb, T_MSG **pk_msg)
{
	/* メッセージが存在しない場合(メッセージ待ちタスクへキューイング) */
	if (mbcb->mkhead == NULL) {
		return E_TMOUT;
	}
	/* メッセージが存在する場合 */
	else {
		put_mbx_retmsg(mbcb); /* メッセージをタスクへ返却 */
  	
  	DEBUG_OUTMSG("massage recv to task for interruput handler.\n");
		return E_OK;
	}
}


/*
* システムコール処理(trcv_mbx():タイムアウト付きメールボックスからの受信)
* *mbcb : 受信するメールボックスコントロールブロックへのポインタ
* **pk_msg : 受信するメッセージパケットの先頭番地
* tmout : タイムアウト値
* (返却値)E_OK : 正常終了(メッセージがなく待ちタスクとなった，メッセージをタスクに与えた)
* (返却値)E_TMOUT : メッセージ待ち失敗(ポーリング)
*/
ER trcv_mbx_isr(MBXCB *mbcb, T_MSG **pk_msg, int tmout)
{
	TMRCB *settbf;

	/* メッセージが存在する場合 */
	if (mbcb->mkhead != NULL) {
		put_mbx_retmsg(mbcb); /* メッセージをタスクへ返却 */
  	
		putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("massage recv to task for interruput handler.\n");
		return E_OK;
	}
	/* メッセージがない場合で，ポーリングが設定された */
	else if (tmout == TMO_POL) {
		putcurrent(); /* システムコール発行スレッドをレディーへ */
  	return E_TMOUT;
  }
	/* メッセージがなく，ポーリング設定以外の場合(タイムアウト付きでメッセージ待ちタスクへキューイング) */
	else {
		if (mbcb->waittail) {
			current->wait_info.wait_prev = mbcb->waittail;
			mbcb->waittail->wait_info.wait_next = current;
    }
    else {
			mbcb->waithead = current;
    }
    mbcb->waittail = current;
    current->state |= TASK_WAIT_MAILBOX; /* 待ち情報をセット(TCBに) */
    mbcb->tobjp = current->wait_info.wobjp = (WAIT_OBJP)mbcb; /* 待ち情報をセット(TCBとMBXCBに) */

		/* タイムアウト処理 */
    settbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, tmout, 0, twai_mbx_callrte, current); /* 差分のキューのノードを作成 */
		current->wait_info.tobjp = (TMR_OBJP)settbf; /* TCBのwait_infoとTMRCBの接続 */
		DEBUG_OUTMSG("massage empty recv to wait task for interruput handler.\n");

		return E_OK;
	}

}
