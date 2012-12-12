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
#include "task_manage.h"
#include "scheduler.h"
#include "ready.h"
#include "target/driver/timer_driver.h"
#include "timer_callrte.h"


/*! タスクの初期化(静的型taskの領域確保と初期化(可変長配列として使用する)) */
static ER static_tsk_init(int tskids);

/*! タスクの初期化(動的型taskの領域確保と初期化(freeリストを作成する)) */
static ER dynamic_tsk_init(void);

/*! 動的tsk alocリストからセマフォコントロールブロックを抜き取る */
static void get_aloclist(TCB *tcb);

/*! タスクの終了の手続きをする関数 */
static void thread_end(void);

/*! タスク(スレッド)のスタートアップ */
static void thread_init(TCB *thp);

/*! スタックの初期化をする関数 */
static void user_stack_init(TCB *worktcb);

/*! 非タスクコンテキスト用の優先度変更関数 */
static void chg_pri_isyscall_isr(TCB *tcb, int tskpri);

/*! タスクコンテキスト用の優先度変更関数 */
static void chg_pri_syscall_isr(TCB *tcb, int tskpri);


/*!
* タスクの初期化(task ID変換テーブルの領域確保と初期化)
* start_init_tsk()で呼ばれる場合と，taskIDが足らなくなった場合に呼ばれる
* (返却値)E_NOMEM : メモリが取得できない(この返却値は)
* (返却値)E_OK : 正常終了
*/
ER tsk_init(void)
{
	int tskids, i;
	TCB *tcb;
	
	tskids = TASK_ID_NUM << mg_tsk_info.power_count; /* 倍の領域を確保する(start_init_tsk()の時はデフォルト通り) */
	
	mg_tsk_info.id_table = (TCB **)get_mpf_isr(sizeof(tcb) * tskids); /* 変換テーブルの動的メモリ確保 */
	/* taskID変換テーブルの初期化(メモリにNULLを埋めるのにmemset()は使用できない) */
	for (i = 0; i < tskids; i++) {
		mg_tsk_info.id_table[i] = NULL;
	}
	/* TCBの静的型配列の作成及び初期化とTCBの動的型freeリストの作成と初期化 */
	if ((E_NOMEM == static_tsk_init(tskids)) || (E_NOMEM == dynamic_tsk_init())) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	
	return E_OK;
}


/*!
* タスクの初期化(静的型taskの領域確保と初期化(可変長配列として使用する))
* tskids : 確保する可変長配列の個数
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER static_tsk_init(int tskids)
{
	TCB *tcb;
	int size = sizeof(*tcb) * tskids; /* 可変長配列のサイズ */
	
	mg_tsk_info.array = (TCB *)get_mpf_isr(size); /* 可変長配列確保 */
	if (mg_tsk_info.array == NULL) {
		return E_NOMEM; /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
	}
	/* 確保した可変長配列を初期化(可変長でも配列はメモリに連続しているのでこれでOK) */
	memset(mg_tsk_info.array, -1, size);
	
	return E_OK;
}

/*!
* タスクの初期化(動的型taskの領域確保と初期化(freeリストを作成する))
* (返却値)E_NOMEM : メモリが取得できない
* (返却値)E_OK : 正常終了
*/
static ER dynamic_tsk_init(void)
{
	int tskids, i;
	TCB *tcb;
	
	mg_tsk_info.freehead = mg_tsk_info.alochead = NULL;
	
	/* start_init_tsk()の時以外 */
	if (mg_tsk_info.power_count) {
		tskids = TASK_ID_NUM << (mg_tsk_info.power_count - 1); /* 現在と同じ領域を確保する */
	}
	/* start_init_tsk()の時 */
	else {
		tskids = TASK_ID_NUM << mg_tsk_info.power_count;
	}
	
	for (i = 0; i < tskids; i++) {
		tcb = (TCB *)get_mpf_isr(sizeof(*tcb)); /* ノードのメモリ確保 */
		/*メモリが取得できない*/
  	if(tcb == NULL) {
  		return E_NOMEM;	 /* initタスクの時は，start_init_tsk()関数内でOSをスリープさせる */
  	}
		memset(tcb, -1, sizeof(*tcb)); /* 確保したノードを初期化 */
		/* freeキューの作成 */
		tcb->free_next = mg_tsk_info.freehead;
		tcb->free_prev = NULL;
		mg_tsk_info.freehead = tcb->free_next->free_prev = tcb;
	}
	
	return E_OK;
}


/*!
* 動的tsk alocリストからセマフォコントロールブロックを抜き取る
* tcb : 対象のTCB
*/
static void get_aloclist(TCB *tcb)
{
	/* 先頭から抜きとる */
	if (tcb == mg_tsk_info.alochead) {
		mg_tsk_info.alochead = tcb->free_next;
		mg_tsk_info.alochead->free_prev = NULL;
	}
	/* それ以外から抜き取る */
	else {
		tcb->free_prev->free_next = tcb->free_next;
		tcb->free_next->free_prev = tcb->free_prev;
	}
}


