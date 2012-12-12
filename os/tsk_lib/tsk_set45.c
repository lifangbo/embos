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


/**********************************************************************************
* 優先度スケジューリング環境下で，μITRON型ラウンドロビンスケジューリングを行うシナリオ	*
* -mz_run_tsk() : タスク生成(ID自動割付)と起動システムコール													*
* -mz_acre_cyc() : 周期ハンドラ生成(ID自動割付)システムコール												*
* -mz_sta_cyc() : 周期ハンドラ起動システムコール																			*
***********************************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


#define ROT_PRI 1			/* 優先度回転を行うタスク優先度 */


/*
* 周期ハンドラ～パート1
* 周期ハンドラは非タスクコンテキストで実行されるので，注意する
*/
void cycle_handler3(void *exinf)
{
	ER_ID ercd;

	puts("sample_tsk111 ~ sample_tsk113 cycle handler1 OK.\n");

	ercd = mv_rot_rdq(ROT_PRI); /* 非タスクコンテキスト用タスク回転システムコール */
	/* タスク回転できた場合 */
	if (ercd == E_OK) {
		puts("cycle handler3 rot dtq OK.\n");
	}
}


/* タスク2 */
int sample_tsk111_main(int argc, char *argv[])
{
	ER_ID ercd;
	SYSCALL_PARAMCB tsk1_param, tsk2_param, cyc0_param;

	/* 同優先度タスク生成のパラメータ設定 */
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;						/* タスク内部データ構造の種類(動的型) */
	tsk1_param.un.acre_tsk.func = sample_tsk113_main;				/* タスク起動番地 */
	tsk1_param.un.acre_tsk.name = "sample_tsk113";						/* タスク名 */
	tsk1_param.un.acre_tsk.priority = 1;									/* タスク優先度 */
	tsk1_param.un.acre_tsk.stacksize = 0x100;							/* タスクスタックサイズ */
	tsk1_param.un.acre_tsk.argc = 0;											/* タスクへ渡すパラメータ */
	tsk1_param.un.acre_tsk.argv = NULL;										/* タスクへ渡すパラメータ */

	/* 低優先度タスク生成のパラメータ設定 */
	tsk2_param.un.acre_tsk.type = DYNAMIC_TASK;						/* タスク内部データ構造の種類(動的型) */
	tsk2_param.un.acre_tsk.func = sample_tsk112_main;				/* タスク起動番地 */
	tsk2_param.un.acre_tsk.name = "sample_tsk112";						/* タスク名 */
	tsk2_param.un.acre_tsk.priority = 3;									/* タスク優先度 */
	tsk2_param.un.acre_tsk.stacksize = 0x100;							/* タスクスタックサイズ */
	tsk2_param.un.acre_tsk.argc = 0;											/* タスクへ渡すパラメータ */
	tsk2_param.un.acre_tsk.argv = NULL;										/* タスクへ渡すパラメータ */


	/* 周期ハンドラID0のパラメータ設定 */
	cyc0_param.un.acre_cyc.type = DYNAMIC_CYCLE_HANDLER;	/* 周期ハンドラ内部データ構造の種類(動的型) */
	cyc0_param.un.acre_cyc.exinf = NULL;									/* 周期ハンドラへ渡す拡張情報 */
	cyc0_param.un.acre_cyc.cyctim = 2000;									/* 周期 */
	cyc0_param.un.acre_cyc.func = cycle_handler3;	/* 周期ハンドラ起動番地 */
	
  puts("sample_tsk111 started.\n");

	cyc3_id = mz_acre_cyc(&cyc0_param); /* 周期ハンドラ生成のシステムコール */
  /* 周期ハンドラが生成できた場合 */
	if (cyc3_id > 0) {
		puts("sample_tsk111 create cycle handler.\n");
	}

	puts("sample_tsk111 create running in (sample_tsk112).\n");
	/* 低優先度タスクの起動 */
  sample_tsk112_id = mz_run_tsk(&tsk2_param);
  puts("sample_tsk111 create running out (sample_tsk112).\n");

	puts("sample_tsk111 create running in (sample_tsk113).\n");
	/* 同優先度タスクの起動 */
  sample_tsk113_id = mz_run_tsk(&tsk1_param);
  puts("sample_tsk111 create running out (sample_tsk113).\n");

  ercd = mz_sta_cyc(cyc3_id); /* 周期ハンドラ起動のシステムコール */
	/* 周期ハンドラが起動できた場合 */
  if (ercd == E_NG) {
  	puts("sample_tsk111 cycle handler start OK.\n");
  }

	/*
	* μITRON型スケジューリングを確認するために，無限ループをさせる
	* タスクを終了させる時は，exitコマンドを使用する
	*/
	while (1) {
		puts("sample_tsk111 roop.\n");
	}

  return 0;
}


int sample_tsk112_main(int argc, char *argv[])
{
	/*
	* μITRON型スケジューリングを確認するために，無限ループをさせる
	* タスクを終了させる時は，exitコマンドを使用する
	*/
	while (1) {
		puts("sample_tsk112 roop.\n");
	}
}


int sample_tsk113_main(int argc, char *argv[])
{
	/*
	* μITRON型スケジューリングを確認するために，無限ループをさせる
	* タスクを終了させる時は，exitコマンドを使用する
	*/
	while (1) {
		puts("sample_tsk113 roop.\n");
	}
}


#endif
