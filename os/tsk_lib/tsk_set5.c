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
* 本タスクセットは,カーネルオブジェクト自動解放機能シナリオ	*
* 使用するタスクセットである			 												*
* 使用システムコール群
* mz_acre_mtx() : mutex生成
*****************************************************/


#ifdef TSK_LIBRARY

#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


int sample_tsk11_main(int argc, char *argv[])
{
  SYSCALL_PARAMCB mtx1_param;

	/* mutexID0生成のパラメータ設定 */
	mtx1_param.un.acre_mtx.type = DYNAMIC_MUTEX;		/* mutex内部データ構造の種類(動的型) */
	mtx1_param.un.acre_mtx.atr = MTX_TA_TPRI;				/* 待ちタスクをレディーへ戻す属性(優先度順) */
	mtx1_param.un.acre_mtx.piver_type = TA_VOIDPCL;	/* 優先度逆転機構なし */
	mtx1_param.un.acre_mtx.maxlocks = 1;						/* mutex再帰ロック上限数 */

	puts("sample_tsk11 started.\n");

	mtx1_id = mz_acre_mtx(&mtx1_param); /* mutexID0の生成 */
	/* mutexID0が生成できなかった場合 */
	if (mtx1_id > E_NG) {
		puts("sample_tsk11 create mutex(mtx_id1).\n");
	}

  return 0;
}

#endif