/*!
* タスクの終了の手続きをする関数
* タスクの終了はスレッドの延長となるのでシステムコールで終了させると実装が簡単になる
*/
static void thread_end(void)
{
	mz_ext_tsk(); /* システムコール(自タスクの終了) */
}


/*!
* タスク(スレッド)のスタートアップ
* この関数の引数でargcとargvをER1~ER2レジスタ経由でもらう事ができるが，引数が増える事，別CPUに移植する事
* を考えて引数はinit領域で渡す事にする.
* *thp: スタートアップするTCB
*/
static void thread_init(TCB *thp)
{
 
  /* タスク(スレッド)のメイン関数を呼び出す.スレッドはすべて最初はここから呼ばれる */
  thp->init.func(thp->init.argc, thp->init.argv);
  thread_end(); /* タスクの終了 */
}


/*!
* スタックの初期化をする関数
* *worktcb : スタック初期化をするTCBのポインタ
*/
static void user_stack_init(TCB *worktcb)
{
	UINT32 *sp;

	/* スタックの初期化 */
  sp = (UINT32 *)worktcb->stack;

  /*
   *プログラム・カウンタを設定する．
   *スレッドの優先度がゼロの場合には，割込み禁止スレッドとする．
   */
  if (worktcb->priority) {
  	*(--sp) = (UINT32)thread_init | (UINT32) 0 << 24;
  }
  else {
  	*(--sp) = (UINT32)thread_init | (UINT32) 0xc0 << 24;
  }

	/* 各汎用レジスタ分の初期化 */
  *(--sp) = 0; /* ER6 */
  *(--sp) = 0; /* ER5 */
  *(--sp) = 0; /* ER4 */
  *(--sp) = 0; /* ER3 */
  *(--sp) = 0; /* ER2 */
  *(--sp) = 0; /* ER1 */

  /* タスクのスタートアップ(thread_init())に渡す引数 */
  *(--sp) = (UINT32)worktcb;  /* ER0 */

  /* タスクのコンテキストを設定.ここは割込みの度に変化する */
  worktcb->intr_info.sp = (UINT32)sp;
}


