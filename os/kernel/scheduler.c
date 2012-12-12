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


#include "arch/cpu/intr.h"
#include "scheduler.h"
#include "kernel.h"
#include "memory.h"
#include "task_manage.h"
#include "timer_callrte.h"
#include "target/driver/timer_driver.h"
#include "ready.h"


static ER switch_schduler(SCHDUL_TYPE type);

/*! スケジューラ情報の読み取り設定 */
static void read_schdul(SCHDULCB *schcb);

/*! First Come First Sarvedスケジューラ */
static void schedule_fcfs(void);

/*! ラウンドロビンスケジューラ */
static void schedule_rr(void);

/*! 優先度ビットップを検索する */
static ER_VLE bit_serch(UINT32 bitmap);

/*! 優先度スケジューラ */
static void schedule_ps(void);

/*! 優先度×ラウンドロビンスケジューラ */
static void schedule_rps(void);

/*! Multilevel Feedback Queue */
static void schedule_mfq(void);

/*! 実行キューと満了キューを交換(簡易O(1)スケジューラで使用する関数) */
static void change_tmprique(void);

/*! 簡易O(1)スケジューラ */
static void schedule_odrone(void);

/*! 簡易O(1)スケジューラの設定 */
static void active_odrone_schduler(int prio);

/*! Fairスケジューリング時使用(タイマ割込みが発生しなかった場合のタスク実行時間の取得) */
static int get_frs_exection_timer(TMRCB *tbf);

/* Fair Scheduler */
static void schedule_fr(void);

/* Priority Fair Scheduler */
static void schedule_pfr(void);

/*! RMスケジューラミスハンドラ(条件よりスケジュールできない) */
static void rmschedule_miss_handler(void);

/*! Rate Monotonic */
static void schedule_rms(void);

/*! DMスケジューラミスハンドラ(条件よりスケジュールできない) */
static void dmschedule_miss_handler(void);

/*! Deadline Monotonic */
static void schedule_dms(void);

/*! Earliest Deadline First */
static void schedule_edf(void);

/*! Least Laxity First */
static void schedule_llf(void);


/*!
* システムコール処理(sel_schdul():スケジューラの切り替え)
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
ER sel_schdul_isr(SCHDUL_TYPE type, long param)
{
	ER ercd;

	/* initタスク生成ルーチンに戻すため，スケジューラ情報メモリセグメントへ書き込み */
	ercd = write_schdul(type, param); /* スケジューラ情報メモリセグメントへ書き込み */
	/* スケジューラを切り替える() */
	if (ercd == E_OK) {
		/*
		* スケジューラを切り替える(rol_sys()が発行されない場合は，後のシステムコールから
		* スケジューラを切り替える(sel_schdulからは切り替えない)ので，その準備をする
		*/
		//ercd = switch_schdulr(type);
	}
	return ercd;
}


/*!
* スケジューラ情報メモリセグメントへ書き込み
* type : スケジューラのタイプ
* param : スケジューラが使用する情報
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 正常終了
*/
ER write_schdul(SCHDUL_TYPE type, long param)
{
	extern char schdul_area;
	char *schdul_info = &schdul_area;
	UINT32 *p = (UINT32 *)schdul_info;

	*(--p) = type;	/* スケジューラのタイプを退避 */
	
	/* FCFSスケジューリング */
	if (type == FCFS_SCHEDULING) {
		*(--p) = -1;
		*(--p) = (UINT32)schedule_fcfs; /* スケジューラコントロールブロックの設定 */
	}
	
	/* ラウンドロビンスケジューリング */
	else if (type == RR_SCHEDULING) {
		/* パラメータチェック */
		if (param < TMR_EFFECT) {
			return E_PAR;
		}
		/* スケジューラコントロールブロックの設定 */
		*(--p) = param;
		*(--p) = (UINT32)schedule_rr;
	}
	
	/* 優先度スケジューリング */
	else if (type == PRI_SCHEDULING) {
		*(--p) = -1;
		*(--p) = (UINT32)schedule_ps; /* スケジューラコントロールブロックの設定 */
	}
	
	/* ラウンドロビン×優先度スケジューリング */
	else if (type == RR_PRI_SCHEDULING) {
		/* パラメータチェック */
		if (param < TMR_EFFECT) {
			return E_PAR;
		}
		*(--p) = param;
		*(--p) = (UINT32)schedule_rps;
	}
	
	/* Multilevel Feedback Queue */
	else if (type == MFQ_SCHEDULING) {
		/* パラメータチェック */
		if (param < TMR_EFFECT) {
			return E_PAR;
		}
		/* スケジューラコントロールブロックの設定 */
		*(--p) = param;
		*(--p) = (UINT32)schedule_mfq;
	}
	
	/* 簡易O(1)スケジューリング */
	else if (type == ODRONE_SCHEDULING) {
		/* パラメータチェック */
		if (param < TMR_EFFECT) {
			return E_PAR;
		}
		*(--p) = param;
		*(--p) = (UINT32)schedule_odrone;
	}
	
	/* Fair Scheduler */
	else if (type == FR_SCHEDULING) {
		/* パラメータチェック */
		if (param < TMR_EFFECT) {
			return E_PAR;
		}
		*(--p) = param;
		*(--p) = (UINT32)schedule_fr;
	}
	
	/* Priority Fair Scheduler */
	else if (type == PFR_SCHEDULING) {
		/* パラメータチェック */
		if (param < TMR_EFFECT) {
			return E_PAR;
		}
		*(--p) = param;
		*(--p) = (UINT32)schedule_pfr;
	}
	
	/* Rate Monotonic */
	else if (type == RM_SCHEDULING) {
		*(--p) = -1;
		*(--p) = (UINT32)schedule_rms; /* スケジューラコントロールブロックの設定 */
	}
	
	/* Deadline Monotonic */
	else if (type == DM_SCHEDULING) {
		*(--p) = -1;
		*(--p) = (UINT32)schedule_dms; /* スケジューラコントロールブロックの設定 */
	}

	/* Earliest Deadline First */
	else if (type == EDF_SCHEDULING) {
		*(--p) = -1;
		*(--p) = (UINT32)schedule_edf; /* スケジューラコントロールブロックの設定 */
	}
	
	/* Least Laxity First */
	else if (type == LLF_SCHEDULING) {
		*(--p) = -1;
		*(--p) = (UINT32)schedule_llf; /* スケジューラコントロールブロックの設定 */
	}
	
	return E_OK;
}


