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


#include "kernel/kernel.h"
#include "log_manage.h"
#include "c_lib/lib.h"


#define LOG_BUFFER_MAX 			4096	/* ログバッファの最大 */
#define CONTEXT_LOG_ONESIZE 92		/* コンテキストログ1つあたりの大きさ */


extern UINT32 logbuffer_start; /* ログ格納専用メモリセグメント */
UINT32 *logbuf;
UINT32 *log_startp;


/*! ログ管理機構の初期化 */
void log_mechanism_init(void)
{
	logbuf = log_startp = &logbuffer_start;
	log_counter = 0;
	*log_startp = 0;
	
	logbuf++;
}


/*!
 * コンテキストスイッチングのログの出力(Linux jsonログを参考(フォーマット変換は行わない))
 * log_tcb : コンテキストスイッチング対象TCB
 */
void get_log(OBJP log_tcb)
{
	static UINT32 tmp = 0;
	TCB *work = (TCB *)log_tcb;
	UINT32 tskid, priority, state;

	/* 実行可能タスクが決定していない時(init_tsk生成等で呼ばれる) */
	if (work == NULL) {
		return;
	}
	/* ログバッファがオーバーフローした場合 */
	else if (LOG_BUFFER_MAX < (*log_startp + CONTEXT_LOG_ONESIZE)) {
		down_system(); /* kernelのフリーズ */
	}
	else {
		/* 処理なし */
	}

	tskid = (UINT32)work->init.tskid;
	priority = (UINT32)work->priority;
	state = (UINT32)work->state;
	
	/*
	 * prevのログをメモリセグメントへ
	 * ・prevログ
	 *  システムコールを発行したタスクの次(ISR適用後(currentが切り替わるものは，適用前のログとなる))の状態
	 */
	if (log_counter == tmp) {
		memset((char *)logbuf, 0, TASK_NAME_SIZE * 4);
		strcpy((char *)logbuf, work->init.name);
		logbuf += TASK_NAME_SIZE;
		*logbuf = tskid;
		logbuf++;
		//*logbuf = secs; //時間
		//logbuf++;
		*logbuf = tskid;
		logbuf++;
		*logbuf = priority;
		logbuf++;
		*logbuf = state;
		logbuf++;
		tmp = log_counter;
		log_counter++;
	}
	/*
	 * nextのログをメモリセグメントへ
	 * ・nextログ
	 *  次にディスパッチされるタスクの状態
	 */
	else {
		*logbuf = tskid;
		logbuf++;
		*logbuf = priority;
		logbuf++;
		*logbuf = state;
		logbuf++;
		tmp = log_counter;
		*log_startp += CONTEXT_LOG_ONESIZE; /* ログサイズの合計 */
	}
}
