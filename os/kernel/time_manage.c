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


#include "c_lib/lib.h"
#include "memory.h"
#include "time_manage.h"
#include "target/driver/timer_driver.h"
#include "scheduler.h"
#include "ready.h"


/*! 周期ハンドラの初期化(静的型cycle handlerの領域確保と初期化(可変長配列として使用する)) */
static ER static_cyc_init(int cycids);

/*! 周期ハンドラの初期化(動的型cycle handlerの領域確保と初期化(freeリストを作成する)) */
static ER dynamic_cyc_init(void);

/*! 動的cycle alocリストから周期ハンドラコントロールブロックを抜き取る */
static void get_cyc_aloclist(CYCCB *cycb);

/*! アラームハンドラの初期化(静的型alarm handlerの領域確保と初期化(可変長配列として使用する)) */
static ER static_alm_init(int almids);

/*! アラームハンドラの初期化(動的型alarm handlerの領域確保と初期化(freeリストを作成する)) */
static ER dynamic_alm_init(void);

/*! 動的alarm alocリストからアラームハンドラコントロールブロックを抜き取る */
static void get_alm_aloclist(ALMCB *acb);


/*!
* 周期ハンドラの初期化(cycle handler ID変換テーブルの領域確保と初期化)
* start()_init_tsk()で呼ばれる場合と，cycleIDが足らなくなった場合に呼ばれる
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER cyc_init(void)
{
	int cycids, i;
	CYCCB *cycb;
	
	cycids = CYCLE_ID_NUM << mg_cyc_info.power_count; /* 倍の領域を確保する(start_init_tsk()の時はデフォルト通り) */
	mg_cyc_info.id_table = (CYCCB **)get_mpf_isr(sizeof(cycb) * cycids); /* 変換テーブルの動的メモリ確保 */
	/* alarmID変換テーブルの初期化(メモリにNULLを埋めるのにmemset()は使用できない) */
	for (i = 0; i < cycids; i++) {
		mg_cyc_info.id_table[i] = NULL;
	}
	/* CYCCBの静的型配列の作成及び初期化とCYCCBの動的型freeリストの作成と初期化 */
	if ((E_NOMEM == static_cyc_init(cycids)) || (E_NOMEM == dynamic_cyc_init())) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	
	return E_OK;
}


/*!
* 周期ハンドラの初期化(静的型cycle handlerの領域確保と初期化(可変長配列として使用する))
* cycids : 確保する可変長配列の個数
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER static_cyc_init(int cycids)
{
	CYCCB *cycb;
	int size = sizeof(*cycb) * cycids; /* 可変長配列のサイズ */
	
	mg_cyc_info.array = (CYCCB *)get_mpf_isr(size); /* 可変長配列確保 */
	if (mg_cyc_info.array == NULL) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	/* 確保した可変長配列を初期化(可変長でも配列はメモリに連続しているのでこれでOK) */
	memset(mg_cyc_info.array, -1, size);
	
	return E_OK;
}

/*!
* 周期ハンドラの初期化(動的型cycle handlerの領域確保と初期化(freeリストを作成する))
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER dynamic_cyc_init(void)
{
	int cycids, i;
	CYCCB *cycb;
	
	mg_cyc_info.freehead = mg_cyc_info.alochead = NULL;
	
	/* start_init_tsk()の時以外 */
	if (mg_cyc_info.power_count) {
		cycids = CYCLE_ID_NUM << (mg_cyc_info.power_count - 1); /* 現在と同じ領域を確保する */
	}
	/* start_init_tsk()の時 */
	else {
		cycids = CYCLE_ID_NUM << mg_cyc_info.power_count;
	}
	
	for (i = 0; i < cycids; i++) {
		cycb = (CYCCB *)get_mpf_isr(sizeof(*cycb)); /* ノードのメモリ確保 */
		/*メモリが取得できない*/
  	if(cycb == NULL) {
  		return E_NOMEM;	 /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
  	}
		memset(cycb, -1, sizeof(*cycb)); /* 確保したノードを初期化 */
		/* freeキューの作成 */
		cycb->next = mg_cyc_info.freehead;
		cycb->prev = NULL;
		mg_cyc_info.freehead = cycb->next->prev = cycb;
	}
	
	return E_OK;
}


