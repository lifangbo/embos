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


/********************************************************************
* 本タスクセットは,セマフォを使用したゼネラル排他制御シナリオ
* (グローバル変数へのアクセス)である	*
* sample_task19~sample_task21は同優先度となる																*
* 使用システムコール																									*
* -mz_acre_tsk() : タスク生成(ID自動割付)システムコール								*
* -mz_sta_tsk() : タスク起動システムコール														*
* -mz_acre_sem() : セマフォ生成(ID自動割付)システムコール							*
* -mz_wai_sem() : セマフォ取得システムコール													*
* -mz_sig_sem() : セマフォ解放システムコール													*
*********************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */

#define MAX_COUNT 2		/* 上限 */

/*
* タスク1，タスク2，タスク3からアクセスされるグローバル
* タスク2とタスク3はprt1，prt2をどちらかアクセス．タスク1はprt1とprt2の両方アクセスする
*/
struct access {
	int prt1;
	int prt2;
} access;


/* タスク3 */
int sample_tsk23_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param, tsk2_param, sem0_param;
	ER ercd;

	/* 同優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk24_main;		/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk24";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

	/* 同優先度タスク生成のパラメータ設定 */
	tsk2_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk2_param.un.acre_tsk.func = sample_tsk25_main;		/* タスク起動番地 */
	tsk2_param.un.acre_tsk.name = "sample_tsk25";				/* タスク名 */
	tsk2_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk2_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk2_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk2_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

	/* semaphoreID0生成のパラメータ設定 */
	sem0_param.un.acre_sem.type = STATIC_SEMAPHORE;		/* semaphore内部データ構造の種類(静的型) */
	sem0_param.un.acre_sem.atr = SEM_TA_TFIFO;				/* 待ちタスクをレディーへ戻す属性(FIFO順) */
	sem0_param.un.acre_sem.semvalue = 1;							/* セマフォ初期値 */
	sem0_param.un.acre_sem.maxvalue = 2;							/* セマフォ最大値 */

  puts("sample_task23 started.\n");

	sem0_id = mz_acre_sem(&sem0_param); /* セマフォ生成のシステムコール */
	/* semaphoreID0が生成できた場合 */
  if (sem0_id > E_NG) {
  	puts("sample_task23 create semaphore (sem0_id).\n");
  }

	puts("sample_task23 create running in (sample_tsk24_id).\n");
	/* 同優先度タスクを休止状態から実行可能状態へ */
  sample_tsk24_id = mz_run_tsk(&tsk1_param); /* タスク起動のシステムコール */
  puts("sample_task23 create running out (sample_tsk24_id).\n");

	puts("sample_task23 create running in (sample_tsk25_id).\n");
	/* 同優先度タスクを休止状態から実行可能状態へ */
  sample_tsk25_id = mz_run_tsk(&tsk1_param); /* タスク起動のシステムコール */
  puts("sample_task23 create running out (sample_tsk25_id).\n");

	while (1) {
		ercd = mz_wai_sem(sem0_id); /* セマフォ取得システムコール */
		/* 取得できた場合 */
		if (ercd == E_OK) {
			puts("sample_task23 wai_sem OK.\n");
		}

		/* 排他制御処理を書く */

		access.prt1++;
		puts("prt1 access.\n");

		puts("sample_task23 release semaphore(sem0_id).\n");
		ercd = mz_sig_sem(sem0_id);

		/* ここは排他処理とはならない */

		/* グローバルアクセス上限数の場合 */
		if (access.prt1 >= MAX_COUNT || access.prt2 >= MAX_COUNT) {
			break;
		}
	}
  
  return 0;
}


/* タスク2 */
int sample_tsk24_main(int argc, char *argv[])
{
	ER ercd;

  puts("sample_task24 started.\n");

	while (1) {
		ercd = mz_wai_sem(sem0_id); /* セマフォ取得システムコール */
		/* 取得できた場合 */
		if (ercd == E_OK) {
			puts("sample_task24 wai_sem OK.\n");
		}

		/* 排他制御処理を書く */
		access.prt2++;
		puts("prt2 access.\n");

		puts("sample_task24 release semaphore (sem0_id).\n");
		ercd = mz_sig_sem(sem0_id);

		/* ここは排他処理とはならない */

		/* グローバルアクセス上限数の場合 */
		if (access.prt1 >= MAX_COUNT || access.prt2 >= MAX_COUNT) {
			break;
		}
	}

  return 0;
}


/* タスク1 */
int sample_tsk25_main(int argc, char *argv[])
{
	ER ercd;

  puts("sample_task25 started.\n");

	while (1) {
		ercd = mz_wai_sem(sem0_id); /* セマフォ取得システムコール */
		/* 取得できた場合 */
		if (ercd == E_OK) {
			puts("sample_tsk25 wai_sem OK.\n");
		}
		ercd = mz_wai_sem(sem0_id); /* セマフォ取得システムコール */
		/* 取得できた場合 */
		if (ercd == E_OK) {
			puts("sample_task25 multipul wai_sem OK.\n");
		}

		/* 排他制御処理を書く */
		access.prt1++;
		access.prt2++;
		puts("prt1 & prt2 access.\n");

		puts("sample_task25 multipul release semaphore(sem0_id).\n");
		ercd = mz_sig_sem(sem0_id);
		puts("sample_task25 release semaphore(sem0_id).\n");
		ercd = mz_sig_sem(sem0_id);

		/* ここは排他処理とはならない */

		/* グローバルアクセス上限数の場合 */
		if (access.prt1 >= MAX_COUNT || access.prt2 >= MAX_COUNT) {
			break;
		}
	}

  return 0;
}

#endif