/*!
* システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付))
* タスクの状態としては未登録状態から休止状態に移行
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
* (返却値)tcb : 正常終了(作成したセマフォコントロールブロックへのポインタ)
*/
OBJP acre_tsk_isr(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int rel_exetim, int deadtim, int floatim, int argc, char *argv[])
{
  TCB *tcb; /* 新規作成するTCB(タスクコントロールブロック) */
  extern char userstack; /* リンカスクリプトで定義されるスタック領域 */
  static char *task_stack = &userstack;
	SCHDUL_TYPE schdul_type = mg_schdul_info.type;
	READY_TYPE ready_type = mg_ready_info.type;
	SCHDUL_DEP_INFOCB *schdul_info;
	READY_DEP_INFOCB *ready_info;

	/* 静的型taskの場合 */
  if (type == STATIC_TASK) {
  	tcb = &mg_tsk_info.array[mg_tsk_info.counter];
  }
  /* 動的型taskの場合 */
 	else {
 		tcb = mg_tsk_info.freehead; /* free headからノードを与える(抜き取るわけではない) */
 		mg_tsk_info.freehead = tcb->free_next; /* free headを一つ進める */
 		/* aloc headが空の場合 */
 		if (mg_tsk_info.alochead == NULL) {
 			mg_tsk_info.alochead = tcb;
 		}
 	}
 	/* TCBの設定(実行時に変化する内容) */
	tcb->state		&= STATE_CLEAR; /* init()で-1が設定されているので，一度0クリア */
  tcb->state    |= TASK_DORMANT; /* タスクを休止状態へ */
	tcb->get_info.flags = tcb->get_info.gobjp = 0; /* 取得情報を初期化 */
	tcb->wait_info.tobjp = tcb->wait_info.wobjp = 0; /* 待ち情報を初期化 */
  tcb->wait_info.wait_next = tcb->wait_info.wait_prev = NULL; /* 待ちポインタをNULLに */

	/* TCBの設定(実行時に変化しない内容) */
	tcb->init.tskid = mg_tsk_info.counter;
	tcb->init.tskatr = type;
  tcb->init.func = func;
  tcb->init.argc = argc;
  tcb->init.argv = argv;
  strcpy(tcb->init.name, name);
  
  /* TCBの設定(スケジューラごとに依存する内容) */
	schdul_info = (SCHDUL_DEP_INFOCB *)get_mpf_isr(sizeof(*schdul_info)); /* 動的メモリ取得 */
	if (schdul_info == NULL) {
		return E_NOMEM;
	}
		
	/* タスクごとにスライスタイムを決定するスライスタイム型スケジューラの場合 */
	if (schdul_type == RR_SCHEDULING || schdul_type == RR_PRI_SCHEDULING) {
  	schdul_info->un.slice_schdul.tm_slice = -1; /* タスクタイムスライスは初期設定は-1 */
  }
  /* レディーキューを2種類もつ簡易O(1)スケジューラの場合 */
  else if (schdul_type == ODRONE_SCHEDULING) {
  	schdul_info->un.odrone_schdul.readyque_flag = 0; /* まだレディーにつながれていないので，0をセット */
  }
  /* Fair Schedulerの場合 */
  else if (schdul_type == FR_SCHEDULING || schdul_type == PFR_SCHEDULING) {
  	schdul_info->un.fr_schdul.rel_exetim = 0;
  }
  /* Rate Monotonicスケジューリングの場合(周期と実行時間のみ使用) */
  else if (schdul_type == RM_SCHEDULING) {
  	schdul_info->un.rt_schdul.rate = rate;
  	schdul_info->un.rt_schdul.rel_exetim = rel_exetim;
  	/* Rate Monotonicの展開スケジューリングをするための値をセット */
  	set_unrolled_schedule_val(rate, rel_exetim);
  }
  /* Deadline Monotonicスケジューリングの場合(周期と実行時間,デッドラインを使用) */
  else if (schdul_type == DM_SCHEDULING) {
  	schdul_info->un.rt_schdul.rate = rate;
  	schdul_info->un.rt_schdul.rel_exetim = rel_exetim;
  	schdul_info->un.rt_schdul.deadtim = deadtim;
  	/* Deadline Monotonicの展開スケジューリングをするための値をセット */
  	set_unrolled_schedule_val(deadtim, rel_exetim);
  }
	/* Earliest Deadline Firstスケジューリングの場合(デッドライン時刻のみ使用) */
	else if (schdul_type == EDF_SCHEDULING) {
		schdul_info->un.rt_schdul.deadtim = deadtim;
		schdul_info->un.rt_schdul.tobjp = 0;
	}
	/* Least Laxity Firstスケジューリングの場合(余裕時刻のみ使用) */
	else if (schdul_type == LLF_SCHEDULING) {
		schdul_info->un.rt_schdul.floatim = floatim;
		schdul_info->un.rt_schdul.tobjp = 0;
	}
  /* 上記以外のスケジューリング */
  else {
  	/* 処理なし */
  }
  /* タスクコントロールブロックとスケジューラ依存コントロールブロックの接続 */
  tcb->schdul_info = schdul_info;
  	
  /* TCBの設定(レディーごとに依存する内容) */
	ready_info = (READY_DEP_INFOCB *)get_mpf_isr(sizeof(*ready_info)); /* 動的メモリ取得 */
	if (ready_info == NULL) {
		return E_NOMEM;
	}
	
	/* レディーが2分木の場合 */
	if (ready_type == BINARY_TREE || ready_type == PRIORITY_BINARY_TREE) {
		ready_info->un.btree_ready.parent = ready_info->un.btree_ready.left_next = ready_info->un.btree_ready.right_next = 										ready_info->un.btree_ready.sort_next = ready_info->un.btree_ready.sort_prev = NULL;
		ready_info->un.btree_ready.dynamic_prio = -1;
	}
	/* レディーがキューの場合 */
	else {
		ready_info->un.que_ready.ready_next = ready_info->un.que_ready.ready_prev = NULL;
	}
	/* タスクコントロールブロックとレディー依存コントロールブロックの接続 */
	tcb->ready_info = ready_info;

	/* 
	* 静的優先度を使用しないスケジューラの場合
	* なお，init.priorityは作成時の優先度を記録(優先度変更のシステムコールと休止状態があるため)
	*/
	if (schdul_type == FCFS_SCHEDULING || schdul_type == RR_SCHEDULING || schdul_type == EDF_SCHEDULING 																	|| schdul_type == LLF_SCHEDULING || type == FR_SCHEDULING) {
		tcb->init.priority = tcb->priority = -1;
	}
	/* 優先度を使用するスケジューラの場合 */
	else {
  	tcb->init.priority = tcb->priority = priority;
	}

  /* ユーザスタック領域を獲得 */
  memset(task_stack, 0, stacksize);
  task_stack += stacksize; /* ユーザスタック領域を切り出す */
  
 	/* ユーザスタック空間検査 */
 	if (task_stack > (char *)(0xffd000 + 0x002f00)) {
 		DEBUG_OUTMSG("user stack over flow.\n");
 		return E_NOMEM; /* down_system();でスリープさせた方がいいかも */
 	}

 	tcb->stack = task_stack; /* スタックを設定 */

	user_stack_init(tcb); /* ユーザスタックの初期化 */

  return (OBJP)tcb;
}


