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
#include "prinvermutex.h"
#include "ready.h"


/*! 遅延型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストで繋ぐ) */
static void put_dyhighest_loker_mtx(MTXCB *mcb);

/*! 遅延型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストから抜き取る) */
static void get_dyhighest_loker_mtx(MTXCB *mcb);

/*! 遅延型優先度最高値固定プロトコル専用(mutexをリリースした時のオーナータスク優先度取得) */
static R_VLE get_dyhighest_locker_tskpri(int tskid);

/*! 即時型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストで繋ぐ) */
static void put_imhighest_loker_mtx(MTXCB *mcb);

/*! 即時型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストから抜き取る) */
static void get_imhighest_loker_mtx(MTXCB *mcb);

/*! 即時型優先度最高値固定プロトコル専用(mutexをリリースした時のオーナータスク優先度取得) */
static R_VLE get_imhighest_locker_tskpri(int tskid);

/*! 優先度上限プロトコル専用(タスクがロックし，シーリング優先度となったmutexを連結リストで繋ぐ) */
static void put_ceilpri_mtx(MTXCB *mcb);

/*! タスクがクリティカルセクションに入れるか */
static BOOL is_criticalsection(MTXCB *mcb, int tskpri, int tskid);

/*! 優先度上限プロトコル専用(タスクがアンロックし，mutexを連結リストから抜き取る) */
static void get_ceilpri_mtx(MTXCB *mcb);

/*! 優先度上限プロトコル専用(mutexをリリースした時のオーナータスク優先度取得) */
static R_VLE get_ceiling_tskpri(int tskid);

/*! virtual mutexの初期ロックをする関数 */
static void loc_virtual_mtx(int tskid);

/*! virtual mutexの再帰ロックをする関数 */
static void loc_multipl_virtual_mtx(void);

/*! virtual mutex待ちタスクの追加 */
static void wait_virtual_mtx_tsk(MTXCB *mcb);

/*! virtual mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理 */
static ER not_lock_first_virtual_mtx(void);

/*! virtual mutexオーナーシップ解除処理 */
static void clear_virtual_mtx_ownership(void);

/*! virtul mutex待ちから，物理mutexを与えタスクをレディーへ戻す関数の分岐 */
static void put_virtual_mtx_waittsk(MTXCB *mcb);

/*! virtual mutex待ちタスクにvirtual mutexと物理mutexを与えレディーへ先頭からレディーへ入れる */
static void put_virtual_mtx_fifo_ready(MTXCB *mcb);

/*!  virtual mutex待ちタスクにvirtual mutexと物理mutexを与えレディーへ先頭からレディーへ入れる */
static void put_virtual_mtx_pri_ready(MTXCB *mcb);

/*! virtual mutex待ちキューからスリープTCBを抜き取る関数 */
void get_tsk_virtual_mtx_waitque(MTXCB *mcb, TCB *maxtcb);



/*!
* 遅延型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストで繋ぐ)
* *mcb : 繋ぐMTXCB
*/
static void put_dyhighest_loker_mtx(MTXCB *mcb)
{
	/* mg_mtx_infoのdyhigh loc headにつなぐ */
	mcb->pcl.pcl_next = mg_mtx_info.dyhigh_loc_head;
	mcb->pcl.pcl_prev = NULL;
	mg_mtx_info.dyhigh_loc_head = mcb->pcl.pcl_next->pcl.pcl_prev = mcb;
}


/*!
* 遅延型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストから抜き取る)
* *mcb : 抜き取るMTXCB
*/
static void get_dyhighest_loker_mtx(MTXCB *mcb)
{
	/* mg_mtx_infoのdyhigh_loc_headから抜き取る */
	if (mcb == mg_mtx_info.dyhigh_loc_head) {
		mg_mtx_info.dyhigh_loc_head = mcb->pcl.pcl_next;
		mg_mtx_info.dyhigh_loc_head->pcl.pcl_prev = NULL;
	}
	/* dyhigh_loc_head以外から抜き取り */
	else {
		mcb->pcl.pcl_prev->pcl.pcl_next = mcb->pcl.pcl_next;
		mcb->pcl.pcl_next->pcl.pcl_prev = mcb->pcl.pcl_prev;
	}
  mcb->pcl.pcl_next = mcb->pcl.pcl_prev = NULL;
}


