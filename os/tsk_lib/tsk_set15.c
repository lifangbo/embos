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

/******************************************************
* ディスパッチ禁止許可を使用した排他制御(応用)シナリオ	*
* 使用システムコール																		*
* -mz_acre_tsk() : タスク生成(ID自動割付)システムコール	*
* -mz_sta_tsk() : タスク起動システムコール							*
* -mz_dis_dsp() : ディスパッチ禁止											*
* -mz_ena_dsp() : ディスパッチ許可											*
*******************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk30_main(int argc, char *argv[])
{
  SYSCALL_PARAMCB tsk1_param;

	/* 同優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;				/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk31_main;		/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk31";				/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;							/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;					/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;									/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;								/* タスクへ渡すパラメータ */

	puts("sample_task30 started.\n");

	/* 同優先度タスクを未登録状態から休止状態へ */
	sample_tsk31_id = mz_acre_tsk(&tsk1_param); /* タスク生成のシステムコール */
	/* タスクが生成できた場合 */
	if (sample_tsk31_id > E_NG) {
		puts("sample_task30 create task(sample_tsk31).\n");
	}

	puts("sample_task30 create running in (sample_task31).\n");
	/* 同優先度タスクを休止状態から実行可能状態へ */
  mz_sta_tsk(sample_tsk31_id); /* タスク起動のシステムコール */
  puts("sample_task30 create running out (sample_task31).\n");

	/* ここからマルチタスクOSではなくなる */
	mz_dis_dsp(); /* ディスパッチ禁止システムコール */

	/* ここに排他処理をかける */

	mz_ena_dsp(); /* ディスパッチ許可システムコール */
	/* ここでマルチタスクOSとなる */

	return 0;
}


int sample_tsk31_main(int argc, char *argv[])
{
	puts("sample_tsk31 started.\n");

	/* ここからマルチタスクOSではなくなる */
	mz_dis_dsp(); /* ディスパッチ禁止システムコール */

	/* ここに排他処理をかける */

	mz_ena_dsp(); /* ディスパッチ許可システムコール */
	/* ここでマルチタスクOSとなる */

	return 0;
}

#endif
