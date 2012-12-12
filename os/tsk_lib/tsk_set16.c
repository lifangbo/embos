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


/********************************************************************************
* ロックフリープロトコルを使用した排他制御																					*
* -グローバルな構造体変数はCPU1命令ではできないため，クリティカルセクションへ置く．		*
* -このシナリオはクリティカルセクションにOSの機能を使用せず(OSのオーバーヘッドなし)，	*
* アプリケーション側で制御する方法																									*
* 使用システムコール																															*
* -mz_acre_cyc() : 周期ハンドラ生成(ID自動割付)																		*
* -mz_sta_cyc() : 周期ハンドラ起動																								*
*********************************************************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


/* クリティカルセクション制御に使用する変数 */
int cs = 1;

/*
* 周期ハンドラ～パート1
* 周期ハンドラは非タスクコンテキストで実行されるので，注意する
*/
void cycle_handler2(void *exinf)
{
	int tmp;

	puts("sample_task32 cycle handler2 OK.\n");

	tmp = cs;

	cs = tmp + 2; /* 奇数にする */
  puts("cyc_handler2 global value acces in.\n");

	/* 構造体変数書き込み処理 */

  puts("cyc_handler2 global value acces out.\n");
	cs = tmp + 1; /* 偶数にする */
}


/* タスク1 */
int sample_tsk32_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB cyc0_param;
	ER ercd;
	int tmp;
	
	/* 周期ハンドラID0のパラメータ設定 */
	cyc0_param.un.acre_cyc.type = DYNAMIC_CYCLE_HANDLER;	/* 周期ハンドラ内部データ構造の種類(動的型) */
	cyc0_param.un.acre_cyc.exinf = NULL;									/* 周期ハンドラへ渡す拡張情報 */
	cyc0_param.un.acre_cyc.cyctim = 2000;									/* 周期 */
	cyc0_param.un.acre_cyc.func = cycle_handler2;	/* 周期ハンドラ起動番地 */

  puts("sample_tsk32 started.\n");

	cyc2_id = mz_acre_cyc(&cyc0_param); /* 周期ハンドラ生成のシステムコール */
  /* 周期ハンドラが生成できた場合 */
	if (cyc2_id > 0) {
		puts("sample_task32 create cycle handler(cyc0_id).\n");
	}

  ercd = mz_sta_cyc(cyc2_id); /* 周期ハンドラ起動のシステムコール */
	/* 周期ハンドラが起動できた場合 */
  if (ercd == E_NG) {
  	puts("sample_task32 cycle handler start OK.\n");
  }

	do {
		do {
			tmp = cs;
		} while (cs % 2); /* 偶数なら読み込み */
    puts("sample_task32 global value acces in.\n");

		/* 構造体変数読み込み処理 */

    puts("sample_task32 global value acces out.\n");
	} while(tmp != cs); /* 読み込み中に書き換えが行た場合やり直す */ 

  return 0;
}

#endif
