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


#ifndef _TIMER_CALLRTE_H_INCLUDE_
#define _TIMER_CALLRTE_H_INCLUDE_


/*! schedule_rr()のコールバックルーチン */
void schedule_rr_callrte(void *argv);

/*! schedule_rps()のコールバックルーチン */
void schedule_rps_callrte(void *argv);

/*! schedule_mfq()のコールバックルーチン */
void schedule_mfq_callrte(void *argv);

/*! schedule_odrone()のコールバックルーチン */
void schedule_odrone_callrte(void *argv);

/*! schedule_fr()のコールバックルーチン */
void schedule_fr_callrte(void *argv);

/*! schedule_pfr()のコールバックルーチン */
void schedule_pfr_callrte(void *argv);

/*! Earliest Deadline Firstスケジューリングデッドラインミスハンドラ */
void edfschedule_miss_handler(void *argv);

/*! Least Laxity Firstスケジューリングミスハンドラ */
void llfschedule_miss_handler(void *argv);

/*! tslp_tsk()のコールバックルーチン */
void tslp_callrte(void *argv);

/*! dly_tsk()のコールバックルーチン */
void dly_callrte(void *argv);

/*! twai_sem()のコールバックルーチン */
void twai_sem_callrte(void *argv);

/*! tloc_mtx()のコールバックルーチン */
void tloc_mtx_callrte(void *argv);

/*! trcv_mbx()のコールバックルーチン */
void twai_mbx_callrte(void *argv);


#endif
