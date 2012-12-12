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


/************************************************************
* 本タスクセットは,ポーリング付きmutexを使用したデッドロックの回避である							*
* (このデッドロックの回避は本質的な回避とは言えない)						*
* 使用システムコール																					*
* -mz_run_tsk() : タスク生成(ID自動割付)と起動システムコール		*
* -mz_acre_mtx() : mutex生成(ID自動割付)システムコール				*
* -mz_ploc_mtx() : mutexロックシステムコール									*
* -mz_unl_mtx() : mutexアンロックシステムコール								*
*************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


/* タスク2 */
int sample_tsk35_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param, mtx0_param, mtx1_param;
	ER ercd1, ercd2;
	
	/* 高優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk36_main;		/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk36";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

	/* mutexID0生成のパラメータ設定 */
	mtx0_param.un.acre_mtx.type = DYNAMIC_MUTEX;			/* mutex内部データ構造の種類(動的型) */
	mtx0_param.un.acre_mtx.atr = MTX_TA_TFIFO;				/* 待ちタスクをレディーへ戻す属性(FIFO順) */
	mtx0_param.un.acre_mtx.piver_type = TA_VOIDPCL;		/* 優先度逆転機構(優先度継承プロトコル) */
	mtx0_param.un.acre_mtx.maxlocks = 1;							/* mutex再帰ロック上限数 */

	/* mutexID1生成のパラメータ設定 */
	mtx1_param.un.acre_mtx.type = DYNAMIC_MUTEX;			/* mutex内部データ構造の種類(動的型) */
	mtx1_param.un.acre_mtx.atr = MTX_TA_TFIFO;				/* 待ちタスクをレディーへ戻す属性(FIFO順) */
	mtx1_param.un.acre_mtx.piver_type = TA_VOIDPCL;		/* 優先度逆転機構(優先度継承プロトコル) */
	mtx1_param.un.acre_mtx.maxlocks = 1;							/* mutex再帰ロック上限数 */

  puts("sample_tsk35 started.\n");

  mtx0_id = mz_acre_mtx(&mtx0_param); /* mutexID0生成のシステムコール */
	/* mutexID0が生成できた場合 */
  if (mtx0_id > E_NG) {
  	puts("sample_tsk35 create mutex(mtx0_id).\n");
  }
  
	/* mutexID1が生成できた場合 */
  mtx1_id = mz_acre_mtx(&mtx1_param); /* mutexID1生成のシステムコール */
  if (mtx1_id > E_NG) {
  	puts("sample_tsk35 create mutex(mtx1_id).\n");
  }

	ercd1 = mz_ploc_mtx(mtx0_id); /* mutexポーリングロックシステムコール */
	/* mutexID0はロックできた場合 */
	if (ercd1 == E_OK) {
		puts("sample_tsk35 loc mutex(mtx0_id)\n");
	}

	ercd2 = mz_ploc_mtx(mtx1_id); /* mutexポーリングアンロックシステムコール */
	/* mutexID1はロックできた場合 */
	if (ercd2 == E_OK) {
		puts("sample_tsk35 loc mutex(mtx1_id)\n");
	}

	puts("sample_tsk35 create running in (sample_tsk36).\n");
	/* 高優先度タスクの起動 */
  sample_tsk36_id = mz_run_tsk(&tsk1_param);
  puts("sample_tsk35 create running out (sample_tsk36).\n");

	/* ploc_mtx()でmutexID0をロックしていたならば，アンロック */
	if (ercd1 == E_OK) {
		puts("sample_tsk35 unlock mutex(mtx0_id)\n");
		mz_unl_mtx(mtx0_id);
	}
  
	/* ploc_mtx()でmutexID1をロックしていたならば，アンロック */
	if (ercd2 == E_OK) {
		puts("sample_tsk35 unlock mutex(mtx1_id)\n");
		mz_unl_mtx(mtx1_id);
	}

  return 0;
}


/* タスク1 */
int sample_tsk36_main(int argc, char *argv[])
{
	ER ercd1, ercd2;

	ercd1 = mz_ploc_mtx(mtx0_id); /* mutexポーリングロックシステムコール */
	/* mutexID0はロックできた場合 */
	if (ercd1 == E_OK) {
		puts("sample_tsk36 loc mutex(mtx0_id)\n");
	}

	ercd2 = mz_ploc_mtx(mtx1_id); /* mutexポーリングアンロックシステムコール */
	/* mutexID1はロックできた場合 */
	if (ercd2 == E_OK) {
		puts("sample_tsk36 loc mutex(mtx1_id)\n");
	}

	/* ploc_mtx()でmutexID0をロックしていたならば，アンロック */
	if (ercd1 == E_OK) {
		puts("sample_tsk36 unlock mutex(mtx0_id)\n");
		mz_unl_mtx(mtx0_id);
	}
  
	/* ploc_mtx()でmutexID1をロックしていたならば，アンロック */
	if (ercd2 == E_OK) {
		puts("sample_tsk36 unlock mutex(mtx1_id)\n");
		mz_unl_mtx(mtx1_id);
	}

  return 0;
}

#endif
