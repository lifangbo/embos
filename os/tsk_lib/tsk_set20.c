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


/********************************************************************************************
* 本タスクセットは,優先度逆転現象が起こるシナリオである																					*
* -優先度逆転機構を設定しなければ，待ちタスクをレディーへFIFO順
* または優先度順で戻しても結果は同じである
* 使用システムコール
* mz_run_tsk() : タスクの生成と起動システムコール
* mz_acre_mtx() : mutexの生成システムコール
* mz_loc_mtx() : mutexのロックシステムコール
* mz_unl_mtx() : mutexのアンロックシステムコール
*********************************************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk39_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param, tsk2_param, mtx0_param;
	
	/* 高優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk41_main;			/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk41";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */
	
	/* 中優先度タスク生成のパラメータ設定 */
	tsk2_param.un.acre_tsk.type = DYNAMIC_TASK;			/* タスク内部データ構造の種類(動的型) */
	tsk2_param.un.acre_tsk.func = sample_tsk40_main;			/* タスク起動番地 */
	tsk2_param.un.acre_tsk.name = "sample_tsk40";				/* タスク名 */
	tsk2_param.un.acre_tsk.priority = 3;							/* タスク優先度 */
	tsk2_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk2_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk2_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

	/* mutexID0生成のパラメータ設定 */
	mtx0_param.un.acre_mtx.type = DYNAMIC_MUTEX;			/* mutex内部データ構造の種類(動的型) */
	mtx0_param.un.acre_mtx.atr = MTX_TA_TPRI;					/* 待ちタスクをレディーへ戻す属性(優先度順) */
	mtx0_param.un.acre_mtx.piver_type = TA_VOIDPCL;		/* 優先度逆転機構(なし) */
	mtx0_param.un.acre_mtx.maxlocks = 1;							/* mutex再帰ロック上限数 */
	
  puts("sample_tsk39 started.\n");

	mtx0_id = mz_acre_mtx(&mtx0_param);	/* mutexID1の生成 */

	/* mutexID1が生成できなかった場合 */
	if (mtx0_id > E_NG) {
		puts("sample_tsk39 create mutex(mtx0_id)\n");
	}
  
  /*初期ロック*/
  if (E_OK == mz_loc_mtx(mtx0_id)) {
  	puts("sample_tsk39 get dynamic mutex(mtx_id0).\n");
	}

	/*ここに排他処理を書ける*/
  
	/*
	* タスクの生成と起動はmz_acre_tsk()とmz_sta_tsk()で行うとタスクがスイッチングするため，
	* 実行結果がわかりずらくなる．
	* なので，生成と起動のmz_acre_tsk()，またはmv_acre_tsk()とmz_sta_tsk()を使用する
	*/
	puts("sample_tsk39 create running in (sample_tsk41).\n");
	/* sample_task35のタスクを生成と起動をする */
  sample_tsk41_id = mz_run_tsk(&tsk1_param);
	puts("sample_tsk39 create runnning out (sample_tsk41).\n");

	/*ここに排他処理を書ける*/

	puts("sample_tsk39 create running in (sample_tsk40).\n");
	/* sample_task36のタスクを生成と起動をする */
  sample_tsk40_id = mz_run_tsk(&tsk2_param);
	puts("sample_tsk39 create running out (sample_tsk40).\n");

	/*ここに排他処理を書ける*/

  /*初期ロック解除*/
	puts("sample_tsk39 release dynamic mutex(mtx0_id).\n");
  mz_unl_mtx(mtx0_id);
  
  return 0;
}


int sample_tsk40_main(int argc, char *argv[])
{

  puts("sample_tsk40 started.\n");
  
  /* ここで中位のタスク(sample_task36_main)が先行し，優先度逆転 */
  
  puts("> ");
	/* 無限ループでタイミングアノマリを発生させる */
  while (1) {
    ;
  }
  
  return 0;
}


int sample_tsk41_main(int argc, char *argv[])
{

  puts("sample_tsk41 started.\n");
  
  /* プロトコルなしだとここで優先度逆転 */

	/* 初期ロック */
	if (E_OK == mz_loc_mtx(mtx0_id)) {
  	puts("sample_tsk41 get dynamic mutex(mtx0_id).\n");
	}

	/*ここに排他処理を書ける*/
  
  /* 初期ロック解除 */
	puts("sample_tsk41 release dynamic mutex(mtx0_id).\n");
  mz_unl_mtx(mtx0_id);

  return 0;
}

#endif
