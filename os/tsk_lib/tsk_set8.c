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
* 本タスクセットは，アラームハンドラを使用したシナリオである					*
* 使用システムコール	*
* -mz_acre_alm() : アラームハンドラ生成			*
* -mz_sta_alm() : アラームハンドラ動作開始	*
* -mz_stp_alm() : アラームハンドラ動作停止	*
* -mz_del_alm() : アラームハンドラ排除			*
*******************************************/

#ifdef TSK_LIBRARY


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


/* アラームハンドラ～パート1 */
static void alarm_handler0(void *exinf)
{
	puts("alarm handler0 OK.\n");
}


/* アラームハンドラ～パート2 */
static void alarm_handler1(void *exinf)
{
	puts("alarm handler1 OK.\n");
}


/* アラームハンドラ～パート3 */
static void alarm_handler2(void *exinf)
{
	puts("alarm handler2 OK.\n");
}


int sample_tsk15_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB alm0_param, alm1_param, alm2_param;
	ER ercd;

	/* アラームハンドラID0のパラメータ設定 */
	alm0_param.un.acre_alm.type = DYNAMIC_ALARM_HANDLER;	/* アラームハンドラ内部データ構造の種類(動的型) */
	alm0_param.un.acre_alm.exinf = NULL;									/* アラームハンドラへ渡す拡張情報 */
	alm0_param.un.acre_alm.func = alarm_handler0;	/* アラームハンドラ起動番地 */

	/* アラームハンドラID1のパラメータ設定 */
	alm1_param.un.acre_alm.type = DYNAMIC_ALARM_HANDLER;	/* アラームハンドラ内部データ構造の種類(動的型) */
	alm1_param.un.acre_alm.exinf = NULL;									/* アラームハンドラへ渡す拡張情報 */
	alm1_param.un.acre_alm.func = alarm_handler1;	/* アラームハンドラ起動番地 */

	/* アラームハンドラID2のパラメータ設定 */
	alm2_param.un.acre_alm.type = DYNAMIC_ALARM_HANDLER;	/* アラームハンドラ内部データ構造の種類(動的型) */
	alm2_param.un.acre_alm.exinf = NULL;									/* アラームハンドラへ渡す拡張情報 */
	alm2_param.un.acre_alm.func = alarm_handler2;	/* アラームハンドラ起動番地 */

	
  puts("sample_tsk15 started.\n");
  
  alm0_id = mz_acre_alm(&alm0_param); /* アラームハンドラ生成のシステムコール */
	/* アラームハンドラID0が生成できた場合 */
  if (alm0_id > E_NG) {
  	puts("sample_tsk15 create alarm handler(alm0_id).\n");
  }
  
  ercd = mz_sta_alm(alm0_id, 2000); /* アラームハンドラ動作開始のシステムコール */
	/* アラームハンドラID0が動作開始できた場合 */
	if (ercd == E_OK) {
		puts("sample_tsk15 start alarm handler(alm0_id).\n");
	}
  
  alm1_id = mz_acre_alm(&alm1_param); /* アラームハンドラ生成のシステムコール */
	/* アラームハンドラID1が生成できた場合 */
  if (alm1_id > E_NG) {
  	puts("sample_tsk15 create alarm handler(alm1_id).\n");
  }
  
  ercd = mz_sta_alm(alm1_id, 2000); /* アラームハンドラ動作開始のシステムコール */
	/* アラームハンドラID1が動作開始できた場合 */
	if (ercd == E_OK) {
		puts("sample_tsk15 start alarm handler(alm1_id).\n");
	}

  ercd = mz_stp_alm(alm1_id); /* アラームハンドラ動作停止のシステムコール */
	/* アラームハンドラID1が動作停止できた場合 */
	if (ercd == E_OK) {
		puts("sample_tsk15 stop alarm handler(alm1_id).\n");
	}
  
  alm2_id = mz_acre_alm(&alm2_param); /* アラームハンドラ生成のシステムコール */
  /* アラームハンドラID2が生成できた場合 */
  if (alm2_id > E_NG) {
  	puts("sample_tsk15 create alarm handler(alm2_id).\n");
  }
  
  ercd = mz_del_alm(alm2_id); /* アラームハンドラ排除のシステムコール */
	/* アラームハンドラID2が排除できた場合 */
	if (ercd == E_OK) {
		puts("sample_task11 delete alarm handler(alm2_id).\n");
	}

	/* 実際に起動されるアラームハンドラは1つとなる */
  
  return 0;
}

#endif
