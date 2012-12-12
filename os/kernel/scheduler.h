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


#ifndef _SCHEDULER_H_INCLUDE_
#define _SCHEDULER_H_INCLUDE_


#include "defines.h"
#include "kernel.h"
#include "ready.h"


/*! スケジューラコントロールブロック */
/*
* ～スケジューラが個別に持つ情報～
*/
typedef struct {
	union {
		/*! FCFSスケジューリングエリア  */
		struct {
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} fcfs_schdul;
		/*! ラウンドロビンスケジューリングエリア */
		struct {
#define RR_TIME_SLICE 1000 				/*! RRスケジューリング,タイムスライスデフォルト値 */
			int tmout; 									/*! スケジューラへ渡すパラメータ(デフォルト時でのタイムスライス) */
			TMR_OBJP tobjp; 						/*! スケジューラが使用する対象タイマコントロールブロックを記録 */
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} rr_schdul;
		/*! 優先度スケジューリングエリア */
		struct {
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} ps_schdul;
		/*! ラウンドロビン×優先度スケジューリングエリア */
		struct {
#define RPS_TIME_SLICE 1000 			/*! RPスケジューリングタイムスライスデフォルト値 */
			int tmout; 									/*! スケジューラへ渡すパラメータ(デフォルト時でのタイムスライス) */
			TMR_OBJP tobjp; 						/*! スケジューラが使用する対象タイマコントロールブロックを記録 */
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} rps_schdul;
		/*! Multilevel Feedback Queueエリア(スケジュール優先度は実装しない) */
		struct {
#define MFQ_TIME_SLICE 1000 			/*! MFQスケジューリング,タイムスライスデフォルト値 */
			int tmout;									/*! スケジューラへ渡すパラメータ(デフォルト時での標準タイムスライス) */
			TMR_OBJP tobjp;							/*! スケジューラが使用する対象タイマコントロールブロックを記録 */
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} mfq_schdul;
		/*! 簡易O(1)スケジューリングエリア */
		struct {
#define ODRONE_SLICE_WEIGHT 100	 	/*! 簡易O(1)スケジューリング,タイムスライス重み値(h8はクロック20MHzのため) */
			int tmout;									/*! タイムスライスのデフォルト値(とりあえずこの実装) */
			TMR_OBJP tobjp;							/*! スケジューラが使用する対象タイマコントロールブロックを記録 */
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} odrone_schdul;
		/*!  Fair Schedulerエリア */
		struct {
#define FR_TIME_SLICE 1000 			/*! FRスケジューリング,タイムスライスデフォルト値 */
			int tmout; 									/*! スケジューラへ渡すパラメータ(デフォルト時でのタイムスライス) */
			TMR_OBJP tobjp; 						/*! スケジューラが使用する対象タイマコントロールブロックを記録 */
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} fr_schdul;
		struct {
#define PFR_TIME_SLICE 1000 			/*! PFRスケジューリング,タイムスライスデフォルト値 */
			int tmout; 									/*! スケジューラへ渡すパラメータ(デフォルト時でのタイムスライス) */
			TMR_OBJP tobjp; 						/*! スケジューラが使用する対象タイマコントロールブロックを記録 */
			void (*rte)(void); 				/*! スケジューラへのポインタ */
		} pfr_schdul;
		/*! Rate Monotonicエリア */
		struct {
			int unroll_rate;						/*! 周期最小公倍数(create()されたタスクまで) */
			int unroll_exetim;					/*! 周期に沿った最大実行時間(create()されたタスクまで).簡単化のため相対デッドライン時間とする */
			void (*rte)(void);					/*! スケジューラへのポインタ */
		} rms_schdul;
		/*! Deadline Monotonicエリア */
		struct {
			int unroll_dead;						/*! デッドライン最小公倍数(create()されたタスクまで) */
			int unroll_exetim;					/*! 周期に沿った最大実行時間(create()されたタスクまで).簡単化のため相対デッドライン時間とする */
			void (*rte)(void);					/*! スケジューラへのポインタ */
		} dms_schdul;
		/* Earliest Deadline Firstエリア */
		struct {
			void (*rte)(void);					/*! スケジューラへのポインタ */
		} edf_schdul;
		/* Least Laxity Firstエリア */
		struct {
			void (*rte)(void);					/*! スケジューラへのポインタ */
		} llf_schdul;
	} un;
} SCHDULCB;


/*! スケジューリング情報 */
/* ～スケジューラで共通で持つ情報～ */
struct scheduler_infomation {
	SCHDUL_TYPE type;								/*! スケジューリングタイプ */
	SCHDULCB *entry; 								/*! スケジューラコントロールブロックポインタ */
} mg_schdul_info;


/*
* これらはinit_tsk()生成でも使用される
* パラメータを必要としないスケジューラは下記の定義はない
*/
/*! ラウンドロビンスケジューリングパラメータ情報 */
enum {
	RR_TMOUT_PARAM_NUMBER = 0,			/*! スケジューラコントロールブロックへ渡す第一パラメータ番号 */
	RR_PARAM_NUM,										/*! パラメータの数 */
};


/*! ラウンドロビン×優先度スケジューリングパラメータ情報 */
enum {
	RR_PRI_TMOUT_PARAM_NUMBER = 0,	/*! スケジューラコントロールブロックへ渡す第一パラメータ番号 */
	RR_PRI_PARAM_NUM,								/*! パラメータの数 */
};


/*! Multilevel Feedback Queueパラメータ情報 */
enum {
	MFQ_TMOUT_PARAM_NUMBER = 0, 		/*! スケジューラコントロールブロックへ渡す第一パラメータ番号 */
	MFQ_PARAM_NUM,									/*! パラメータの数 */
};


/*! 簡易O(1)スケジューリングパラメータ情報 */
enum {
	ODRONE_TMOUT_PARAM_NUMBER = 0,	/*! スケジューラコントロールブロックへ渡す第一パラメータ番号 */
	ODRONE_PARAM_NUM,								/*! パラメータの数 */
};


/*! Fair Schedulerパラメータ情報 */
enum {
	FR_TMOUT_PARAM_NUMBER = 0,			/*! スケジューラコントロールブロックへ渡す第一パラメータ番号 */
	FR_PARAM_NUM,										/*! パラメータの数 */
};


/*! Priority Fair Schedulerパラメータ情報 */
enum {
	PFR_TMOUT_PARAM_NUMBER = 0,			/*! スケジューラコントロールブロックへ渡す第一パラメータ番号 */
	PFR_PARAM_NUM,									/*! パラメータの数 */
};


/*! スケジューラ情報メモリセグメントへ書き込み */
ER write_schdul(SCHDUL_TYPE type, long param);

/*! システムコール処理(sel_schdul():スケジューラの切り替え) */
ER sel_schdul_isr(SCHDUL_TYPE type, long param);

/*! スケジューラの初期化 */
ER schdul_init(void);

/*! 指定されたTCBをどのタイプのレディーキューから抜き取るか分岐 */
void schedule(void);

/*! タイムスライス型スケジューリング環境下で割込みが発生しタイマブロックを排除するか検査する関数 */
void check_tmslice_schedul(SOFTVEC type);

/*! Rate Monotonic専用，展開スケジューリングをするための関数 */
void set_unrolled_schedule_val(int rate, int exetim);


#endif