/*!
* 動的cycle alocリストから周期ハンドラコントロールブロックを抜き取る
* cycb : 対象のCYCCB
*/
static void get_cyc_aloclist(CYCCB *cycb)
{
	/* 先頭から抜きとる */
	if (cycb == mg_cyc_info.alochead) {
		mg_cyc_info.alochead = cycb->next;
		mg_cyc_info.alochead->prev = NULL;
	}
	/* それ以外から抜き取る */
	else {
		cycb->prev->next = cycb->next;
		cycb->next->prev = cycb->prev;
	}
}


/*!
* システムコール処理(acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* *exinf : アラームハンドラに渡すパラメータ
* cyctim : 起動周期(msec)
* func : 周期ハンドラ
* ID自動割付なので，IDチェックはない.
* 属性はないので，E_RSATRエラーはない.
* ID自動割付なので，ID重複のE_OBJエラーはない.
* cyctimは0以下でエラーとなるが実際はもっと多く見積もらないと動作できない
* エラーコードとオブジェクトのポインタを返却するためER_OBJP型としている
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)cycb : 正常終了(作成したアラームハンドラコントロールブロックへのポインタ)
*/
OBJP acre_cyc_isr(CYC_TYPE type, void *exinf, int cyctim, TMR_CALLRTE func)
{
  CYCCB *cycb;
  
  /* 静的型cycle handlerの場合 */
  if (type == STATIC_CYCLE_HANDLER) {
  	cycb = &mg_cyc_info.array[mg_cyc_info.counter];
  }
  /* 動的型cycle handlerの場合 */
  else {
  	cycb = mg_cyc_info.freehead; /* free headからノードを与える(抜き取るわけではない) */
  	mg_cyc_info.freehead = cycb->next; /* free headを一つ進める */
  	/* aloc headが空の場合 */
  	if (mg_cyc_info.alochead == NULL) {
  		mg_cyc_info.alochead = cycb;
  	}
  }
  cycb->cycid = mg_cyc_info.counter;
	cycb->cycatr = type;
  cycb->exinf = exinf;
  cycb->cyctim = cyctim;
 	cycb->func = func;
 	cycb->tobjp = 0;
 
 	DEBUG_OUTMSG("create cycle handler contorol block for interrput handler\n");
 
 	return (OBJP)cycb;
}


/*!
* システムコール処理(del_cyc():周期ハンドラコントロールブロックの排除)
* del_cyc()はアラームハンドラが動作開始していた場合，設定を解除して排除する
* *cycb : 排除する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(排除完了)
*/
ER del_cyc_isr(CYCCB *cycb)
{
	TMRCB *tbf;
	
	/* 解除または排除処理(不整合を少しでも抑えるため，タイマを解除してから排除処理を行う) */
	if (cycb->tobjp) {
		tbf = (TMRCB *)cycb->tobjp;
		delete_tmrcb_diffque(tbf); /* 差分のキューからタイマコントロールブロックの排除 */
	}
	memset(cycb, -1, sizeof(*cycb)); /* ノードを初期化 */
	/* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
	if (cycb->cycatr &= DYNAMIC_CYCLE_HANDLER) {
		get_cyc_aloclist(cycb); /* alocリストから抜き取り */
		/* freeリスト先頭へ挿入 */
		cycb->next = mg_cyc_info.freehead;
		cycb->prev = mg_cyc_info.freehead->prev;
		mg_cyc_info.freehead = cycb->next->prev = cycb->prev->next = cycb;
	}
		
	DEBUG_OUTMSG("delete cycle handler contorol block for interrput handler\n");
	
	return E_OK;
}


