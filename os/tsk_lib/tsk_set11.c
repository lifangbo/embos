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
* 本タスクセットは,セマフォを使用した排他制御シナリオ
* (グローバル変数へのアクセス)である	*
* sample_tsk21~sample_tsk22は同優先度となる												*
* 使用システムコール																					*
* -mz_acre_tsk() : タスク生成(ID自動割付)システムコール				*
* -mz_sta_tsk() : タスク起動システムコール										*
* -mz_acre_sem() : セマフォ生成(ID自動割付)システムコール			*
* -mz_wai_sem() : セマフォ取得システムコール									*
* -mz_sig_sem() : セマフォ解放システムコール									*
* -mz_rot_dtq() : タスク優先度回転システムコール								*
*************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


#define MAX_COUNT 5		/* 最大アクセス回数 */
int count;						/* タスク1とタスク2からアクセスされるグローバル */


/* タスク2 */
int sample_tsk21_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param, sem0_param;
	ER ercd;

	/* 同優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk22_main;		/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk22";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

	/* semaphoreID0生成のパラメータ設定 */
	sem0_param.un.acre_sem.type = STATIC_SEMAPHORE;		/* semaphore内部データ構造の種類(静的型) */
	sem0_param.un.acre_sem.atr = SEM_TA_TFIFO;				/* 待ちタスクをレディーへ戻す属性(FIFO順) */
	sem0_param.un.acre_sem.semvalue = 1;							/* セマフォ初期値 */
	sem0_param.un.acre_sem.maxvalue = 1;							/* セマフォ最大値 */

  puts("sample_tsk21 started.\n");

	/* 同優先度タスクを未登録状態から休止状態へ */
	sample_tsk22_id = mz_acre_tsk(&tsk1_param); /* タスク生成のシステムコール */
	/* タスクが生成できた場合 */
	if (sample_tsk22_id > E_NG) {
		puts("sample_tsk21 create task(sample_tsk22).\n");
	}

	sem0_id = mz_acre_sem(&sem0_param); /* セマフォ生成のシステムコール */
	/* semaphoreID0が生成できた場合 */
  if (sem0_id > E_NG) {
  	puts("sample_task21 create semaphore(sem0_id).\n");
  }

	puts("sample_task21 create running in (sample_tsk22).\n");
	/* 同優先度タスクを休止状態から実行可能状態へ */
  mz_sta_tsk(sample_tsk22_id); /* タスク起動のシステムコール */
  puts("sample_tsk21 create running out (sample_tsk22).\n");

	while (1) {
		ercd = mz_wai_sem(sem0_id); /* セマフォ取得システムコール */
		/* 取得できた場合 */
		if (ercd == E_OK) {
			puts("sample_task21 wai_sem OK.\n");
		}

		/* 排他制御処理を書く */

		count++;
		putxval(count, 0);
		puts(" access num.\n");

		puts("sample_task21 release semaphore(sem0_id).\n");
		ercd = mz_sig_sem(sem0_id);

		/* ここは排他処理とはならない */

		/* グローバルアクセス上限数の場合 */
		if (count == MAX_COUNT) {
			break;
		}

		mz_rot_rdq(1); /* タスク優先度回転システムコール */
	}

  /* sample_tsk22をここで強制終了させないと,MAX_COUNTを越えてしまう(またはsample_tsk22のbreak条件を<=にする) */
  mz_ter_tsk(sample_tsk22_id); /* タスク強制終了システムコール */
  
  return 0;
}


/* タスク1 */
int sample_tsk22_main(int argc, char *argv[])
{
	ER ercd;

  puts("sample_tsk22 started.\n");

	while (1) {
		ercd = mz_wai_sem(sem0_id); /* セマフォ取得システムコール */
		/* 取得できた場合 */
		if (ercd == E_OK) {
			puts("sample_tsk22 wai_sem OK.\n");
		}

		/* 排他制御処理を書く */

		count++;
		putxval(count, 0);
		puts(" access num.\n");

		puts("sample_tsk22 release semaphore(sem0_id).\n");
		ercd = mz_sig_sem(sem0_id);

		/* ここは排他処理とはならない */

		/* グローバルアクセス上限数の場合 */
		if (count == MAX_COUNT) {
			break;
		}

		mz_rot_rdq(1); /* タスク優先度回転システムコール */
	}
  
  return 0;
}

#endif