static ER switch_schduler(SCHDUL_TYPE type)
{
	puts("coming soon.\n");

	/*SCHDULCB *next_schcb;

	next_schcb = (SCHDULCB *)get_mpf_isr(sizeof(*next_schcb));
	if (next_schcb == NULL) {
		return E_NOMEM;
	}
	memset(next_schcb, 0, sizeof(*next_schcb));

	if (type == RM_SCHEDULING)	 {
		if (is_switch_rm_schdulr())	{
			remove_ready(next_schcb);
		}
		else {
			return E_NOSWT;
		}
	}
	else if (type == DM_SCHEDULING)	 {
		if (is_switch_dm_schdulr())	{
			remove_ready(next_schcb);
		}
		else {
			return E_NOSWT;
		}
	}
	else if (type == EDF_SCHEDULING)	 {
		if (is_switch_edf_schdulr())	{
			remove_ready(next_schcb);
		}
		else {
			return E_NOSWT;
		}
	}
	else if (type == LLF_SCHEDULING)	 {
		if (is_switch_llf_schdulr())	{
			remove_ready(next_schcb);
		}
		else {
			return E_NOSWT;
		}
	}
	else {
		remove_ready(next_schcb);
	} */
	return E_NOSPT;
}


/*!
* スケジューラ情報を読み取り設定
* *schcb : 設定するスケジューラコントロールブロック
*/
static void read_schdul(SCHDULCB *schcb)
{
	SCHDUL_TYPE type;
	extern char schdul_area;
	char *schdul_info = &schdul_area;
	UINT32 *p = (UINT32 *)schdul_info;
	long dummy;
	
	mg_schdul_info.type = type = *(--p);	/* スケジューラタイプを復旧 */
	
	/* FCFSスケジューリング */
	if (type == FCFS_SCHEDULING) {
		dummy = *(--p);
		schcb->un.fcfs_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* ラウンドロビンスケジューリング */
	else if (type == RR_SCHEDULING) {
		/* スケジューラコントロールブロックの設定 */
		schcb->un.rr_schdul.tmout = (int)*(--p);
		schcb->un.rr_schdul.tobjp = 0;
		schcb->un.rr_schdul.rte = (void *)(*(--p));
	}
	
	/* 優先度スケジューリング */
	else if (type == PRI_SCHEDULING) {
		dummy = *(--p);
		schcb->un.ps_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* ラウンドロビン×優先度スケジューリング */
	else if (type == RR_PRI_SCHEDULING) {
		/* スケジューラコントロールブロックの設定 */
		schcb->un.rps_schdul.tmout = (int)*(--p);
		schcb->un.rps_schdul.tobjp = 0;
		schcb->un.rps_schdul.rte = (void *)(*(--p));
	}
	
	/* Multilevel Feedback Queue */
	else if (type == MFQ_SCHEDULING) {
		/* スケジューラコントロールブロックの設定 */
		schcb->un.mfq_schdul.tmout = (int)*(--p);
		schcb->un.mfq_schdul.tobjp = 0;
		schcb->un.mfq_schdul.rte = (void *)(*(--p));
	}
	
	/* 簡易O(1)スケジューリング */
	else if (type == ODRONE_SCHEDULING) {
		/* スケジューラコントロールブロックの設定 */
		schcb->un.odrone_schdul.tmout = (int)*(--p);
		schcb->un.odrone_schdul.tobjp = 0;
		schcb->un.odrone_schdul.rte = (void *)(*(--p));
	}
	
	/* Fair Scheduler */
	else if (type == FR_SCHEDULING) {
		/* スケジューラコントロールブロックの設定 */
		schcb->un.fr_schdul.tmout = *(--p);
		schcb->un.fr_schdul.tobjp = 0;
		schcb->un.fr_schdul.rte = (void *)(*(--p));
	}
	
	/* Priority Fair Scheduler */
	else if (type == PFR_SCHEDULING) {
		/* スケジューラコントロールブロックの設定 */
		schcb->un.pfr_schdul.tmout = (int)*(--p);
		schcb->un.pfr_schdul.tobjp = 0;
		schcb->un.pfr_schdul.rte = (void *)(*(--p));
	}
	
	/* Rate Monotonic */
	else if (type == RM_SCHEDULING) {
		dummy = *(--p);
		schcb->un.rms_schdul.unroll_rate = 0;
		schcb->un.rms_schdul.unroll_exetim = 0;
		schcb->un.rms_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* Deadline Monotonic */
	else if (type == DM_SCHEDULING) {
		dummy = *(--p);
		schcb->un.dms_schdul.unroll_dead = 0;
		schcb->un.dms_schdul.unroll_exetim = 0;
		schcb->un.dms_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}

	/* Earliest Deadline First */
	else if (type == EDF_SCHEDULING) {
		dummy = *(--p);
		schcb->un.edf_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	/* Least Laxity First */
	else if (type == LLF_SCHEDULING) {
		dummy = *(--p);
		schcb->un.llf_schdul.rte = (void *)(*(--p)); /* スケジューラコントロールブロックの設定 */
	}
	
	mg_schdul_info.entry = schcb;
}


/*!
* スケジューラの初期化
* typeはenumでやっているので，パラメータチェックはいらない
* type : スケジューラのタイプ
* *exinf : スケジューラに渡すパラメータ
* (返却値)E_NOMEM : メモリ不足
* (返却値)E_OK : 正常終了
*/
ER schdul_init(void)
{
	SCHDULCB *schcb;
	
	schcb = (SCHDULCB *)get_mpf_isr(sizeof(*schcb)); /* 動的メモリ取得 */
	if (schcb == NULL) {
		return E_NOMEM;
	}
	memset(schcb, 0, sizeof(*schcb));
	
	read_schdul(schcb); /* スケジューラ情報の読み込み */

	return E_OK;
}


/*!
* 有効化されているスケジューラは分岐する
* typeはenumでやっているので，パラメータチェックはいらない
*/
void schedule(void)
{
	/* 登録しておいたスケジューラを関数ポインタで呼ぶ */
	/* FCFSスケジューリング */
	if (mg_schdul_info.type == FCFS_SCHEDULING) {
		(*mg_schdul_info.entry->un.fcfs_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* ラウンドロビンスケジューリング */
	else if (mg_schdul_info.type == RR_SCHEDULING) {
		(*mg_schdul_info.entry->un.rr_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* 優先度スケジューリング */
	else if (mg_schdul_info.type == PRI_SCHEDULING) {
		(*mg_schdul_info.entry->un.ps_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* 優先度×ラウンドロビンスケジューリング */
	else if (mg_schdul_info.type == RR_PRI_SCHEDULING) {
		(*mg_schdul_info.entry->un.rps_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Multilevel Feedback Queue */
	else if (mg_schdul_info.type == MFQ_SCHEDULING) {
		(*mg_schdul_info.entry->un.mfq_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* 簡易O(1)スケジューリング */
	else if (mg_schdul_info.type == ODRONE_SCHEDULING) {
		(*mg_schdul_info.entry->un.odrone_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Fair Scheduler */
	else if (mg_schdul_info.type == FR_SCHEDULING) {
		(*mg_schdul_info.entry->un.fr_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Priority Fair Scheduler */
	else if (mg_schdul_info.type == PFR_SCHEDULING) {
		(*mg_schdul_info.entry->un.pfr_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Rate Monotonic */
	else if (mg_schdul_info.type == RM_SCHEDULING) {
		(*mg_schdul_info.entry->un.rms_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Deadline Monotonic */
	else if (mg_schdul_info.type == DM_SCHEDULING) {
		(*mg_schdul_info.entry->un.dms_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Earliest Deadline First */
	else if (mg_schdul_info.type == EDF_SCHEDULING) {
		(*mg_schdul_info.entry->un.edf_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
	/* Least Laxity First */
	else if (mg_schdul_info.type == LLF_SCHEDULING) {
		(*mg_schdul_info.entry->un.llf_schdul.rte)(); /* 関数ポインタ呼び出し */
	}
}


/*!
* First Come First Sarvedスケジューラ
* 優先度はなく，到着順にスケジューリングする
* p : FCFCスケジューラ使用時のレディーキューのポインタ
*/
static void schedule_fcfs(void)
{
	RQUECB *p = &mg_ready_info.entry->un.single.ready;
	TCB **p_ique = &mg_ready_info.init_que;

	/* 単一のレディーキューの先頭は存在するか */
	if (!p->head) {
		/* initタスクは存在する場合 */
		if (*p_ique != NULL) {
			current = *p_ique;
			return;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* 単一のレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
		current = p->head;
		return;
	}
}


/*!
* タイムスライス型スケジューリング環境下で割込みが発生しタイマブロックを排除するか検査する関数
* type : 割込みの種類
* (kernel.cのthread_intr()から呼ばれる)
* 割込みがかかると，タスクが切り替わるのでスケジューラの
* タイムスライス情報をリセット
* スケジューラが起こすタイマ割込み以外は抜かす．抜かさないとタイムスライススケジューリングの
* コールバックルーチンが実行されない．
* スケジューラ以外のタイマ割込み時は，スケジューラのタイマブロックを排除する
* syscall.cと下のif分の間にタイマが満了し，タイマ割込みが起こるとタイミング依存なバグになるかも
*/
void check_tmslice_schedul(SOFTVEC type)
{
	TMRCB *tbf;
	SCHDUL_TYPE schdul_type = mg_schdul_info.type;
	TMR_OBJP *p_tobjp;

	/* if文条件内で代入比較はサポートさせていない */

	/* ラウンドロビンスケジューリングかつ，タイマブロックが作成されているか */
	if (schdul_type == RR_SCHEDULING) {
		p_tobjp = &mg_schdul_info.entry->un.rr_schdul.tobjp;
		if (*p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
		else {
			/* 何もしない */
			return;
		}
	}
	/* ラウンドロビン×優先度スケジューリングかつ，タイマブロックが作成されているか */
	else if (schdul_type == RR_PRI_SCHEDULING) {
		p_tobjp = &mg_schdul_info.entry->un.rps_schdul.tobjp;
		if (*p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
		else {
			/* 何もしない */
			return;
		}
	}
	/* Multilevel Feedback Queueかつ，タイマブロックが作成されているか */
	else if (schdul_type == MFQ_SCHEDULING) {
		p_tobjp = &mg_schdul_info.entry->un.mfq_schdul.tobjp;
		if (*p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
		else {
			/* 何もしない */
			return;
		}
	}
	/* 簡易O(1)スケジューリングかつ，タイマブロックが作成されているか */
	else if (schdul_type == ODRONE_SCHEDULING) {
		p_tobjp = &mg_schdul_info.entry->un.odrone_schdul.tobjp;
		if (*p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
		else {
			/* 何もしない */
			return;
		}
	}
	/* Fairスケジューリングかつ，タイマブロックが作成されているか */
	else if (schdul_type == FR_SCHEDULING) {
		p_tobjp = &mg_schdul_info.entry->un.fr_schdul.tobjp;
		if (*p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			/* 実行時間の加算(CPUバウンドかI/Oバウンドの公平性実現) */
			current->schdul_info->un.fr_schdul.rel_exetim += get_frs_exection_timer(tbf);
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
		else {
			/* 何もしない */
			return;
		}
	}
	/* Priority Fairスケジューリングかつ，タイマブロックが作成されているか */
	else if (schdul_type == PFR_SCHEDULING) {
		p_tobjp = &mg_schdul_info.entry->un.pfr_schdul.tobjp;
		if (*p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			/* 実行時間の加算(CPUバウンドかI/Oバウンドの公平性実現) */
			current->schdul_info->un.fr_schdul.rel_exetim += get_frs_exection_timer(tbf);
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
		else {
			/* 何もしない */
			return;
		}
	}
	/* それ以外のスケジューリング */
	else {
		/* 何もしない */
		return;
	}

	/* タイマ割込みではない場合 */
	if (type != SOFTVEC_TYPE_TIMINTR) {
		/* タイマブロックを排除 */
		delete_tmrcb_diffque(tbf);
	}
	/*
	* タイマ割込みの場合であり，かつソフトタイマキュー先頭がスケジューラのタイマブロックでない場合
	* タイマ割込みが起こった時のタイマブロックは必ずソフトタイマキューの先頭である
	*/
	else if (mg_timerque.tmrhead->flag == OTHER_MAKE_TIMER) {
		/* タイマブロックを排除 */
		delete_tmrcb_diffque(tbf);
	}
	/* それ以外(スケジューラのタイマ割込みの場合) */
	else {
		/* 何もしない */
		return;
	}
}


/*!
* ラウンドロビンスケジューラ
* 優先度はなく，タイムスライスによってタスクを切っていく
* すべてのタスクが公平に(CPUバウンドかI/Oバウンドを無視した場合)スケジューリングされる方式である
* p : RRスケジューラ使用時のレディーキューのポインタ
*/
static void schedule_rr(void)
{
	RQUECB *p = &mg_ready_info.entry->un.single.ready;
	TMRCB *settbf;
	TCB **p_ique = &mg_ready_info.init_que;
	int *p_tm = &current->schdul_info->un.slice_schdul.tm_slice;

	/* 単一のレディーキューの先頭は存在するか */
	if (!p->head) {
		/* initタスクは存在する場合 */
		if (*p_ique) {
			current = *p_ique;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* 単一のレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
		current = p->head;

		/* タスクにタイムスライスが設定されていない場合，デフォルトのタイムスライスを設定 */
		if (*p_tm == -1) {
			*p_tm = mg_schdul_info.entry->un.rr_schdul.tmout;
		}
	 	/* 差分のキューのノードを作成 */
		settbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, *p_tm, 0, schedule_rr_callrte, NULL);
		/* スケジューラコントロールブロックとTMRCBの接続 */
		mg_schdul_info.entry->un.rr_schdul.tobjp = (TMR_OBJP)settbf;
	}
}


/*!
* 優先度ビットップを検索する
* bitmap : 検索するスケジューラのビットマップ
* (返却値)E_NG : ビットがない(この関数呼出側でOSをスリープさせる)
* priority : 立っているビット(優先度となる)
*/
static ER_VLE bit_serch(UINT32 bitmap)
{
#if PRIORITY_NUM > 32
#error ビットマップを配列化する
#endif
	ER_VLE priority = 0; /* 下の処理で変化する */

	/*
	* ビットサーチアルゴリズム(検索の高速化)
	* ビットサーチ命令を持っていないので適用した
	* 32ビット変数を半分づつシフトし，下位4ビットを残し，削り取っていく方法でO(logn)となる
	* 下位4ビットのビットパターンは配列としてlsb_4bits_table[]で持っておく
	*/
	if (!(bitmap & 0xffff)) {
		bitmap >>= (PRIORITY_NUM / 2);
		priority += (PRIORITY_NUM / 2);
	}
	if (!(bitmap & 0xff)) {
		bitmap >>= (PRIORITY_NUM / 4);
		priority += (PRIORITY_NUM / 4);
	}
	if (!(bitmap & 0xf)) {
		bitmap >>= (PRIORITY_NUM / 8);
		priority += (PRIORITY_NUM / 8);
	}

	/* 下位4ビット分 */
	priority += mg_ready_info.entry->un.pri.lsb_4bits_table[bitmap & 0xf];
		
	/* ビットが立っていないならば */
	if (priority < 0) {
		return E_NG;
	}
	/* ビットが立っているならば */
	else {
		return priority;
	}
}


/*!
* 優先度スケジューラ
* 優先度によって高いものから順にスケジューリングする
*/
static void schedule_ps(void)
{
	ER_VLE priority;
	TCB **p = &mg_ready_info.init_que;

  priority = bit_serch(mg_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	current = mg_ready_info.entry->un.pri.ready.que[priority].head;
  }
}


/*!
* 優先度×ラウンドロビンスケジューラ
* 優先度の高いものから順にスケジューリングし，タスクをタイムスライスで切っていく
* 同レベルの優先度タスクが公平に(CPUバウンドかI/Oバウンドを無視した場合)スケジューリングされる方式である
*/
static void schedule_rps(void)
{
	ER_VLE priority;
	TMRCB *settbf;
	TCB **p = &mg_ready_info.init_que;
	int *p_tm = &current->schdul_info->un.slice_schdul.tm_slice;

  priority = bit_serch(mg_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	current = mg_ready_info.entry->un.pri.ready.que[priority].head;

		/* タスクにタイムスライスが設定されていない場合，デフォルトのタイムスライスを設定 */
		if (*p_tm == -1) {
			*p_tm = mg_schdul_info.entry->un.rps_schdul.tmout;
		}
	 	/* 差分のキューのノードを作成 */
		settbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, *p_tm, 0, schedule_rps_callrte, NULL);
		/* スケジューラコントロールブロックとTMRCBの接続 */
		mg_schdul_info.entry->un.rps_schdul.tobjp = (TMR_OBJP)settbf;
	}
}


/*!
* Multilevel Feedback Queue
* I/Oバウンドタスクの優先度を下げていく事によって，
* I/OバウンドタスクよりCPUバウンドタスクが優先的に実行されるスケジューリング
*/
static void schedule_mfq(void)
{
	ER_VLE prio;
	int tm_slice;
	TMRCB *tbf;
	TCB **p = &mg_ready_info.init_que;
	
	prio = bit_serch(mg_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (prio == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	current = mg_ready_info.entry->un.pri.ready.que[prio].head;
  
  	/* タスクの優先度をデフォルトスライスタイムを乗算(簡単化) */
  	tm_slice = mg_schdul_info.entry->un.mfq_schdul.tmout * current->priority;
  
	 	/* 差分のキューのノードを作成 */
		tbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, tm_slice, 0, schedule_mfq_callrte, NULL);
		/* スケジューラコントロールブロックとTMRCBの接続 */
		mg_schdul_info.entry->un.mfq_schdul.tobjp = (TMR_OBJP)tbf;
	}
}


/*! 実行キューと満了キューを交換(簡易O(1)スケジューラで使用する関数) */
static void change_tmprique(void)
{
	PRIRQUECB *tmp;
	PRIRQUECB **p_activ = &mg_ready_info.entry->un.tmout_pri.activ;
	PRIRQUECB **p_expired = &mg_ready_info.entry->un.tmout_pri.expired;
	
	tmp = *p_activ;
	*p_activ = *p_expired;
	*p_expired = tmp;
}


/*!
* 簡易O(1)スケジューラ
* I/OバウンドタスクよりCPUバウンドタスクが優先的に実行されるスケジューリング
*/
static void schedule_odrone(void)
{
	ER_VLE prio;
	UINT32 *p = &mg_ready_info.entry->un.tmout_pri.activ->bitmap;
	TCB **p_ique = &mg_ready_info.init_que;
	
	/* activキューをビットサーチ */
	prio = bit_serch(*p);
	
	/* activキューにタスクが存在しない場合 */
	if (prio == E_NG) {
		change_tmprique(); /* キューを入れ替える */
		/* expiredキューをビットサーチ */
		prio = bit_serch(*p);
		/* expiredキューにも存在しない場合 */
		if (prio == E_NG) {
			/* initタスクは存在する場合 */
			if (*p_ique) {
				current = *p_ique;
			}
			/* initタスクは存在しない場合 */
			else {
				down_system();
			}
		}
		/* expiredキューに存在する場合 */
		else {
			active_odrone_schduler((int)prio); /* スケジュール */
		}
	}
	/* activキューにタスクが存在する場合(initタスク以外) */
	else {
		active_odrone_schduler((int)prio); /* スケジュール */
	}
}


/*!
* 簡易O(1)スケジューラの設定
* (仮引数)prio : スケジュール対象の優先度
*/
static void active_odrone_schduler(int prio)
{
	int tm_slice;
	TMRCB *tbf;
	TSK_INITCB *p = &current->init;
	
	/*
	* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
	* (実行状態タスクとしてディスパッチャへ渡す)
	*/
  current = mg_ready_info.entry->un.tmout_pri.activ->que[prio].head;
  
 	/* タスクの優先度をデフォルトスライスタイム重みずけ */
 	/* 優先度半分以上のスライスタイム */
 	if ((*p).priority <= PRIORITY_NUM / 2) {
 		tm_slice = (PRIORITY_NUM - (*p).priority) * 20 * mg_schdul_info.entry->un.odrone_schdul.tmout;
 	}
 	/* 優先度半分以下のスライスタイム */
 	else {
 		tm_slice = (PRIORITY_NUM - (*p).priority) * 5 * mg_schdul_info.entry->un.odrone_schdul.tmout;
 	}
 
	/* 差分のキューのノードを作成 */
	tbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, tm_slice, 0, schedule_odrone_callrte, NULL);
	/* スケジューラコントロールブロックとTMRCBの接続 */
	mg_schdul_info.entry->un.odrone_schdul.tobjp = (TMR_OBJP)tbf;
}


/*!
* Fairスケジューリング時使用
* タイマ割込みが発生しなかった場合のタスク実行時間の習得
*/
static int get_frs_exection_timer(TMRCB *tbf)
{
	int exetim = 0;
	
	/* スケジューラ以外のソフトタイマブロックがある時は差分をすべて加算 */
	for (; tbf != mg_timerque.tmrhead; tbf = tbf->prev) {
		exetim += tbf->msec;
	}
	
	/* 現在のタイマカウント値を加算 */
	exetim += (int)get_timervalue(mg_timerque.index);
	
	return exetim;
}


/*!
* Fair Scheduler
* CPUバウンドタスクとI/Oバウンドタスクをヒューリスティックを使用せずに公平にスケジューリングする方式
*/
static void schedule_fr(void)
{
	TCB *p = mg_ready_info.entry->un.btree.ready.slist_head;
	TCB **p_ique = &mg_ready_info.init_que;
	TMRCB *tbf;
	
	/* 実行可能なタスクが存在しない場合 */
	if (p == NULL) {
		/* initタスクは存在する場合 */
		if (*p_ique) {
			current = *p_ique;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		current = p; /* 整列リストに実行時間順に並んでいるので，先頭を設定 */
  
	 	/* 差分のキューのノードを作成 */
		tbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, mg_schdul_info.entry->un.fr_schdul.tmout, 0, schedule_fr_callrte, NULL);
		/* スケジューラコントロールブロックとTMRCBの接続 */
		mg_schdul_info.entry->un.fr_schdul.tobjp = (TMR_OBJP)tbf;
	}
}


/*!
* Priority Fair Scheduler
* Fairスケジューリングを優先度レベルで行うスケジューリング方式
*/
static void schedule_pfr(void)
{
	ER_VLE prio;
	TCB *p = mg_ready_info.entry->un.pri_btree.ready[current->priority].slist_head;
	TCB **p_ique = &mg_ready_info.init_que;
	TMRCB *tbf;
	
	prio = bit_serch(mg_ready_info.entry->un.pri_btree.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (prio == E_NG) {
		/* initタスクは存在する場合 */
		if (*p_ique) {
			current = *p_ique;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		current = p; /* 整列リストに実行時間順に並んでいるので，先頭を設定 */
  
	 	/* 差分のキューのノードを作成 */
		tbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, mg_schdul_info.entry->un.pfr_schdul.tmout, 0, schedule_pfr_callrte, NULL);
		/* スケジューラコントロールブロックとTMRCBの接続 */
		mg_schdul_info.entry->un.pfr_schdul.tobjp = (TMR_OBJP)tbf;
	}
}


/*!
* Rate Monotonic，Deadline Monotonic展開スケジューリングをするための関数
*** もっと効率よく実装できると思う***
* ・ユーグリッド互除法を改良したバージョン
* ・create()システムコール(task_manage.cのacre_isr())で呼ばれ,create()されたタスクまでの周期の最小公倍数,
*   周期の最小公倍数に沿った最大実行時間を求めて,スケジューラコントロールブロックへセットする
* len : タスク一つあたりの周期またはデッドライン
* exetim : タスク一つあたりの仮想実行時間
*/
void set_unrolled_schedule_val(int len, int exetim)
{
	SCHDUL_TYPE type = mg_schdul_info.type;
	int work_len;
	int work_exetim;
	int tmp_len; /* 現在の総合周期 */
	int rest; /* 余り */
	int quo; /* 商 */
	int now_len = len; /* 退避用(現在の周期) */
	
	if (type == RM_SCHEDULING) {
		tmp_len = work_len = mg_schdul_info.entry->un.rms_schdul.unroll_rate;
		work_exetim = mg_schdul_info.entry->un.rms_schdul.unroll_exetim;
	}
	else {
		tmp_len = work_len = mg_schdul_info.entry->un.dms_schdul.unroll_dead;
		work_exetim = mg_schdul_info.entry->un.dms_schdul.unroll_exetim;
	}
	
	/* すでに周期の最小公倍数と最大実行時間がセットされている場合 */			
	if (work_len && work_exetim) {
		/* ユーグリッド互除法改良ver */
		while ((rest = work_len % len) != 0) {
			work_len = len;
			len = rest;
		}
		/* 周期最小公倍数の計算(スケジューラコントロールブロックへセット) */
		if (type == RM_SCHEDULING) {
			mg_schdul_info.entry->un.rms_schdul.unroll_rate = work_len = tmp_len * now_len / len;
		}
		else {
			mg_schdul_info.entry->un.dms_schdul.unroll_dead = work_len = tmp_len * now_len / len;
		}
		/* 周期に沿った最大実行時間の計算 */
		quo = work_len / tmp_len; /* 乗算する値の決定 */
		work_exetim *= quo; /* すでにセットされているスケジューラコントロールブロックの実行時間メンバの乗算 */
		/* 現在の分を加算(16ビット同士のmsecあたりの乗算となるのでオーバーフローしないように注意) */
		work_exetim += (exetim * (work_len / now_len));
		if (type == RM_SCHEDULING) {
			mg_schdul_info.entry->un.rms_schdul.unroll_exetim = work_exetim; /* スケジューラコントロールブロックへセット */
		}
		else {
			mg_schdul_info.entry->un.dms_schdul.unroll_exetim = work_exetim; /* スケジューラコントロールブロックへセット */
		}
	}
	/* 初期セット */
	else {
		if (type == RM_SCHEDULING) {
			mg_schdul_info.entry->un.rms_schdul.unroll_rate = len;
			mg_schdul_info.entry->un.rms_schdul.unroll_exetim = exetim;
		}
		else {
			mg_schdul_info.entry->un.dms_schdul.unroll_dead = len;
			mg_schdul_info.entry->un.dms_schdul.unroll_exetim = exetim;
		}
	}
}


/*!
* スケジューラのデッドラインミスハンドラ(条件よりスケジュールできない)
* Rate Monotonicに展開スケジューリング専用
*/
static void rmschedule_miss_handler(void)
{
	KERNEL_OUTMSG(" Rate Monotonic Deadline Miss task set\n");
	KERNEL_OUTMSG(" OS sleep... Please push reset button\n");
	freeze_kernel(); /* kernelのフリーズ */
	
}


/*!
* Rate Monotonic Schduler
* 周期的タスクセットに対して起動周期の短いタスクの順にスケジューリングする
* OS実装レベルでは周期順に優先度スケジューリングを行えばよい
*/
static void schedule_rms(void)
{
	ER_VLE priority;
	TCB **p = &mg_ready_info.init_que;
	
	/* Deadline(周期と同じ)ミスをしないかチェック */
  if (mg_schdul_info.entry->un.rms_schdul.unroll_rate < mg_schdul_info.entry->un.rms_schdul.unroll_exetim) {
		rmschedule_miss_handler(); /* Deadlineミスハンドラを呼ぶ */
	}
	
 	priority = bit_serch(mg_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	current = mg_ready_info.entry->un.pri.ready.que[priority].head;
  }
}


/*!
* スケジューラのデッドラインミスハンドラ(条件よりスケジュールできない)
* Rate Monotonicに展開スケジューリング専用
*/
static void dmschedule_miss_handler(void)
{
	KERNEL_OUTMSG(" Deadline Monotonic Deadline Miss task set\n");
	KERNEL_OUTMSG(" OS sleep... Please push reset button\n");
	freeze_kernel(); /* kernelのフリーズ */
	
}


/*!
* Deadline Monotonic Scheduler
* 周期タスクセットに対してデッドラインの短い順にスケジューリングする
* OS実装レベルではデッドライン順に優先度スケジューリングを行えばよい
*/
static void schedule_dms(void)
{
	ER_VLE priority;
	TCB **p = &mg_ready_info.init_que;
	
	/* Deadline(周期と異なる)ミスをしないかチェック */
  if (mg_schdul_info.entry->un.dms_schdul.unroll_dead < mg_schdul_info.entry->un.dms_schdul.unroll_exetim) {
		dmschedule_miss_handler(); /* Deadlineミスハンドラを呼ぶ */
	}
	
 	priority = bit_serch(mg_ready_info.entry->un.pri.ready.bitmap); /* ビットサーチ */
	
	/* 実行可能なタスクが存在しない場合 */
	if (priority == E_NG) {
		/* initタスクは存在する場合 */
		if (*p) {
			current = *p;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		/*
		* ビットサーチ検索した優先度レベルのレディーキューの先頭をスケジュール
		* (実行状態タスクとしてディスパッチャへ渡す)
		*/
  	current = mg_ready_info.entry->un.pri.ready.que[priority].head;
  }
}


/*!
* Earliest Deadline First
* デッドライン時刻の早いものから動的に優先度をつけて，動的優先度の高いタスクからスケジューリングする
* OS実装レベルでは2分木と整列リストを使用して実現している
*/
static void schedule_edf(void)
{
	TCB *p = mg_ready_info.entry->un.btree.ready.slist_head;
	TCB **p_ique = &mg_ready_info.init_que;
	TMRCB *tbf;
	TMR_OBJP *p_tm;

	/* 実行可能なタスクが存在しない場合 */
	if (p == NULL) {
		/* initタスクは存在する場合 */
		if (*p_ique) {
			current = *p_ique;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		current = p; /* 整列リストにデッドライン順に並んでいるので，先頭を設定 */
		p_tm = &current->schdul_info->un.rt_schdul.tobjp;
		/* タスク起動時のみタイマブロックを作成 */
 		if (!(*p_tm)) {
	 		/* 差分のキューのノードを作成 */
			tbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, current->schdul_info->un.rt_schdul.deadtim, 0, edfschedule_miss_handler, NULL);
			/* とTMRCBの接続 */
			*p_tm = (TMR_OBJP)tbf;
		}
	}
}


/*!
* Least Laxity First
* タスクの余裕時刻の小さいものから動的に優先度を割り付けて,動的優先度の高いタスクからスケジューリングする
* OS実装レベルでは2分木と整列リストを使用して実現している
*/
static void schedule_llf(void)
{
	TCB *p = mg_ready_info.entry->un.btree.ready.slist_head;
	TCB **p_ique = &mg_ready_info.init_que;
	TMRCB *tbf;
	TMR_OBJP *p_tm;
	
	/* 実行可能なタスクが存在しない場合 */
	if (p == NULL) {
		/* initタスクは存在する場合 */
		if (*p_ique) {
			current = *p_ique;
		}
		/* initタスクは存在しない場合 */
		else {
			down_system();
		}
	}
	/* 実行するタスクが存在する場合(initタスク以外) */
	else {
		current = p; /* 整列リストに余裕時刻順に並んでいるので，先頭を設定 */
		p_tm = &current->schdul_info->un.rt_schdul.tobjp;
		/* タスク起動時のみタイマブロックを作成 */
 		if (!(*p_tm)) {
	 		/* 差分のキューのノードを作成 */
			tbf = (TMRCB *)create_tmrcb_diffque(SCHEDULER_MAKE_TIMER, current->schdul_info->un.rt_schdul.floatim, 0, llfschedule_miss_handler, NULL);
			/* とTMRCBの接続 */
			*p_tm = (TMR_OBJP)tbf;
		}
	}
}