/*!
* システムコール処理(del_tsk():スレッドの排除)
* 休止状態にあるタスクを未登録状態にする(自タスクの排除は認めない)
* *tcb : 排除するタスクコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクがその他の状態)
*/
ER del_tsk_isr(TCB *tcb)
{
	/*
   * 本来ならスタックも解放して再利用できるようにすべきだが省略．
   * 内臓RAM(16KB)をすべてスタック領域として使用しているため，
   * 頻繁にスレッドを生成してもあふれない．
   */
	/* 休止状態の場合(排除) */
	if (tcb->state == TASK_DORMANT) {
		memset(tcb, -1, sizeof(*tcb)); /* ノードを初期化 */
		/* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
		if (tcb->init.tskatr &= DYNAMIC_TASK) {
			get_aloclist(tcb); /* alocリストから抜き取り */
			/* freeリスト先頭へ挿入 */
			tcb->free_next = mg_tsk_info.freehead;
			tcb->free_prev = mg_tsk_info.freehead->free_prev;
			mg_tsk_info.freehead = tcb->free_next->free_prev = tcb->free_prev->free_next = tcb;
		}
		DEBUG_OUTMSG("delete task contorol block for interrput handler\n");
		
		return E_OK;
	}
	/* その他の状態 */
	else {
		DEBUG_OUTMSG("not delete.\n");
		return E_OBJ;
	}
}


/*!
* システムコール処理(sta_tsk():スレッドの起動)
* タスクの状態としては休止状態から実行可能状態へ移行
* *tcb : 起動するtcb
* (返却値)E_OK : 正常終了
* (返却値)E_OBJ : エラー終了(タスクが休止状態ではない)
*/
ER sta_tsk_isr(TCB *tcb)
{
	current = tcb;
	current->syscall_info.flag = MZ_VOID;
	/* sta_tsk()のシステムコールは休止状態の時の使用可能 */
	if (current->state == TASK_DORMANT) {
		current->state &= ~TASK_DORMANT; /* 休止状態解除 */
		putcurrent(); /* 起動要求タスクをレディーへ */
		return E_OK;
	}
	/* 休止状態ではない場合 */
	else {
		return E_OBJ;
	}
}


/*!
* システムコールの処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)と起動)
* タスクの状態としては未登録状態から休止状態に移行後，実行可能状態へ移行
* オリジナルのシステムコールで，優先度逆転機構のプロトコル(即時型優先度最高値固定プロトコル)
* を効率的に使用するため追加(タスクの切り替えを行わないサービスコールであるmv_acre_tsk()と
* タスクの切り替えを行うシステムコールであるmz_sta_tsk()を併用すれば，run_tsk()システムコールはいらない)
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
* (返却値)rcd(E_PAR) : システムコールの引数不正
* (返却値)rcd(E_NOID) : 動的メモリが取得できない(割付可能なIDがない)
* (返却値)rcd(E_NOMEM) : メモリが確保できない
* (返却値)rcd : 自動割付したID
* (返却値)ercd(E_ID) : エラー終了(タスクIDが不正)
* (返却値)ercd(E_NOEXS) : エラー終了(対象タスクが未登録)
* (返却値)ercd(E_OBJ) : エラー終了(タスクが休止状態ではない)
*/
ER_ID run_tsk_isr(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int rel_exetim, int deadtim, int floatim, int argc, char *argv[])
{
  ER_ID rcd; /* 自動割付IDまたはエラーコードを記録 */
	ER ercd;

	/* タスク生成ルーチン呼び出し */
	rcd = kernelrte_acre_tsk(type, func, name, priority, stacksize, rate, rel_exetim, deadtim, floatim, argc, argv);
	/* 生成できなかった場合はkernelrte_acre_tsk()のエラーコードを返却 */
	if (rcd < E_OK) {
		return rcd;
	}

	/* タスク起動ルーチン呼び出し(kernelrte_sta_tsk()のputcurrent()はすでにレディーへ存在するので，無視される) */
	ercd = kernelrte_sta_tsk(rcd);

	/* 起動できた場合はIDを返却 */
	if (ercd == E_OK) {
		return rcd;
	}
	/* 起動できなかった場合はkernelrte_sta_tsk()のエラーコードを返却 */
	else {
		return (ER_ID)ercd;
	}
}


