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


/****************************************************************************
* 本タスクセットは，主にタスク付属同期システムコール(tslp_tsk()，dly_tsk())			*
* 使用したタスクセットである																									*
* 使用システムコール																													*
* -mz_acre_tsk()	: タスク生成																								*
* -mz_sta_tsk()		: タスク起動																								*
* -mz_ter_tsk()		: タスクの強制終了
* -mz_tslp_tsk() 	: 自タスクタイムアウト付き起床待ちシステムコール															*
* -mz_dly_tsk() 	: 自タスクの遅延													 *
*****************************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk9_main(int argc, char *argv[])
{
  SYSCALL_PARAMCB tsk10_param;

	/* 高優先度タスク生成のパラメータ設定 */
	tsk10_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk10_param.un.acre_tsk.func = sample_tsk10_main;			/* タスク起動番地 */
	tsk10_param.un.acre_tsk.name = "sample_tsk10";				/* タスク名 */
	tsk10_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk10_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk10_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk10_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */


	puts("sample_tsk9 started.\n");

	/* 中優先度タスク(sample_tsk7)をタスクを未登録状態から休止状態へ */
	sample_tsk10_id = mz_acre_tsk(&tsk10_param); /* タスク生成のシステムコール */
	/* 生成できた場合 */
	if (sample_tsk10_id > E_NG) {
		puts("sample_tsk9 create tsk(sample_tsk10).\n");
	}

	puts("sample_tsk9 running in (sample_tsk10).\n");
	/* 中優先度タスク(sample_tsk7)を休止状態から実行可能状態へ(中優先度タスクに処理が移る) */
  mz_sta_tsk(sample_tsk10_id); /* タスク起動のシステムコール */
  puts("sample_tsk9 running out (sample_tsk10).\n");

	puts(" ...waiting.\n");

	while(1) {
		;
	}

  return 0;
}


int sample_tsk10_main(int argc, char *argv[])
{
	ER ercd;

  puts("sample_tsk10 started.\n");

	ercd = mz_tslp_tsk(3000); /* 自タスクのタイムアウト付き起床待ちシステムコール */ 
	/* タイムアウトした場合 */
	if (ercd == E_TMOUT) {
		puts("sample_tsk10 time out.\n");
	}

	ercd = mz_dly_tsk(5000); /* 自タスクの遅延システムコール */
	/* 時間経過した場合 */
	if (ercd == E_OK) {
		puts("sample_tsk10 delay task.\n");
	}

	mz_ter_tsk(sample_tsk9_id);

  return 0;
}


#endif