/*!
* システムコール処理(sta_cyc():周期ハンドラの動作開始)
* *cycb : 動作開始する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(起動完了)
*/
ER sta_cyc_isr(CYCCB *cycb)
{
	TMRCB *tbf;
	
	/* すでに動作している周期ハンドラの場合は一度排除して新しい起動時間の再設定 */
	if (cycb->tobjp) {
		tbf = (TMRCB *)cycb->tobjp;
		delete_tmrcb_diffque(tbf); /* 差分のキューからタイマコントロールブロックの排除 */
		tbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, cycb->cyctim, (TMRRQ_OBJP)cycb, cycb->func, cycb->exinf); /* 差分のキューのノードを作成 */
		cycb->tobjp = (TMR_OBJP)tbf;
		return E_OK;
	}
	/* まだ動作していない場合 */
	else {
		tbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, cycb->cyctim, (TMRRQ_OBJP)cycb, cycb->func, cycb->exinf); /* 差分のキューのノードを作成 */
		cycb->tobjp = (TMR_OBJP)tbf; /* 周期ハンドラコントロールブロックとタイマコントロールブロックの接続 */
		return E_OK;
	}
}


/*!
* システムコール処理(stp_cyc():周期ハンドラの動作停止)
* 周期ハンドラが動作開始していたならば，設定を解除する(排除はしない)
* タイマが動作していない場合は何もしない
* *cycb : 動作停止する周期ハンドラコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER stp_cyc_isr(CYCCB *cycb)
{
	TMRCB *tbf;
	
	/*
	* 以下のif文で条件付けしなくてもdelete_tmrcb_diffque()でNULLチェックを行っているので
	* 実際いらない.(でもわかりやすくするために追加(冗長的かな～))
	*/
	if (cycb->tobjp) {
		tbf = (TMRCB *)cycb->tobjp;
		delete_tmrcb_diffque(tbf); /* 差分のキューからタイマコントロールブロックの排除 */
		cycb->tobjp = 0; /* タイマコントロールブロックと接続を切り離す */
		DEBUG_OUTMSG("stop timer for interrput handler.\n");
	}
	else {
		/* 何もしない */
	}
	return E_OK;
}


/*!
* アラームハンドラの初期化(alarm handler ID変換テーブルの領域確保と初期化)
* start()_init_tsk()で呼ばれる場合と，alarmIDが足らなくなった場合に呼ばれる
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER alm_init(void)
{
	int almids, i;
	ALMCB *acb;
	
	almids = ALARM_ID_NUM << mg_alm_info.power_count; /* 倍の領域を確保する(start_init_tsk()の時はデフォルト通り) */
	mg_alm_info.id_table = (ALMCB **)get_mpf_isr(sizeof(acb) * almids); /* 変換テーブルの動的メモリ確保 */
	/* alarmID変換テーブルの初期化(メモリにNULLを埋めるのにmemset()は使用できない) */
	for (i = 0; i < almids; i++) {
		mg_alm_info.id_table[i] = NULL;
	}
	/* ALMCBの静的型配列の作成及び初期化とALMCBの動的型freeリストの作成と初期化 */
	if ((E_NOMEM == static_alm_init(almids)) || (E_NOMEM == dynamic_alm_init())) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	
	return E_OK;
}


/*!
* アラームハンドラの初期化(静的型alarm handlerの領域確保と初期化(可変長配列として使用する))
* almids : 確保する可変長配列の個数
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER static_alm_init(int almids)
{
	ALMCB *acb;
	int size = sizeof(*acb) * almids; /* 可変長配列のサイズ */
	
	mg_alm_info.array = (ALMCB *)get_mpf_isr(size); /* 可変長配列確保 */
	if (mg_alm_info.array == NULL) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	/* 確保した可変長配列を初期化(可変長でも配列はメモリに連続しているのでこれでOK) */
	memset(mg_alm_info.array, -1, size);
	
	return E_OK;
}