/*!
* システムコールの処理(ext_tsk():自タスクの終了)
* タスクの状態としては実行状態から休止状態へ移行
* 自タスクの終了は資源を取得しているものは解放する処理をする(ただし，取得したオブジェクト1つの時のみ)
* リターンパラメータはあってはならない
*/
void ext_tsk_isr(void)
{
	TCB *tmpcurrent;
	TMRCB *tbf;
	SCHDUL_TYPE schdul_type = mg_schdul_info.type;
	TMR_OBJP *p_tobjp = &current->schdul_info->un.rt_schdul.tobjp;
	
	/* 
	* カーネルオブジェクトを取得しているかチェック.
	* release_objectの後のコールスタックでcurrentが書き換えられる場合があるので，
	* 呼び出したタスク(自タスク)を一時退避させておく
	*/
	if (current->get_info.flags != TASK_NOT_GET_OBJECT) {
		tmpcurrent = current;
		release_object(current); /* 実体はkernel.cにある */
		current = tmpcurrent;
	}
	
  /* システムコール発行時スレッドはカレントスレッドから抜かれてくるのでstateは0になっている */
  current->state |= TASK_DORMANT; /* スレッドを休止状態へ */
  KERNEL_OUTMSG(current->init.name);
  KERNEL_OUTMSG(" DORMANT.\n");

	/*
	* EDFかLLFで，タイマブロックが作成されている場合
	* EDFまたはLLFは休止状態または未登録状態時にデッドラインタイマを解除する
	* (スライス型スケジューリングのようにプリエンプト時ではない)
	*/
	if ((schdul_type == EDF_SCHEDULING || schdul_type == LLF_SCHEDULING) && *p_tobjp != 0) {
		tbf = (TMRCB *)*p_tobjp;
		/* タイマブロックを排除 */
		delete_tmrcb_diffque(tbf);
		*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
	}
  
  current->priority = current->init.priority; /* タスクの優先度を起動状態へ戻す */
  user_stack_init(current); /* ユーザスタックを起動時に戻す */
}


/*!
* システムコールの処理(exd_tsk():自スレッドの終了と排除)
* TCBが排除されるので返却値はなしにする
*/
void exd_tsk_isr(void)
{
	TCB *tmpcurrent;
	TMRCB *tbf;
	SCHDUL_TYPE schdul_type = mg_schdul_info.type;
	TMR_OBJP *p_tobjp = &current->schdul_info->un.rt_schdul.tobjp;
	
	/*
	* カーネルオブジェクトを取得しているかチェック.
	* release_objectの後のコールスタックでcurrentが書き換えられる場合があるので，
	* 呼び出したタスク(自タスク)を一時退避させておく
	*/
	if (current->get_info.flags != TASK_NOT_GET_OBJECT) {
		tmpcurrent = current;
		release_object(current); /* 実体はkernel.cにある */
		current = tmpcurrent;
	}
	
	/*
   * 本来ならスタックも解放して再利用できるようにすべきだが省略．
   * 内臓RAM(16KB)をすべてスタック領域として使用しているため，
   * 頻繁にスレッドを生成してもあふれない．
   */
  KERNEL_OUTMSG(current->init.name);
  KERNEL_OUTMSG(" EXIT.\n");

	/*
	* EDFかLLFで，タイマブロックが作成されている場合
	* EDFまたはLLFは休止状態または未登録状態時にデッドラインタイマを解除する
	* (スライス型スケジューリングのようにプリエンプト時ではない)
	*/
	if ((schdul_type == EDF_SCHEDULING || schdul_type == LLF_SCHEDULING) && *p_tobjp != 0) {
		tbf = (TMRCB *)*p_tobjp;
		/* タイマブロックを排除 */
		delete_tmrcb_diffque(tbf);
		*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
	}
  
  memset(current, -1, sizeof(*current)); /* ノードの初期化 */
  /* 動的型の場合，alocリストから抜き取りfreeリストへ挿入 */
	if (current->init.tskatr &= DYNAMIC_TASK) {
		get_aloclist(current); /* alocリストから抜き取り */
		/* freeリスト先頭へ挿入 */
		current->free_next = mg_tsk_info.freehead;
		current->free_prev = mg_tsk_info.freehead->free_prev; /* NULLが入る */
		mg_tsk_info.freehead = current->free_next->free_prev = current->free_prev->free_next = current;
		mg_tsk_info.id_table[current->init.tskid] = NULL; /* ID変換テーブルのクリア */
	}
}


