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


/************************************************************************
* sample_tsk4~sample_tsk6は，主にタスク管理システムコール群を使用する				*
* サンプルシナリオ																												*
* 使用システムコール																											*
* -mz_run_tsk() : タスク生成と起動システムコール														*
* -mz_get_pri() : タスク優先度取得システムコール														*
* -mz_chg_pri() : タスク優先度変更システムコール														*
*************************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


/* 低優先度タスク(優先度3で起動される) */
int sample_tsk4_main(int argc, char *argv[])
{
  SYSCALL_PARAMCB tsk5_param;

	/* 中優先度タスク生成のパラメータ設定 */
	tsk5_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk5_param.un.acre_tsk.func = sample_tsk5_main;			/* タスク起動番地 */
	tsk5_param.un.acre_tsk.name = "sample_tsk5";				/* タスク名 */
	tsk5_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk5_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk5_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk5_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */


	puts("sample_tsk4 started.\n");

	/*
	* タスクの生成と起動はmz_acre_tsk()とmz_sta_tsk()で行うとタスクがスイッチングするため，
	* 実行結果がわかりずらくなるので注意する
	*/
	puts("sample_tsk4 create running in (sample_tsk5).\n");
	/* sample_tsk5のタスクを生成と起動をする */
  sample_tsk5_id = mz_run_tsk(&tsk5_param);
	puts("sample_tsk4 create runnning out (sample_tsk5).\n");

  return 0;
}


/* 高優先度タスク(優先度1で起動される) */
int sample_tsk5_main(int argc, char *argv[])
{
	int tskpri;
	ER ercd;

  puts("sample_tsk5 started.\n");

	ercd = mz_get_pri(sample_tsk5_id, &tskpri); /* タスク優先度取得システムコール */
	putxval(ercd, 0);
	puts(" get_pri ercd\n");
	/* 優先度取得できた場合 */
	if (ercd == E_OK) {
		putxval(tskpri, 0);
		puts(" sample_tsk5 task priority(sample_tsk5)\n");
	}

	ercd = mz_chg_pri(sample_tsk5_id, 4); /* タスク優先度変更システムコール */
	putxval(ercd, 0);
	puts(" chg_pri ercd\n");
	/* タスクの優先度変更できた場合 */
	if (ercd == E_OK) {
		puts("sample_tsk5 change priority(sample_tsk5).\n");
	}

  return 0;
}


#endif