/*!
* アラームハンドラの初期化(動的型alarm handlerの領域確保と初期化(freeリストを作成する))
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER dynamic_alm_init(void)
{
	int almids, i;
	ALMCB *acb;
	
	mg_alm_info.freehead = mg_alm_info.alochead = NULL;
	
	/* start_init_tsk()の時以外 */
	if (mg_alm_info.power_count) {
		almids = ALARM_ID_NUM << (mg_alm_info.power_count - 1); /* 現在と同じ領域を確保する */
	}
	/* start_init_tsk()の時 */
	else {
		almids = ALARM_ID_NUM << mg_alm_info.power_count;
	}
	
	for (i = 0; i < almids; i++) {
		acb = (ALMCB *)get_mpf_isr(sizeof(*acb)); /* ノードのメモリ確保 */
		/*メモリが取得できない*/
  	if(acb == NULL) {
  		return E_NOMEM;	 /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
  	}
		memset(acb, -1, sizeof(*acb)); /* 確保したノードを初期化 */
		/* freeキューの作成 */
		acb->next = mg_alm_info.freehead;
		acb->prev = NULL;
		mg_alm_info.freehead = acb->next->prev = acb;
	}
	
	return E_OK;
}


/*!
* 動的alarm alocリストからアラームハンドラコントロールブロックを抜き取る
* acb : 対象のALMCB
*/
static void get_alm_aloclist(ALMCB *acb)
{
	/* 先頭から抜きとる */
	if (acb == mg_alm_info.alochead) {
		mg_alm_info.alochead = acb->next;
		mg_alm_info.alochead->prev = NULL;
	}
	/* それ以外から抜き取る */
	else {
		acb->prev->next = acb->next;
		acb->next->prev = acb->prev;
	}
}


/*!
* システムコール処理(acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付))
* type : 静的型か動的型か？
* *exinf : アラームハンドラに渡すパラメータ
* func : アラームハンドラ
* ID自動割付なので，IDチェックはない.
* 属性はないので，E_RSATRエラーはない.
* ID自動割付なので，ID重複のE_OBJエラーはない.
* エラーコードとオブジェクトのポインタを返却するためER_OBJP型としている
* (返却値)E_PAR : システムコールの引数不正
* (返却値)E_NOID : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)acb : 正常終了(作成したアラームハンドラコントロールブロックへのポインタ)
*/
OBJP acre_alm_isr(ALM_TYPE type, void *exinf, TMR_CALLRTE func)
{
  ALMCB *acb;
  
  /* 静的型alarm handlerの場合 */
  if (type == STATIC_ALARM_HANDLER) {
  	acb = &mg_alm_info.array[mg_alm_info.counter];
  }
  /* 動的型alarm handlerの場合 */
  else {
  	acb = mg_alm_info.freehead; /* free headからノードを与える(抜き取るわけではない) */
  	mg_alm_info.freehead = acb->next; /* free headを一つ進める */
  	/* aloc headが空の場合 */
  	if (mg_alm_info.alochead == NULL) {
  		mg_alm_info.alochead = acb;
  	}
  }
  acb->almid = mg_alm_info.counter;
	acb->almatr = type;
  acb->exinf = exinf;
  acb->func = func;
  acb->tobjp = 0;
  
 	DEBUG_OUTMSG("create alarm handler contorol block for interrput handler\n");
  
  return (OBJP)acb;
}