/*!
* システムコール処理(ter_tsk():タスクの強制終了)
* 自タスク以外のタスクを強制終了する(実行可能状態と待ち状態にあるもの)
* 実行可能状態にあるものはレディーキューから抜き取る処理，
* 待ち状態にあるものは待ち要件を調べ該当の待ちキューから抜き取る
* 強制終了なので，待ち状態に入っているシステムコールの返却値は書き換えない
* tskid : 強制終了するタスクコントロールブロックへのポインタ
* (返却値)E_ILUSE : エラー終了(タスクが実行状態.つまり自タスク)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了(タスクが実行可能状態または待ち状態)
*/
ER ter_tsk_isr(TCB *tcb)
{
	TMRCB *tbf;
	SCHDUL_TYPE schdul_type = mg_schdul_info.type;
	TMR_OBJP *p_tobjp = &tcb->schdul_info->un.rt_schdul.tobjp;

	/* 実行状態 */
	if (current == tcb) {
		DEBUG_OUTMSG("not temination activ.\n");
		return E_ILUSE;
	}
	/* 休止状態の場合 */
	else if (tcb->state == TASK_DORMANT) {
		DEBUG_OUTMSG("not termination dormant.\n");
		return E_OBJ;
	}
	/* ここから強制終了処理 */
	else {
		/* 実行可能状態 */
		if (tcb->state == TASK_READY) {
			get_tsk_readyque(tcb); /* レディーキューから抜き取る(呼び出した後はcurrentに設定されている) */
			release_object(tcb); /* 何らかのカーネルオブジェクトを取得したままの場合，自動解放する */	
		}
		/* 待ち状態(何らかの待ち行列につながれている時は対象タスクを待ち行列からはずす) */
		else {
			get_tsk_waitque(tcb, (tcb->state & ~TASK_STATE_INFO));
			/* タイマブロックを持っているものは対象タイマブロックを排除する */
			if (tcb->wait_info.tobjp != 0) {
				delete_tmrcb_diffque((TMRCB *)tcb->wait_info.tobjp);
				tcb->wait_info.tobjp = 0; /* クリアにしておく */
			}
			/*
			* カーネルオブジェクトを取得しているかチェック.
			* release_objectの後のコールスタックでcurrentが書き換えられる場合があるので，
			* 一時退避させておく
			*/
			if (tcb->get_info.flags != TASK_NOT_GET_OBJECT) {
				release_object(tcb); /* 何らかのカーネルオブジェクトを取得したままの場合，自動解放する */
			}
		}
		tcb->state |= TASK_DORMANT; /* スレッドを休止状態へ */
		KERNEL_OUTMSG(tcb->init.name);
  	KERNEL_OUTMSG(" DORMANT.\n");

		/*
		* EDFかLLFで，タイマブロックが作成されている場合
		* EDFまたはLLFは休止状態または未登録状態時にデッドラインタイマを解除する
		* (スライス型スケジューリングのようにプリエンプト時ではない)
		*/
		if ((schdul_type == EDF_SCHEDULING || schdul_type == LLF_SCHEDULING) && *p_tobjp != 0) {
			tbf = (TMRCB *)*p_tobjp;
			/* タイマブロックを排除 */
			delete_tmrcb_diffque(tbf);
			*p_tobjp = 0; /* タイマブロックとの接続をクリアにしておく */
		}
  
  	tcb->priority = tcb->init.priority; /* タスクの優先度を起動状態へ戻す */
  	user_stack_init(tcb); /* ユーザスタックを起動時に戻す */
  	
		return E_OK;
	}
}


/*!
* システムコールの処理(get_pri():スレッドの優先度取得)
* tskid : 優先度を参照するタスクコントロールブロックへのポインタ
* *p_tskpri : 参照優先度を格納するポインタ(実体はユーザタスク側で宣言されているもの)
* (返却値)E_OBJ : エラー終了(対象タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER get_pri_isr(TCB *tcb, int *p_tskpri)
{
	/* 休止状態の場合 */
	if (tcb->state == TASK_DORMANT) {
		DEBUG_OUTMSG("not change priority is tsk dormant for interrput hanler.\n");
		return E_OBJ;
	}
	/* タスクの優先度を設定 */
	else {
		*p_tskpri = tcb->priority;
		return E_OK;
	}
}


/*!
* システム・コールの処理(chg_pri():スレッドの優先度変更)
* タスクの状態が未登録状態，休止状態の場合は変更できない
* 同じレベルでの優先度変更は変更対象スレッドはそれぞれの待ち行列の最後に接続される
* E_NOEXSはμITRONにはない
* E_PARは上限優先度違反ではなく(シーリング値はシステムコールの引数指定しない)デフォルト時の優先度個数とする
* (defines.hに記述してあるやつ)
* tcb : 変更対象のタスクコントロールブロックへのポインタ
* tskpri : 変更する優先度
* (返却値)E_PAR : エラー終了(tskpriが不正)
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER chg_pri_isr(TCB *tcb, int tskpri)
{
	/*優先度は有効化か*/
	if (tskpri < 0 || PRIORITY_NUM < tskpri) {
		putcurrent();
		DEBUG_OUTMSG("not change priority is deffer tskpri fot interrput handler.\n");
		return E_PAR;
	}
	/*休止状態の場合*/
	else if (tcb->state == TASK_DORMANT) {
		putcurrent();
		DEBUG_OUTMSG("not change priority is tsk dormant for interrput hanler.\n");
		return E_OBJ;
	}
	/*その他の場合優先度を変更する*/
	else {
		/* 実行状態タスクがタスクコンテキスト用システムコールを呼んだ場合 */
		if (current->syscall_info.flag == MZ_SYSCALL) {
				chg_pri_syscall_isr(tcb, tskpri); /*優先度変更*/
		}
		/* 実行状態タスクが非タスクコンテキスト用システムコールを呼んだ場合 */
		else if (current->syscall_info.flag == MZ_ISYSCALL) {
			chg_pri_isyscall_isr(tcb, tskpri); /*優先度変更*/
		}
		return E_OK;
	}
}


