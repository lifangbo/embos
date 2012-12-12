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


/*********************************************
* 本タスクセットは,同一タスクセットに対して異なるスケジューリングを
* 適用した結果である
* 使用システムコール
* mz_acre_tsk() : タスクの生成システムコール
* mz_sta_tsk() : タスクの起動システムコール
* mz_get_id() : タスクID取得システムコール
* mz_rol_sys() : システムロールバックシステムコール
* 使用サービスコール
* mv_sel_schdul : スケジューラ切替えシステムコール
*************************************************************/


#include "kernel/defines.h"		/* このosの型の定義 */
#include "kernel/kernel.h"		/* システムコール及びユーザタスクのシステムコールの定義 */
#include "c_lib/lib.h"				/* 標準ライブラリの定義 */


static int i;

int sample_tsk109_main(int argc, char *argv[])
{
	SYSCALL_PARAMCB tsk1_param;
	
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk110_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk110";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.deadtim = 2000;
	tsk1_param.un.acre_tsk.floatim = 10;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;
	
  puts("sampel_tsk109 started.\n");
  
  sample_tsk110_id = mz_acre_tsk(&tsk1_param);
  mz_sta_tsk(sample_tsk110_id);
  
  mz_get_tid();
  
  i++;
  switch(i) {
  	case 1:
  		mv_sel_schdul(FCFS_SCHEDULING, 0);
			/* NMIからロールバックする場合 */
			while (1) {
				;
			}
			mz_rol_sys(TA_EXECHG);
  		break;
  	/*case 2:
  		mz_sel_schdul(RR_SCHEDULING, 1000);
  		break;
  	case 3: 
  		mz_sel_schdul(EDF_SCHEDULING, 0);
  		break;
  	case 4:
  		mz_sel_schdul(LLF_SCHEDULING, 0);
  		break;*/
  }
  
  return 0;
}


int sample_tsk110_main(int argc, char *argv[])
{
	
  puts("sample_tsk110 started.\n");
  
  mz_get_tid();
  
  return 0;
}

