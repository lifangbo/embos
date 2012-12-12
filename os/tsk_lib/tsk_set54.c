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


/******************************************
* 	*
*******************************************/


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


/* 周期ハンドラ1つでLEDの点灯・消灯*/
void cycle_handler4(void *exinf)
{
	unsigned char x;
	x = (*(volatile unsigned char *)0xffffd3);

	puts("sample_tsk138 cycle handler4 OK.\n");

	/* P4DDR */
	(* (volatile unsigned char *)0xfee003) = 0xc0;

	/* LED点灯・消灯判断条件文(0なら点灯) */
	if((x & 0x40) == 0) {
		/* 0xffffd3(P4DR)を番地にキャストして、そこへアクセスする */
		(*(volatile unsigned char *)0xffffd3) = 0x40;
	}
	else {
		(*(volatile unsigned char *)0xffffd3) = 0x00;
	}
	
}


/* 周期ハンドラ1つでLEDの点灯・消灯*/
void cycle_handler5(void *exinf)
{
	unsigned char x;
	x = (*(volatile unsigned char *)0xffffd3);

	puts("sample_tsk138 cycle handler5 OK.\n");

	/* P4DDR */
	(* (volatile unsigned char *)0xfee003) = 0xc0;

	/* LED点灯・消灯判断条件文(0なら点灯) */
	if((x & 0x80) == 0) {
		/* 0xffffd3(P4DR)を番地にキャストして、そこへアクセスする */
		(*(volatile unsigned char *)0xffffd3) = 0x80;
	}
	else {
		(*(volatile unsigned char *)0xffffd3) = 0x00;
	}
	
}


int sample_tsk138_main(int argc, char *argv[])
{
	ER_ID ercd;
	SYSCALL_PARAMCB cyc0_param, cyc1_param;

	/* 周期ハンドラID0のパラメータ設定 */
	cyc0_param.un.acre_cyc.type = DYNAMIC_CYCLE_HANDLER;	/* 周期ハンドラ内部データ構造の種類(動的型) */
	cyc0_param.un.acre_cyc.exinf = NULL;									/* 周期ハンドラへ渡す拡張情報 */
	cyc0_param.un.acre_cyc.cyctim = 1000;									/* 周期 */
	cyc0_param.un.acre_cyc.func = cycle_handler4;	/* 周期ハンドラ起動番地 */

	/* 周期ハンドラID0のパラメータ設定 */
	cyc1_param.un.acre_cyc.type = DYNAMIC_CYCLE_HANDLER;	/* 周期ハンドラ内部データ構造の種類(動的型) */
	cyc1_param.un.acre_cyc.exinf = NULL;									/* 周期ハンドラへ渡す拡張情報 */
	cyc1_param.un.acre_cyc.cyctim = 2000;									/* 周期 */
	cyc1_param.un.acre_cyc.func = cycle_handler5;	/* 周期ハンドラ起動番地 */	

  puts("sample_tsk138 started.\n");

  cyc4_id = mz_acre_cyc(&cyc0_param); /* 周期ハンドラ生成のシステムコール */
  /* 周期ハンドラが生成できた場合 */
	if (cyc4_id > 0) {
		puts("sample_tsk138 create cycle handler4.\n");
	}

	cyc5_id = mz_acre_cyc(&cyc1_param); /* 周期ハンドラ生成のシステムコール */
  /* 周期ハンドラが生成できた場合 */
	if (cyc5_id > 0) {
		puts("sample_tsk138 create cycle handler5.\n");
	}

  ercd = mz_sta_cyc(cyc4_id); /* 周期ハンドラ起動のシステムコール */
	/* 周期ハンドラが起動できた場合 */
  if (ercd == E_NG) {
  	puts("sample_tsk138 cycle handler4 start OK.\n");
  }

	ercd = mz_sta_cyc(cyc5_id); /* 周期ハンドラ起動のシステムコール */
	/* 周期ハンドラが起動できた場合 */
  if (ercd == E_NG) {
  	puts("sample_tsk138 cycle handler5 start OK.\n");
  }

  return 0;
}