/*!
* タスクコンテキスト用の優先度変更関数
* システムコール発行タスク(実行状態タスク)はレディーから抜き取られてくる
* *tcb : 優先度変更対象タスク
* tskpri : 変更する優先度
*/
static void chg_pri_syscall_isr(TCB *tcb, int tskpri)
{
	/*
	* 優先度変更タスクが実行状態の場合，実行状態タスクはレディーから抜き取られてくるので，
	* 優先度を変更してから，実行状態タスクをレディーへ戻す
	*/
	if (tcb == current) {
		current->priority = tskpri; /* 実行状態タスクの優先度変更 */
		putcurrent(); /* システムコール発行タスクをレディーへ */
		puts("tskid ");
		putxval(current->init.tskid, 0);
		puts(" ");
		DEBUG_OUTVLE(current->priority, 0);
		DEBUG_OUTMSG(" change priority activ(context syscall) for interruput handler.\n");
	}
	/*
	* 優先度変更タスクが実行可能状態(レディーに存在する)の場合，実行状態タスクはレディーから抜き取られてくるので，
	* 実行状態タスクをレディーへ戻してから(get_tsk_readyque()実行後，抜き取りタスクはcurrentに設定される)，
	* 実行可能状態タスクをレディーから抜き取り，優先度を変更して，レディーへ戻す
	*/
	else if (tcb->state & TASK_READY) {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		get_tsk_readyque(tcb); /* レディーキューから抜き取る関数(scheduler.cにある) */
		current->priority = tskpri; /* 実行可能状態タスクの優先度を変更 */
		putcurrent(); /* 変更したタスクをレディーへ */
		puts("tskid ");
		putxval(current->init.tskid, 0);
		puts(" ");
		DEBUG_OUTVLE(current->priority, 0);
		DEBUG_OUTMSG(" change priority ready(context syscall) for interruput handler.\n");
	}
	/*
	* 優先度変更タスクが待ち状態(レディーに存在しない)の場合，実行状態タスクはレディーから抜き取られてくるので，
	* 待ち状態タスクの優先度を変更して，実行状態タスクをレディーへ戻す
	*/
	else {
		putcurrent(); /* システムコール発行タスクをレディーへ */
		tcb->priority = tskpri; /* 待ち状態タスクの優先度変更 */
		puts("tskid ");
		putxval(tcb->init.tskid, 0);
		puts(" ");
		DEBUG_OUTVLE(tcb->priority, 0);
		DEBUG_OUTMSG(" change priority sleep(context syscall) for interruput handler.\n");
	}
}