/*!
* システムコール処理(del_alm():アラームハンドラコントロールブロックの排除)
* del_alm()はアラームハンドラが動作開始していた場合，設定を解除して排除する
* *acb : 排除するアラームハンドラコントロールブロックへのポインタ
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(排除完了)
*/
ER del_alm_isr(ALMCB *acb)
{
	TMRCB *tbf;
	
	/* アラームハンドラ未登録 */
	if (!acb->func) {
		return EV_NORTE;
	}
	/* ここからシステムコール処理 */
	else {
		/* 解除または排除処理(不整合を少しでも抑えるため，タイマを解除してから排除処理を行う) */
		if (acb->tobjp) {
			tbf = (TMRCB *)acb->tobjp;
			delete_tmrcb_diffque(tbf); /* 差分のキューからタイマコントロールブロックの排除 */
		}
		memset(acb, -1, sizeof(*acb)); /* ノードを初期化 */
		/* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
		if (acb->almatr &= DYNAMIC_ALARM_HANDLER) {
			get_alm_aloclist(acb); /* alocリストから抜き取り */
			/* freeリスト先頭へ挿入 */
			acb->next = mg_alm_info.freehead;
			acb->prev = mg_alm_info.freehead->prev;
			mg_alm_info.freehead = acb->next->prev = acb->prev->next = acb;
		}
			
		DEBUG_OUTMSG("delete alarm handler contorol block for interrput handler\n");
		
		return E_OK;
	}
}


/*!
* システムコール処理(sta_dalm():アラームハンドラの動作開始)
* *acb : 動作開始するアラームハンドラコントロールブロックへのポインタ
* msec : 動作開始時間
* (返却値)E_PAR : パラメータエラー(タイマ有効値の1より小さい)
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(起動完了)
*/
ER sta_alm_isr(ALMCB *acb, int msec)
{
	TMRCB *tbf;
	
	/* アラームハンドラ未登録 */
	if (!acb->func) {
		return EV_NORTE;
	}
	/* パラメータは正しいか */
	else if (msec < TMR_EFFECT) {
		return E_PAR;
	}
	/* すでに動作しているアラームハンドラの場合は一度排除して新しい起動時間の再設定 */
	else if (acb->tobjp) {
		tbf = (TMRCB *)acb->tobjp;
		delete_tmrcb_diffque(tbf); /* 差分のキューからタイマコントロールブロックの排除 */
		tbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, msec, 0, acb->func, acb->exinf); /* 差分のキューのノードを作成 */
		acb->tobjp = (TMR_OBJP)tbf;
		return E_OK;
	}
	/* まだ動作していない場合 */
	else {
		tbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, msec, 0, acb->func, acb->exinf); /* 差分のキューのノードを作成 */
		acb->tobjp = (TMR_OBJP)tbf; /* 動的アラームハンドラコントロールブロックとタイマコントロールブロックの接続 */
		return E_OK;
	}
}


/*!
* システムコール処理(stp_dalm():動的アラームの動作停止)
* アラームハンドラが動作開始していたならば，設定を解除する(排除はしない)
* タイマが動作していない場合は何もしない
* *acb : 動作停止するアラームハンドラコントロールブロックへのポインタ
* (返却値)EV_NORTE : アラームハンドラ未登録
* (返却値)E_OK : 正常終了(停止完了，タイマは動作していない)
*/
ER stp_alm_isr(ALMCB *acb)
{
	TMRCB *tbf;
	
	/* アラームハンドラ未登録 */
	if (!acb->func) {
		return EV_NORTE;
	}
	/* 解除処理 */
	else {
		/*
		* 以下のif文で条件付けしなくてもdelete_tmrcb_diffque()でNULLチェックを行っているので
		* 実際いらない.(でもわかりやすくするために追加(冗長的かな～))
		*/
		if (acb->tobjp) {
			tbf = (TMRCB *)acb->tobjp;
			delete_tmrcb_diffque(tbf); /* 差分のキューからタイマコントロールブロックの排除 */
			acb->tobjp = 0; /* タイマコントロールブロックと接続を切り離す */
			DEBUG_OUTMSG("stop timer for interrput handler.\n");
		}
		else {
			/* 何もしない */
		}
		return E_OK;
	}
}
