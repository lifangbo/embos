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


/**************************************************************************
* 本タスクセットは、連続ブロッキング発生(Priority Inheritance Protocol時)の
* サンプルシナリオである		*
* -待ちタスクをFIFO順，優先度順ともに結果は同じ															*
* -タスクは低優先度タスク→中優先度タスク→高優先度タスクの順に									*
*  順次起動																																	*
* -中優先度のタスクから高優先度のタスクを立ち上げるとこの場合では，低優先度の		*
*  タスクがmutexをすべてリリースした時点で処理が止まる．つまり，プロトコルなし，*
*  Priority Inheritance, Highest Lockerでは続行できるが，Priority Ceiling		*
*  Protocol，Virtual Priority Inheritanceでは続行できないパターンがある								*
* -Priority Ceiling Protocol，Virtual Priority Inheritance以外は						*
*  中優先度のタスクが高優先度のタスクを起動させる
* 使用システムコール
* mz_run_tsk() : タスクの生成と起動システムコール
* mz_acre_mtx() : mutexの生成システムコール
* mz_loc_mtx() : mutexのロックシステムコール
* mz_unl_mtx() : mutexのアンロックシステムコール
***************************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk48_main(int argc, char *argv[])
{
        SYSCALL_PARAMCB /*tsk1_param,*/ tsk2_param, mtx0_param, mtx1_param;

	/* 高優先度タスク生成のパラメータ設定 */
	//tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	//tsk1_param.un.acre_tsk.func = sample_task50_main;		/* タスク起動番地 */
	//tsk1_param.un.acre_tsk.name = "sample_task50";				/* タスク名 */
	//tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	//tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	//tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	//tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */
	
	/* 中優先度タスク生成のパラメータ設定 */
	tsk2_param.un.acre_tsk.type = DYNAMIC_TASK;			/* タスク内部データ構造の種類(動的型) */
	tsk2_param.un.acre_tsk.func = sample_tsk49_main;		/* タスク起動番地 */
	tsk2_param.un.acre_tsk.name = "sample_tsk49";				/* タスク名 */
	tsk2_param.un.acre_tsk.priority = 3;							/* タスク優先度 */
	tsk2_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk2_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk2_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

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

	puts("sample_task48 started.\n");

	mtx0_id = mz_acre_mtx(&mtx0_param); /* mutexID0の生成 */
	
	mtx1_id = mz_acre_mtx(&mtx1_param); /* mutexID1の生成 */

	/* mutexID0が生成できなかった場合 */
	if (mtx0_id > E_NG) {
		puts("sample_task48 create mutex(mtx0_id)\n");
	}

	/* mutexID1が生成できなかった場合 */
	if (mtx1_id > E_NG) {
		puts("sample_task48 create mutex(mtx1_id)\n");
	}
  
	/* 初期ロック */  
  if (E_OK == mz_loc_mtx(mtx0_id)) {
  	puts("sample_task48 get mutex(mtx0_id).\n");
	}

	/* ここに排他処理を書ける */
  
	/*
	* タスクの生成と起動はmz_acre_tsk()とmz_sta_tsk()で行うとタスクがスイッチングするため，
	* 実行結果がわかりずらくなる．
	* なので，生成と起動のmz_acre_tsk()，またはmv_acre_tsk()とmz_sta_tsk()を使用する
	*/
  puts("sample_task48 create running in (sample_task49).\n");
	/* 中優先度タスク生成と起動 */
  sample_tsk49_id = mz_run_tsk(&tsk2_param);
  puts("sample_task48 create running out (sample_task49).\n");

	/* ここに排他処理を書ける */

	/*
	* Priority Ceiling Protocol，virtual priority inheritanceではここで処理が止まる
	* (中優先度のタスクが高優先度のタスクを起動させずにスリープしているから，つまりここで高優先度タスクを起動させる)
	*/
	/* 高優先度タスクの生成と起動 */
	//puts("sample_task48 create running in (sample_task50).\n");
  //sample_tsk50_id = mz_run_tsk(&tsk1_param);
  // puts("sample_task48 create running out (sample_task50).\n");

	/* 初期ロック解除 */
  puts("sample_task48 release mutex(mtx0_id).\n");
  mz_unl_mtx(mtx0_id);

  return 0;
}


int sample_tsk49_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param;
	
	/* 高優先度タスクタスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk50_main;		/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk50";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

  puts("sample_task49 started.\n");
  
	/* 初期ロック */  
  if (E_OK == mz_loc_mtx(mtx1_id)) {
  	puts("sample_task49 get mutex(mtx1_id).\n");
	}

	/* ここに排他処理を書ける */
	
	/* priority ceiling protocol,virrual priority inheritace以外はここからタスクを起動させる */
	/* 高優先度タスク生成と起動 */
	puts("sample_tsk49 create running in (sample_tsk50).\n");
  sample_tsk50_id = mz_run_tsk(&tsk1_param);
  puts("sample_tsk49 create running out (sample_tsk50).\n");
  
	/* ここに排他処理を書ける */
  
  /* 初期ロック解除 */
  puts("sample_task49 release mutex(mtx1_id).\n");
  mz_unl_mtx(mtx1_id);

  return 0;
}


int sample_tsk50_main(int argc, char *argv[])
{

  puts("sample_task50 started.\n");

	/* 初期ロック */  
  if (E_OK == mz_loc_mtx(mtx0_id)) {
  	puts("sample_task50 get mutex(mtx0_id).\n");
	}

	/* ここに排他処理を書ける */

	/* 初期ロック解除 */
  puts("sample_task50 release mutex(mtx0_id).\n");
  mz_unl_mtx(mtx0_id);


	/* 初期ロック */  
  if (E_OK == mz_loc_mtx(mtx1_id)) {
  	puts("sample_task50 get mutex(mtx1_id).\n");
	}

	/* ここに排他処理を書ける */

	/* 初期ロック解除 */
  puts("sample_task50 release mutex(mtx1_id).\n");
  mz_unl_mtx(mtx1_id);
  
  return 0;
}

#endif