/*!
* 非タスクコンテキスト用の優先度変更関数
* -システムコール発行タスク(実行状態タスク)はレディーから抜き取られてくる場合と抜き取られない場合がある
* 抜き取られる場合 : システムコール割込みの非タスクコンテキストで呼ばれる
* 抜き取られない場合 : シリアル割込みハンドラの非タスクコンテキストで呼ばれる(シリアル割込みはタスクを切り替えず，
*                     かつユーザが自作できるものとしてしている)
* *tcb : 優先度変更対象タスク
* tskpri : 変更する優先度
*/
static void chg_pri_isyscall_isr(TCB *tcb, int tskpri)
{
	/*
	* /優先度変更タスクが実行状態の場合，実行状態タスクはレディーから抜き取られる場合と
	* 抜き取られない場合がある
	* /抜き取られる場合は優先度を変更するのみ
	* /抜き取られない場合は一度レディーから実行状態タスクを抜き取り，優先度を変更して，再度レディーへ戻す
	*/
	if (tcb == current) {
		/* 抜き取られない場合 */
		if (tcb->intr_info.type == SERIAL_INTERRUPT) {
			getcurrent(); /* 非タスクコンテキスト用システムコール発行タスクをレディーへ */
		}
		current->priority = tskpri; /* 実行状態タスクの優先度変更 */
		/* 抜き取られない場合 */
		if (tcb->intr_info.type == SERIAL_INTERRUPT) {
			putcurrent(); /* 非タスクコンテキスト用システムコール発行タスクをレディーへ */
		}
		puts("tskid ");
		putxval(current->init.tskid, 0);
		puts(" ");
		DEBUG_OUTVLE(current->priority, 0);
		DEBUG_OUTMSG(" change priority activ(not context syscall) for interruput handler.\n");
	}
	/*
	* 優先度変更タスクが実行可能状態(レディーに存在する)の場合，実行状態タスクはレディーから抜き取られないので，
	* 実行可能状態タスクをレディーから抜き取り，優先度を変更して，レディーへ戻す
	*/
	else if (tcb->state & TASK_READY) {
		get_tsk_readyque(tcb); /* レディーキューから抜き取る関数(scheduler.cにある) */
		current->priority = tskpri;
		putcurrent();
		puts("tskid ");
		putxval(current->init.tskid, 0);
		puts(" ");
		DEBUG_OUTVLE(current->priority, 0);
		DEBUG_OUTMSG(" change priority ready(not context syscall) for interruput handler.\n");
	}
	/*
	* 優先度変更タスクが待ち状態(レディーに存在しない)の場合，実行状態タスクはレディーから抜き取られないので，
	* 待ち状態タスクの優先度を変更のみ
	*/
	else {
		tcb->priority = tskpri;
		puts("tskid ");
		putxval(tcb->init.tskid, 0);
		puts(" ");
		DEBUG_OUTVLE(tcb->priority, 0);
		DEBUG_OUTMSG(" change priority sleep(not context syscall) for interruput handler.\n");
	}
}


/*!
* システム・コールの処理(chg_slt():タスクタイムスライスの変更)
* 有効化されているスケジューラによってスライスタイム変更の手順がかわる
* RRとRR_PRIはタスクごと，MFQ，O(1)は優先度レベルごととなる
* type : スケジューラのタイプ
* *tcb : タイムスライスを変更するタスクコントロールブロックへのポインタ
* slice : 変更するタスクスライスタイム
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_PAR : エラー終了(tm_sliceが不正)
* (返却値)E_OK : 正常終了
*/
ER chg_slt_isr(SCHDUL_TYPE type, TCB *tcb, int slice)
{
	/* ラウンドロビンスケジューラまたはラウンドロビン×優先度スケジューリングの場合 */
	if (type == RR_SCHEDULING || type == RR_PRI_SCHEDULING) {
		/* 休止状態の場合 */
		if (tcb->state == TASK_DORMANT) {
			return E_OBJ;
		}
		/* パラメータチェック */
		else if (slice < TMR_EFFECT) {
			return E_PAR;
		}
		/* タスクタイムスライス変更 */
		else {
			tcb->schdul_info->un.slice_schdul.tm_slice = slice;
			return E_OK;
		}
	}
	/* Multilevel Feedback Queueの場合 */
	else if (type == MFQ_SCHEDULING) {
		mg_schdul_info.entry->un.mfq_schdul.tmout = slice;
		return E_OK;
	}
	/* 簡易O(1)スケジューリングの場合 */
	else if (type == ODRONE_SCHEDULING) {
		mg_schdul_info.entry->un.odrone_schdul.tmout = slice;
		return E_OK;
	}
	else {
		/* 処理なし */
		return E_OK;
	}
}


/*!
* システム・コールの処理(get_slt():タスクタイムスライスの取得)
* type : スケジューラのタイプ
* *tcb : タイムスライスを変更するタスクコントロールブロックへのポインタ
* *p_slice : タイムスライスを格納するパケットへのポインタ
* (返却値)E_OBJ : エラー終了(タスクが休止状態)
* (返却値)E_OK : 正常終了
*/
ER get_slt_isr(SCHDUL_TYPE type, TCB *tcb, int *p_slice)
{
	/* ラウンドロビンスケジューラまたはラウンドロビン×優先度スケジューリングの場合 */
	if (type == RR_SCHEDULING || type == RR_PRI_SCHEDULING) {
		/* 休止状態の場合 */
		if (tcb->state == TASK_DORMANT) {
			return E_OBJ;
		}
		/* タイムスライスを格納 */
		else {
			*p_slice = tcb->schdul_info->un.slice_schdul.tm_slice;
			return E_OK;
		}
	}
	/* Multilevel Feedback Queueの場合 */
	else if (type == MFQ_SCHEDULING) {
		*p_slice = mg_schdul_info.entry->un.mfq_schdul.tmout;
		return E_OK;
	}
	/* 簡易O(1)スケジューリングの場合 */
	else if (type == ODRONE_SCHEDULING) {
		*p_slice = mg_schdul_info.entry->un.odrone_schdul.tmout;
		return E_OK;
	}
	else {
		/* 処理なし */
		return E_OK;
	}
}
