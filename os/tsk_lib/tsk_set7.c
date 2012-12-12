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


/**********************************************************
* 本タスクセットは,周期ハンドラによるポーリングチェックシナリオ
* である
* 使用システムコール
* mz_acre_cyc() : 周期ハンドラ生成システムコール
* mz_sta_cyc()  : 周期ハンドラ起動システムコール
***********************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk14_main(int argc, char *argv[])
{
	ER_ID ercd;
	SYSCALL_PARAMCB cyc1_param;
	int count;

	/* 周期ハンドラID0のパラメータ設定 */
	cyc1_param.un.acre_cyc.type = DYNAMIC_CYCLE_HANDLER;	/* 周期ハンドラ内部データ構造の種類(動的型) */
	cyc1_param.un.acre_cyc.exinf = NULL;									/* 周期ハンドラへ渡す拡張情報 */
	cyc1_param.un.acre_cyc.cyctim = 2000;									/* 周期 */
	cyc1_param.un.acre_cyc.func = cycle_handler1;	/* 周期ハンドラ起動番地 */
	
  puts("sample_tsk14 started.\n");
  
  cyc1_id = mz_acre_cyc(&cyc1_param); /* 周期ハンドラ生成のシステムコール */
  /* 周期ハンドラが生成できた場合 */
	if (cyc1_id > 0) {
		puts("sample_tsk14 create cycle handler1(cyc1_id).\n");
	}

  ercd = mz_sta_cyc(cyc1_id); /* 周期ハンドラ起動のシステムコール */
	/* 周期ハンドラが起動できた場合 */
  if (ercd == E_NG) {
  	puts("sample_tsk14 cycle handler1 start OK.\n");
  }

	/* 無限ループでポーリングチェック(周期ハンドラで定期的に見にいく) */
	while (1) {
		if (mz_slp_tsk == E_OK) {
			/* タスクが起床されるとここへ戻ってくる */
			count++;
			putxval(count, 0);
			puts("event count");
		}
	}

  return 0;
}


static BOOL is_event(void)
{
	/* ここにポーリング処理を書く */
	return TRUE;
}


/*
* 周期ハンドラ～パート1
* 周期ハンドラは非タスクコンテキストで実行されるので注意する
*/
void cycle_handler1(void *exinf)
{
	int data = 0;
	int check_data;

	puts("sample_tsk14 cycle handler1 OK.\n");

	check_data = is_event(); /* イベントの取得 */
	/* イベントが入ったならば，タスク(sample_tsk14)を起床する */
	if (data == 0 && check_data == TRUE) {
		mz_iwup_tsk(sample_tsk14_id); /* 非タスクコンテキスト用タスク起床システムコール */
	}
}

#endif