/*!
* 遅延型優先度最高値固定プロトコル専用(mutexをリリースした時のオーナータスク優先度取得)
* tskid : オーナータスクID
*/
static R_VLE get_dyhighest_locker_tskpri(int tskid)
{
	MTXCB *workmcb = mg_mtx_info.dyhigh_loc_head;
	int chgpri = mg_tsk_info.id_table[tskid]->init.priority; /* 起動時の優先度を格納しておく */

	putxval(chgpri, 0);
	puts(" init priority for delay highest locker\n");

	/* highest lockerしたmutexの連結リストを検索する */
	for (workmcb = mg_mtx_info.dyhigh_loc_head; workmcb != NULL; workmcb = workmcb->pcl.pcl_next) {
		/* オーナータスクとなっているmutexであり，優先度が高いものを選択する */
		if (workmcb->ownerid == tskid && chgpri > workmcb->pcl.pcl_param) {
			chgpri = workmcb->pcl.pcl_param;
		}
	}

	putxval(chgpri, 0);
	puts(" chgpri priority delay highest locker\n");
	return chgpri;
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(遅延型Highest Lockerプロトコル))
* -遅延型highest lockerはタスクがmutexを取得できなかった時に，最高値固定まで引き上げる
* -最高値固定はmutexごとに割り付ける．単に最高値固定を一つとすると，高い優先度タスクの処理が遅延するから
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER loc_dyhighmtx_isr(MTXCB *mcb)
{
  int tskid, ownerpri;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */
	ownerpri = mg_tsk_info.id_table[mcb->ownerid]->priority; /* mutexをロックした時のオーナー優先度を取得 */
  
	/* ここからロック操作 */
	if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		/* 待ちタスクに追加されるときにhighest lockerへ引き上げ */
		if (mcb->ownerid != tskid) {
			/* Highest Lockerに引き上げるか */
   		if (ownerpri > mcb->pcl.pcl_param) {
				put_dyhighest_loker_mtx(mcb); /* highest lockerしたmutexをmg_mtx_infoエリアへつなぐ */
 				mz_ichg_pri(mcb->ownerid, mcb->pcl.pcl_param); /*最高値固定に設定*/
  		}
			wait_mtx_tsk(mcb, TMO_POL); /* mutex待ちタスクの追加をする関数 */
    	return E_OK;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* 再帰ロックをする関数 */
  	}
  }

  /* 初期ロック */
  else {
  	loc_first_mtx(tskid, mcb); /* 初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(遅延型Higest Lockerプロトコル))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER unl_dyhighmtx_isr(MTXCB *mcb)
{
	int tskid, ownerpri, chgpri;
	
  tskid = current->init.tskid; /* アンロックしてきたタスクのIDを取得 */
  ownerpri = mg_tsk_info.id_table[mcb->ownerid]->init.priority; /* タスク起動時の優先度 */

	/* スリープTCBは一度にすべて戻してはいけない */
  
  /* ここからアンロック操作 */
  if (mcb->ownerid == tskid) { /* オーナータスクか */
  	/* 再帰ロック解除状態の場合 */
  	if (mcb->locks == 1) {
  		/* 待ちタスクが存在する場合 */
			if (mcb->waithead != NULL) {
				/* オーナーとなっていたタスクの優先度を変更するか */
				if (current->priority != ownerpri) { /* 優先度はhighest lockerとなったか */
					get_dyhighest_loker_mtx(mcb); /* highest lockerしたmutexをmg_mtx_infoエリアから抜き取る */
					/* 変更する優先度の決定(必ずしもinit.priority時の優先度になるわけではない) */
					chgpri = get_dyhighest_locker_tskpri(tskid);
					mz_ichg_pri(tskid, chgpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				/* 取得情報クリア */
				current->get_info.flags &= ~TASK_GET_MUTEX;
				current->get_info.gobjp = 0;
				putcurrent(); /* オーナータスクをレディーへ */
				put_mtx_waittsk(mcb); /* 待ちタスクをレディーへ */
    		return E_OK;
  		}
  		/* 待ちタスクが存在しなければ，オーナーシップのクリア */
  		else {
				/* オーナーとなっていたタスクの優先度を変更するか */
				if (current->priority != ownerpri) { /* 優先度はhighest lockerとなったか */
					get_dyhighest_loker_mtx(mcb); /* highest lockerしたmutexをmg_mtx_infoエリアから抜き取る */
					/* 変更する優先度の決定(必ずしもinit.priority時の優先度になるわけではない) */
					chgpri = get_dyhighest_locker_tskpri(tskid);
					mz_ichg_pri(tskid, chgpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				putcurrent(); /* システムコール発行スレッドをレディーへ */
				clear_mtx_ownership(mcb); /* オーナーシップのクリアをする関数 */
  			return E_OK;
  		}
  	}
		return not_lock_first_mtx(mcb); /*初期ロック状態(mxvalueが1)以外の場合の場合の処理*/
	}

  /* オーナータスクでない場合 */
  else {
  	putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("not release mutex semaphoreID for interrput handler\n");
  	return E_ILUSE;
  }
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(優先度継承プロトコル))
* -優先度継承プロトコルはタスクがmutexをロックできなかった時に，mutexオーナータスクの優先度をロック要求タスクまで継承する
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER loc_inhermtx_isr(MTXCB *mcb)
{
  int tskid, tskpri;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */
  tskpri = current->priority; /* ロックしてきたタスクの優先度を取得 */
  
	/* ここからロック操作 */
	if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		if (mcb->ownerid != tskid) {
			if (tskpri < (mg_tsk_info.id_table[mcb->ownerid]->priority)) {
				mz_ichg_pri(mcb->ownerid, tskpri); /* 優先度の低いタスクの優先度を引き上げる(オーナーとなっているタスク自身) */
			}
			wait_mtx_tsk(mcb, TMO_POL); /* mutex待ちタスクの追加をする関数 */
    	return E_OK;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* 再帰ロックをする関数 */
  	}
  }

  /* 初期ロック */
  else {
  	loc_first_mtx(tskid, mcb); /* 初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(優先度継承プロトコル))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER unl_inhermtx_isr(MTXCB *mcb)
{
	int tskid, ownerpri;
	
  tskid = current->init.tskid; /* アンロックしてきたタスクのIDを取得 */
  ownerpri = mg_tsk_info.id_table[mcb->ownerid]->init.priority;

	/* スリープTCBは一度にすべて戻してはいけない */
  
  /* ここからアンロック操作 */
  if (mcb->ownerid == tskid) { /* オーナータスクか */
  	/* 再帰ロック解除状態の場合 */
  	if (mcb->locks == 1) {
  		/* 待ちタスクが存在する場合 */
			if (mcb->waithead != NULL) {
				/* オーナーとなっていたタスクの優先度戻すか */
				if (current->priority != ownerpri) {/* タスクの優先度は継承されたか */
					mz_ichg_pri(tskid, ownerpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				/* 取得情報クリア */
				current->get_info.flags &= ~TASK_GET_MUTEX;
				current->get_info.gobjp = 0;
				putcurrent(); /* オーナータスクをレディーへ */
				put_mtx_waittsk(mcb); /* 待ちタスクをレディーへ */
    		return E_OK;
  		}
  		/* 待ちタスクが存在しなければ，オーナーシップのクリア */
  		else {
				if (current->priority != ownerpri) {/* タスクの優先度は継承されたか */
					mz_ichg_pri(tskid, ownerpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				putcurrent(); /* システムコール発行スレッドをレディーへ */
				clear_mtx_ownership(mcb); /* オーナーシップのクリアをする関数 */
  			return E_OK;
  		}
  	}
		return not_lock_first_mtx(mcb); /*初期ロック状態(mxvalueが1)以外の場合の場合の処理*/
	}

  /* オーナータスクでない場合 */
  else {
  	putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("not release mutex semaphoreID for interrput handler\n");
  	return E_ILUSE;
  }
}


/*!
* 即時型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストで繋ぐ)
* *mcb : 繋ぐMTXCB
*/
static void put_imhighest_loker_mtx(MTXCB *mcb)
{
	/* mg_mtx_infoのimhigh loc headにつなぐ */
	mcb->pcl.pcl_next = mg_mtx_info.imhigh_loc_head;
	mcb->pcl.pcl_prev = NULL;
	mg_mtx_info.imhigh_loc_head = mcb->pcl.pcl_next->pcl.pcl_prev = mcb;
}


/*!
* 即時型優先度最高値固定プロトコル専用(highest lockerしたmutexを連結リストから抜き取る)
* *mcb : 抜き取るMTXCB
*/
static void get_imhighest_loker_mtx(MTXCB *mcb)
{
	/* mg_mtx_infoのimhigh_loc_headから抜き取る */
	if (mcb == mg_mtx_info.imhigh_loc_head) {
		mg_mtx_info.imhigh_loc_head = mcb->pcl.pcl_next;
		mg_mtx_info.imhigh_loc_head->pcl.pcl_prev = NULL;
	}
	else {
		mcb->pcl.pcl_prev->pcl.pcl_next = mcb->pcl.pcl_next;
		mcb->pcl.pcl_next->pcl.pcl_prev = mcb->pcl.pcl_prev;
	}
  mcb->pcl.pcl_next = mcb->pcl.pcl_prev = NULL;
}


/*!
* 即時型優先度最高値固定プロトコル専用(mutexをリリースした時のオーナータスク優先度取得)
* tskid : オーナータスクID
*/
static R_VLE get_imhighest_locker_tskpri(int tskid)
{
	MTXCB *workmcb = mg_mtx_info.imhigh_loc_head;
	int chgpri = mg_tsk_info.id_table[tskid]->init.priority; /* 起動時の優先度を格納しておく */

	putxval(chgpri, 0);
	puts(" inti priority\n");

	/* highest lockerしたmutexの連結リストを検索する */
	for (workmcb = mg_mtx_info.imhigh_loc_head; workmcb != NULL; workmcb = workmcb->pcl.pcl_next) {
		/* オーナータスクとなっているmutexであり，優先度が高いものを選択する */
		if (workmcb->ownerid == tskid && chgpri > workmcb->pcl.pcl_param) {
			chgpri = workmcb->pcl.pcl_param;
		}
	}

	putxval(chgpri, 0);
	puts(" chgpri output\n");
	return chgpri;
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(即時型Higest Lockerプロトコル))
* -即時型highest lockerはmutexの初期ロック時に最高値固定までタスクの優先度を引き上げる
* -最高値固定はmutexごとに割り付ける．単に最高値固定を一つとすると，高い優先度タスクの処理が遅延するから
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER loc_imhighmtx_isr(MTXCB *mcb)
{
  int tskid, tskpri;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */
  tskpri = current->priority; /* ロックしてきたタスクの優先度を取得 */
  
	/* ここからロック操作 */
	if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		if (mcb->ownerid != tskid) {
			wait_mtx_tsk(mcb, TMO_POL); /* mutex待ちタスクの追加をする関数 */
    	return E_OK;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* 再帰ロックをする関数 */
  	}
  }

  /* 初期ロック */
	/* 初期ロック時でhigest lockerへ引き上げ */
  else {
		/* Highest Lockerに引き上げるか*/
   	if (tskpri > mcb->pcl.pcl_param) {
			put_imhighest_loker_mtx(mcb); /* highest lockerしたmutexをmg_mtx_infoエリアへつなぐ */
 			mz_ichg_pri(tskid, mcb->pcl.pcl_param); /*最高値固定に設定*/
  	}
  	loc_first_mtx(tskid, mcb); /* 初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(即時型Higest Lockerプロトコル))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER unl_imhighmtx_isr(MTXCB *mcb)
{
	int tskid, ownerpri, chgpri;
	
  tskid = current->init.tskid; /* アンロックしてきたタスクのIDを取得 */
  ownerpri = mg_tsk_info.id_table[mcb->ownerid]->init.priority; /* タスク起動時の優先度 */

	/* スリープTCBは一度にすべて戻してはいけない */
  
  /* ここからアンロック操作 */
  if (mcb->ownerid == tskid) { /* オーナータスクか */
  	/* 再帰ロック解除状態の場合 */
  	if (mcb->locks == 1) {
  		/* 待ちタスクが存在する場合 */
			if (mcb->waithead != NULL) {
				/* オーナーとなっていたタスクの優先度を変更するか */
				if (current->priority != ownerpri) { /* 優先度はhighest lockerとなったか */
					get_imhighest_loker_mtx(mcb); /* highest lockerしたmutexをmg_mtx_infoエリアから抜き取る */
					/* 変更する優先度の決定(必ずしもinit.priority時の優先度になるわけではない) */
					chgpri = get_imhighest_locker_tskpri(tskid);
					mz_ichg_pri(tskid, chgpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				/* 取得情報クリア */
				current->get_info.flags &= ~TASK_GET_MUTEX;
				current->get_info.gobjp = 0;
				putcurrent(); /* オーナータスクをレディーへ */
				put_mtx_waittsk(mcb); /* 待ちタスクをレディーへ */
    		return E_OK;
  		}
  		/* 待ちタスクが存在しなければ，オーナーシップのクリア */
  		else {
				/* オーナーとなっていたタスクの優先度を変更するか */
				if (current->priority != ownerpri) { /* 優先度はhighest lockerとなったか */
					get_imhighest_loker_mtx(mcb); /* highest lockerしたmutexをmg_mtx_infoエリアから抜き取る */
					/* 変更する優先度の決定(必ずしもinit.priority時の優先度になるわけではない) */
					chgpri = get_imhighest_locker_tskpri(tskid);
					mz_ichg_pri(tskid, chgpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				putcurrent(); /* システムコール発行スレッドをレディーへ */
				clear_mtx_ownership(mcb); /* オーナーシップのクリアをする関数 */
  			return E_OK;
  		}
  	}
		return not_lock_first_mtx(mcb); /*初期ロック状態(mxvalueが1)以外の場合の場合の処理*/
	}

  /* オーナータスクでない場合 */
  else {
  	putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("not release mutex semaphoreID for interrput handler\n");
  	return E_ILUSE;
  }
}


/*!
* 優先度上限プロトコル専用(タスクがロックし，シーリング優先度となったmutexを連結リストで繋ぐ)
* *mcb : 繋ぐMTXCB
*/
static void put_ceilpri_mtx(MTXCB *mcb)
{
	/* mg_mtx_infoのimhigh loc headにつなぐ */
	mcb->pcl.pcl_next = mg_mtx_info.ceil_pri_head;
	mcb->pcl.pcl_prev = NULL;
	mg_mtx_info.ceil_pri_head = mcb->pcl.pcl_next->pcl.pcl_prev = mcb;
}


/*!
* タスクがクリティカルセクションに入れるか
* /シーリング優先度となったmutexを検索し，ロック要求タスクの優先度より低ければ，
*  タスクをクリティカルセクションに入れる
* tskpri : ロック要求タスクの優先度
* (返却値)FALSE : クリティカルセクションに入れない
* (返却値)TRUE : クリティカルセクションに入れる
*/
static BOOL is_criticalsection(MTXCB *mcb, int tskpri, int tskid)
{
	MTXCB *workmcb;
	int tmpid;

	/* シーリング優先度となっているmutexの連結リストを検索する */
	for (workmcb = mg_mtx_info.ceil_pri_head; workmcb != NULL; workmcb = workmcb->pcl.pcl_next) {
		/* 他のタスクがmutexをロックしているか */
		if (workmcb->ownerid != tskid) {
			/* ロック要求タスクより高いシーリング優先度となっているか */
			if (tskpri >= workmcb->pcl.pcl_param) {
				puts("tskid ");
				putxval(workmcb->ownerid, 0);
				puts(" ");
				putxval(workmcb->pcl.pcl_param, 0);
				puts(" diff workmcb->pcl.pcl_param output\n");
				return FALSE;
			}
			tmpid = mcb->ownerid;
		}
	}

	/*
	* TRUEが返却される時は，
	* ①他のタスクが一つもmutexを取得していない場合．
	* ②他のタスクがmutexをロックしているが，ロック要求タスクより高いシーリング優先度が存在しない場合．
	*/
	return TRUE;
}


/*!
* 優先度上限プロトコル専用(タスクがアンロックし，mutexを連結リストから抜き取る)
* *mcb : 抜き取るMTXCB
*/
static void get_ceilpri_mtx(MTXCB *mcb)
{
	/* mg_mtx_infoのceil_pri_headから抜き取る */
	if (mcb == mg_mtx_info.ceil_pri_head) {
		mg_mtx_info.ceil_pri_head = mcb->pcl.pcl_next;
		mg_mtx_info.ceil_pri_head->pcl.pcl_prev = NULL;
	}
	else {
		mcb->pcl.pcl_prev->pcl.pcl_next = mcb->pcl.pcl_next;
		mcb->pcl.pcl_next->pcl.pcl_prev = mcb->pcl.pcl_prev;
	}
  mcb->pcl.pcl_next = mcb->pcl.pcl_prev = NULL;
}


/*!
* 優先度上限プロトコル専用(mutexをリリースした時のオーナータスク優先度取得)
* tskid : オーナータスクID
*/
static R_VLE get_ceiling_tskpri(int tskid)
{
	MTXCB *workmcb = mg_mtx_info.ceil_pri_head;
	int chgpri = mg_tsk_info.id_table[tskid]->init.priority; /* 起動時の優先度を格納しておく */

	putxval(chgpri, 0);
	puts(" init priority for priority ceiling protocol\n");

	/* highest lockerしたmutexの連結リストを検索する */
	for (workmcb = mg_mtx_info.ceil_pri_head; workmcb != NULL; workmcb = workmcb->pcl.pcl_next) {
		/* オーナータスクとなっているmutexであり，優先度が高いものを選択する */
		if (workmcb->ownerid == tskid && chgpri > workmcb->pcl.pcl_param) {
			chgpri = workmcb->pcl.pcl_param;
		}
	}

	putxval(chgpri, 0);
	puts(" chgpri output for priority ceiling protocol\n");
	return chgpri;
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(優先度上限プロトコル))
* -優先度上限プロトコルは優先度シーリングより，ロック要求タスクの優先度が高ければ，
*  クリティカルセクションに入れる．ロック要求mutexがすでにロック状態の場合は優先度継承プロトコルとする
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER loc_ceilmtx_isr(MTXCB *mcb)
{
  int tskid, tskpri;
	MTXCB *workmcb;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */
  tskpri = current->priority; /* ロックしてきたタスクの優先度を取得 */
  
	/* ここからロック操作 */
	if (mcb->ownerid != -1) { /* ロックしているタスクがあるか */
		/* オーナータスクでない場合 */
		if (mcb->ownerid != tskid) {
			/* 優先度継承プロトコルの部分 */
			if (tskpri < (mg_tsk_info.id_table[mcb->ownerid]->priority)) {
				mz_ichg_pri(mcb->ownerid, tskpri); /* 優先度の低いタスクの優先度を引き上げる(オーナーとなっているタスク自身) */
			}
			wait_mtx_tsk(mcb, TMO_POL); /* mutex待ちタスクの追加をする関数 */
    	return E_OK;
		}
  	/* オーナータスクの場合，再帰ロック */
  	else {
  		return loc_multipl_mtx(mcb); /* 再帰ロックをする関数 */
  	}
  }

  /* 初期ロック */
  else {
		workmcb = mg_mtx_info.ceil_pri_head;
		/* 優先度シーリングが存在し，criticalsectionに入れない場合 */
		if ((workmcb != NULL) && (!is_criticalsection(mcb, tskpri, tskid))) {
			DEBUG_OUTMSG("CS NG\n");
			/* mutexを取得しているすべてのタスクの優先度を引き上げる */
			for (; workmcb != NULL; workmcb = workmcb->pcl.pcl_next) {
				/* 優先度継承プロトコルの部分 */
				if (tskpri < (mg_tsk_info.id_table[workmcb->ownerid]->priority)) {
					mz_ichg_pri(workmcb->ownerid, tskpri); /* 優先度の低いタスクの優先度を引き上げる(オーナーとなっているタスク自身) */
				}
			}
			wait_mtx_tsk(mcb, TMO_POL); /* mutex待ちタスクの追加をする関数 */
			return E_OK;
		}
		DEBUG_OUTMSG(" CS OK\n");
		put_ceilpri_mtx(mcb); /* シーリングしたmutexをmg_mtx_infoエリアへつなぐ */
  	loc_first_mtx(tskid, mcb); /* 初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(優先度上限プロトコル))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER unl_ceilmtx_isr(MTXCB *mcb)
{
	int tskid, ownerpri, chgpri;
	
  tskid = current->init.tskid; /* アンロックしてきたタスクのIDを取得 */
  ownerpri = mg_tsk_info.id_table[mcb->ownerid]->init.priority;

	/* スリープTCBは一度にすべて戻してはいけない */
  
  /* ここからアンロック操作 */
  if (mcb->ownerid == tskid) { /* オーナータスクか */
  	/* 再帰ロック解除状態の場合 */
  	if (mcb->locks == 1) {
  		/* 待ちタスクが存在する場合 */
			if (mcb->waithead != NULL) {
				/* オーナーとなっていたタスクの優先度を変更するか */
				if (current->priority != ownerpri) { /* 優先度は変更されたか */
					get_ceilpri_mtx(mcb); /* シーリング優先度となったmutexをmg_mtx_infoエリアから抜き取る */
					/* 
					 * 変更するシーリングタスク優先度(シーリング優先度のオーナーで最も高い優先度)を決定
					 * (必ずしもinit.priority時の優先度になるわけではない)
					 */
					chgpri = get_ceiling_tskpri(tskid);
					mz_ichg_pri(tskid, chgpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				/* 取得情報クリア */
				current->get_info.flags &= ~TASK_GET_MUTEX;
				current->get_info.gobjp = 0;
				putcurrent(); /* オーナータスクをレディーへ */
				put_mtx_waittsk(mcb); /* 待ちタスクをレディーへ */
    		return E_OK;
  		}
  		/* 待ちタスクが存在しなければ，オーナーシップのクリア */
  		else {
				/* オーナーとなっていたタスクの優先度を変更するか */
				if (current->priority != ownerpri) { /* 優先度は変更されたか */
					get_ceilpri_mtx(mcb); /* シーリング優先度となったmutexをmg_mtx_infoエリアから抜き取る */
					/* 
					 * 変更するシーリングタスク優先度(シーリング優先度のオーナーで最も高い優先度)を決定
					 * (必ずしもinit.priority時の優先度になるわけではない)
					 */
					chgpri = get_ceiling_tskpri(tskid);
					mz_ichg_pri(tskid, chgpri); /* オーナーシップタスクの優先度を元に戻す */
				}
				putcurrent(); /* システムコール発行スレッドをレディーへ */
				clear_mtx_ownership(mcb); /* オーナーシップのクリアをする関数 */
  			return E_OK;
  		}
  	}
		return not_lock_first_mtx(mcb); /*初期ロック状態(mxvalueが1)以外の場合の場合の処理*/
	}

  /* オーナータスクでない場合 */
  else {
  	putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("not release mutex semaphoreID for interrput handler\n");
  	return E_ILUSE;
  }
}


/*!
* virtual mutexの初期ロックをする関数
* tskid : 初期ロックするタスクID
*/
static void loc_virtual_mtx(int tskid)
{
	MTXCB *vmcb = mg_mtx_info.virtual_mtx;

	/* 初期ロック可能状態の場合 */
	if (vmcb->mtxvalue == 0) {
   	vmcb->mtxvalue += 1;
   	vmcb->locks += 1;
 		/* オーナー情報のセット */
 		vmcb->ownerid = tskid;
 		current = mg_tsk_info.id_table[tskid];
 		
		/* 取得情報(TCB)のセットはいらない */
 		
 		/* ここではシステムコール発行タスクをレディーへ戻さない */
 	}
	/* 本来は実行されない(実行される時は再入されたなど) */
 	else {
 		down_system();
 	}
  DEBUG_OUTMSG("get virtual mutexID for interrput handler\n");
}


/*!
* virtual mutexの再帰ロックをする関数
* -上限値エラーはないものとする(virtual mutexに再帰ロック上限値はない)
*/
static void loc_multipl_virtual_mtx(void)
{
	/* ここではオーナータスクをレディーへ戻さない */

 	mg_mtx_info.virtual_mtx->locks += 1;
 	DEBUG_OUTMSG("get multipl virtual mutexID for interrput handler\n");
}


/*!
* virtual mutex待ちタスクの追加
* -物理mutexには繋がない
* -待ち情報は物理mutexを記録しておく
* -優先度逆転機構にタイムアウト付きmutex及びポーリングmutexは認めていない
* *mcb : ロックできなかった(記録する)物理mutex
*/
static void wait_virtual_mtx_tsk(MTXCB *mcb)
{
	MTXCB *vmcb = mg_mtx_info.virtual_mtx;

	/* すでに仮想mutex待ちタスクが存在する場合 */
	if (vmcb->waittail) {
		current->wait_info.wait_prev = vmcb->waittail;
		vmcb->waittail->wait_info.wait_next = current; /* システムコールを呼び出したタスクはcurrentに入っている */
 	}
	/* まだ仮想mutex待ちタスクが存在しない場合 */
 	else {
		vmcb->waithead = current;
  }
 	vmcb->waittail = current;
 	
	/* 待ち情報(TCB)をセット(物理mutexを記録しておく) */
 	current->state |= TASK_WAIT_VIRTUAL_MUTEX;
  current->wait_info.wobjp = (WAIT_OBJP)mcb;
  
  DEBUG_OUTMSG("not get virtual mutexID for interrput handler\n");
}


/*! virtual mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理 */
static ER not_lock_first_virtual_mtx(void)
{
	MTXCB *vmcb = mg_mtx_info.virtual_mtx;

	/* ここではシステムコール発行スレッドをレディーへ戻さない */

	/* 仮想mutex再帰ロック解除処理 */
  if (vmcb->locks > 1) {
  	vmcb->locks -= 1;
  	DEBUG_OUTMSG("release multipl virtual mutexID for interrput handler\n");
  	return E_OK;
  }
  /* すでにアンロック状態 */
  else {
  	DEBUG_OUTMSG("virtual mutexID release yet.\n");
  	return E_ILUSE; /* すでに解放されてる.エラー終了 */
	}
}


/*! virtual mutexオーナーシップ解除処理 */
static void clear_virtual_mtx_ownership(void)
{
	MTXCB *vmcb = mg_mtx_info.virtual_mtx;

	/* タスクの取得情報をクリア処理はいらない(物理mutex側で行う) */

	vmcb->mtxvalue -= 1;
  vmcb->ownerid = -1;
  vmcb->locks -= 1;
  DEBUG_OUTMSG("virtual mutexID owner ship clear.\n");
}


/*!
* virtul mutex待ちから，物理mutexを与えタスクをレディーへ戻す関数の分岐
* *mcb : 解除するMTXCB
*/
static void put_virtual_mtx_waittsk(MTXCB *mcb)
{
	MTXCB *vmcb = mg_mtx_info.virtual_mtx;

	/* ロック解除時，待ちタスクをFIFO順でレディーへ入れる */
	if (vmcb->atr == MTX_TA_TFIFO) {
		put_virtual_mtx_fifo_ready(mcb);
	}
	 /* ロック解除時，待ちタスクを優先度順でレディーへ入れる */
	else {
		put_virtual_mtx_pri_ready(mcb);
	}
}


/*!
* virtual mutex待ちタスクにvirtual mutexと物理mutexを与えレディーへ先頭からレディーへ入れる
* -全体で最も早くロック要求をしたタスクがレディーへ入れられる(従来の方法とは異なるので注意)
* ラウンドロビンスケジューリングをしている時は先頭からレディーへ戻す
* *mcb : 解除するMTXCB
*/
static void put_virtual_mtx_fifo_ready(MTXCB *mcb)
{
	MTXCB *vmcb, *workmcb;

	vmcb = mg_mtx_info.virtual_mtx;
	
	/* 先頭のセマフォ待ちタスクをレディーへ */
	current = vmcb->waithead;
	vmcb->waithead = current->wait_info.wait_next;
	vmcb->waithead->wait_info.wait_prev = NULL;
	
	/* currentが最尾の時tailをNULLにしておく */
	if (current->wait_info.wait_next == NULL) {
		vmcb->waittail = NULL;
	}
  current->wait_info.wait_next = current->wait_info.wait_next = NULL;

	vmcb->ownerid = current->init.tskid; /* virtual mutexのオーナーシップ設定 */
  
	/* 物理mutexを与える準備 */
	workmcb = (MTXCB *)current->wait_info.wobjp;
	workmcb->mtxvalue = 0;
	workmcb->locks = 0;

	/* 解放するmutexと与えるmutexが異なる場合，解放するmutexを初期化 */
	if (workmcb != mcb) {
		mcb->mtxvalue = 0;
		mcb->ownerid = -1;
		mcb->locks = 0;
	}

	 /* 待ち情報のクリア(TCBに) */
	current->state &= ~TASK_WAIT_VIRTUAL_MUTEX;
  current->wait_info.wobjp = 0;

	/* 物理mutexを与える(タスクをレディーへはloc_first_mtx()で戻す) */
	loc_first_mtx(current->init.tskid, workmcb);
  
  DEBUG_OUTMSG("virtual mtx wait task ready que for interrupt handler.\n");
}


/*!
* virtual mutex待ちタスクにvirtual mutexと物理mutexを与えレディーへ先頭からレディーへ入れる
* -virtual mutexロック要求をした最も高い優先度のタスクをレディーへ入れられる
* -基本的にvirtual priority inheritanceでは待ちタスクを優先度順に入れる
* *mcb : 解除するMTXCB
*/
static void put_virtual_mtx_pri_ready(MTXCB *mcb)
{
	TCB *worktcb, *maxtcb;
	MTXCB *vmcb, *workmcb;

	vmcb = mg_mtx_info.virtual_mtx;
	worktcb = maxtcb = mg_mtx_info.virtual_mtx->waithead;

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

	get_tsk_virtual_mtx_waitque(vmcb, maxtcb); /* virtual mutex待ちキューからスリープTCBを抜き取る関数 */
		
	/* セマフォ待ちタスクの中で優先度が最高のタスクをレディーへ入れる */
	current = maxtcb;
	current->wait_info.wait_next = current->wait_info.wait_prev = NULL;
	
	/* 物理mutexを与える準備 */
	workmcb = (MTXCB *)current->wait_info.wobjp;
	workmcb->mtxvalue = 0;
	workmcb->locks = 0;

	/* 解放するmutexと与えるmutexが異なる場合，解放するmutexを初期化 */
	if (workmcb != mcb) {
		mcb->mtxvalue = 0;
		mcb->ownerid = -1;
		mcb->locks = 0;
	}

	/* 待ち情報のクリア(TCBに) */
  current->state &= ~TASK_WAIT_VIRTUAL_MUTEX;
  current->wait_info.wobjp = 0;

	vmcb->ownerid = current->init.tskid; /* オーナーシップの設定 */
	/* 物理mutexを与える(タスクをレディーへはloc_first_mtx()で戻す) */
	loc_first_mtx(current->init.tskid, workmcb);
	
  DEBUG_OUTMSG("virtual mtx wait task ready que for interrupt handler.\n");
}


/*!
* virtual mutex待ちキューからスリープTCBを抜き取る関数
* *vmcb : virtual mutexのポインタ
* *maxcb : 抜き取るTCB
*/
void get_tsk_virtual_mtx_waitque(MTXCB *vmcb, TCB *maxtcb)
{
	
	/* 先頭から抜きとる */
	if (maxtcb == vmcb->waithead) {
		vmcb->waithead = maxtcb->wait_info.wait_next;
		vmcb->waithead->wait_info.wait_prev = NULL;
		/* mutex待ちキューにタスが一つの場合 */
		if (maxtcb == vmcb->waittail) {
			vmcb->waittail = NULL;
		}
	}
	/* 最尾から抜き取りる */
	else if (maxtcb == vmcb->waittail) {
		vmcb->waittail = maxtcb->wait_info.wait_prev;
		vmcb->waittail->wait_info.wait_next = NULL;
	}
	/* 中間から抜き取る */
	else {
		maxtcb->wait_info.wait_prev->wait_info.wait_next = maxtcb->wait_info.wait_next;
		maxtcb->wait_info.wait_next->wait_info.wait_prev = maxtcb->wait_info.wait_prev;
	}
}


/*!
* システムコール処理(mz_loc_mtx():mutexロック処理(仮想優先度継承プロトコル))
* -優先度継承プロトコルはタスクがmutexをロックできなかった時に，mutexオーナータスクの優先度をロック要求タスクまで継承する
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER loc_vinhermtx_isr(MTXCB *mcb)
{
  int tskid, tskpri;
  
  tskid = current->init.tskid; /* ロックしてきたタスクのIDを取得 */
  tskpri = current->priority; /* ロックしてきたタスクの優先度を取得 */
  
	/* 仮想mutexがロックされている場合 */
	if (mg_mtx_info.virtual_mtx->ownerid != -1) {
		/* 仮想mutexオーナータスクでない場合 */
		if (mg_mtx_info.virtual_mtx->ownerid != tskid) {
			/* ロック要求タスクの方が優先度が高い場合(優先度を継承させる) */
			if (tskpri < (mg_tsk_info.id_table[mg_mtx_info.virtual_mtx->ownerid]->priority)) {
				mz_ichg_pri(mg_mtx_info.virtual_mtx->ownerid, tskpri); /* 優先度の低いタスクの優先度を引き上げる(オーナーとなっているタスク自身) */
			}
			wait_virtual_mtx_tsk(mcb); /* 仮想mutex待ちタスクの追加をする関数 */
			/* 物理mutexには繋がない */
    	return E_OK;
		}
  	/* 仮想mutexオーナータスクの場合 */
  	else {
			/* 物理mutexがロックされている場合 */
			if (mcb->ownerid != -1) {
  			return loc_multipl_mtx(mcb); /* 物理mutexを再帰ロックをする関数 */
			}
			/* 物理mutexがロックされていない場合 */
			else {
				loc_multipl_virtual_mtx(); /* 仮想mutexを再帰ロックする関数 */
				loc_first_mtx(tskid, mcb); /* 物理mutexを初期ロックをする関数 */
  			return E_OK;
			}
  	}
  }

  /* 仮想mutexがロックされていない場合 */
  else {
		loc_virtual_mtx(tskid); /* 仮想mutexをロック */
  	loc_first_mtx(tskid, mcb); /* 物理mutexを初期ロックをする関数 */
  	return E_OK;
  }
}


/*!
* システムコール処理(mz_unl_mtx():mutexアンロック処理(仮想優先度継承プロトコル))
* *mcb : 対象mutexコントロールブロックへのポインタ
* (返却値)E_OK : 正常終了(mutexロック完了，タスクをmutex待ちにする，
*								再帰ロック完了(loc_multipl_mtx()の返却値))
* (返却値)E_ILUSE : loc_multipl_mtx()の返却値(多重再帰ロック)
*/
ER unl_vinhermtx_isr(MTXCB *mcb)
{
	int tskid, tskpri, ownerpri;
	ER ercd;
	
  tskid = current->init.tskid; /* アンロックしてきたタスクのIDを取得 */
	tskpri = current->priority;
  ownerpri = mg_tsk_info.id_table[mg_mtx_info.virtual_mtx->ownerid]->init.priority;

	/* スリープTCBは一度にすべて戻してはいけない */
  
  /* オーナータスクの場合 */
  if (mg_mtx_info.virtual_mtx->ownerid == tskid) {
  	/* 再帰ロック解除状態の場合 */
  	if (mg_mtx_info.virtual_mtx->locks == 1) {
			/* 物理mutexが再帰ロック解除状態の場合 */
			if (mcb->locks == 1) {
  			/* 待ちタスクが存在する場合 */
				if (mg_mtx_info.virtual_mtx->waithead != NULL) {
					/* オーナーとなっていたタスクの優先度戻す場合 */
					if (tskpri != ownerpri) {/* タスクの優先度は継承されたか */
						mz_ichg_pri(tskid, ownerpri); /* オーナーシップタスクの優先度を元に戻す */
					}
					putcurrent(); /* オーナータスクをレディーへ */
					put_virtual_mtx_waittsk(mcb); /* 仮想mutexと物理mutexを与え，待ちタスクをレディーへ */
    			return E_OK;
  			}
  			/* 待ちタスクが存在しなければ，オーナーシップのクリア */
  			else {
					/* タスクの優先度は継承された場合 */
					if (tskpri != ownerpri) {
						mz_ichg_pri(tskid, ownerpri); /* オーナーシップタスクの優先度を元に戻す */
					}
					putcurrent(); /* システムコール発行スレッドをレディーへ */
					clear_virtual_mtx_ownership(); /* 仮想mutexのオーナーシップをクリアする関数 */
					clear_mtx_ownership(mcb); /* 物理mutexのオーナーシップのクリアをする関数 */
  				return E_OK;
				}
			}
			/* 物理mutexが再帰ロック解除状態でない場合 */
			else {
				return not_lock_first_mtx(mcb); /* 物理mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理 */
			} 
  	}
		/* 仮想mutexが再帰ロック解除状態でない場合 */
		else {
			/* 物理mutexが再帰ロック解除状態でない場合 */
			if (mcb->locks != 1) {
				return not_lock_first_mtx(mcb); /* 物理mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理 */
			}
			/* 物理mutexが再帰ロック解除状態の場合 */
			else {
				putcurrent(); /* システムコール発行タスクをレディーへ */
				ercd = not_lock_first_virtual_mtx(); /* 仮想mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理 */
				clear_mtx_ownership(mcb); /* 物理mutexのオーナーシップのクリアをする関数 */
				return ercd;
			}
		}
	}

  /* オーナータスクでない場合 */
  else {
  	putcurrent(); /* システムコール発行スレッドをレディーへ */
  	DEBUG_OUTMSG("not release mutex semaphoreID for interrput handler\n");
  	return E_ILUSE;
  }
}

