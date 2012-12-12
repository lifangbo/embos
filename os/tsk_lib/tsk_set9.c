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


/********************************************************
* 本タスクセットは,セマフォを使用したタスク同期シナリオである											*
* 使用システムコール																			*
* -mz_acre_tsk() : タスク生成(ID自動割付)システムコール		*
* -mz_sta_tsk() : タスク起動システムコール								*
* -mz_acre_sem() : セマフォ生成(ID自動割付)システムコール	*
* -mz_wai_sem() : セマフォ取得システムコール							*
* -mz_sig_sem() : セマフォ解放システムコール							*
*********************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk16_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param, sem0_param;
	ER ercd;

	/* 低優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk17_main;		/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk17";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 5;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */
	
	/* semaphoreID0生成のパラメータ設定 */
	sem0_param.un.acre_sem.type = STATIC_SEMAPHORE;		/* semaphore内部データ構造の種類(静的型) */
	sem0_param.un.acre_sem.atr = SEM_TA_TFIFO;				/* 待ちタスクをレディーへ戻す属性(FIFO順) */
	sem0_param.un.acre_sem.semvalue = 0;							/* セマフォ初期値 */
	sem0_param.un.acre_sem.maxvalue = 1;							/* セマフォ最大値 */

  puts("sample_tsk16 started.\n");

	/* 低優先度タスクを未登録状態から休止状態へ */
	sample_tsk17_id = mz_acre_tsk(&tsk1_param); /* タスク生成のシステムコール */
	/* タスクが生成できた場合 */
	if (sample_tsk17_id > E_NG) {
		puts("sample_tsk16 create task(sample_tsk17).\n");
	}

	sem0_id = mz_acre_sem(&sem0_param); /* セマフォ生成のシステムコール */
	/* semaphoreID0が生成できた場合 */
  if (sem0_id > E_NG) {
  	puts("sample_tsk16 create semaphore(sem0_id).\n");
  }

	puts("sample_tsk16 create running in (sample_tsk17).\n");
	/* 低優先度タスクを休止状態から実行可能状態へ(低優先度タスクに処理が移る) */
  ercd = mz_sta_tsk(sample_tsk17_id); /* タスク起動のシステムコール */
  puts("sample_task16 create running out (sample_tsk17).\n");

	/* 何か処理を書く */
  
	ercd = mz_wai_sem(sem0_id); /* セマフォ取得要求システムコール */
	/* セマフォID0が取得できた場合 */
	if (ercd == E_OK) {
		puts("sample_tsk16 wait semaphore(sem0_id).\n");
	}

	/* ここで同期 */

	/* 何か処理を書く */
  
  return 0;
}


int sample_tsk17_main(int argc, char *argv[])
{
  puts("sample_tsk17 started.\n");

	/* 何か処理を書く */
  
	puts("sample_tsk17 release semaphore(sem0_id).\n");
  mz_sig_sem(sem0_id); /* セマフォ解放要求システムコール */

	/* タスク同期(高優先度タスクに処理が移る) */

	/* 何か処理を書く */
  
  return 0;
}

#endif
