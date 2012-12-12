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


#ifndef _TIMER_DRIVER_H_INCLUDE_
#define _TIMER_DRIVER_H_INCLUDE_


#define HARD_TIMER_CYCLE_FLAG 	(1 << 0)	/*! 周期的に使用するためのフラグ */

/* ソフトタイマの要求種類を操作するフラグ */
#define SCHEDULER_MAKE_TIMER 		(0 << 0)	/*! スケジューラが使用するタイマを記録する */
#define OTHER_MAKE_TIMER 				(1 << 0)	/*! 上記以外が使用するタイマを記録する */

/*! タイマコントロールブロック*/
typedef struct _timer_struct {
	struct _timer_struct *next;							/*! 次ポインタ*/
	struct _timer_struct *prev;							/*! 前ポインタ*/
	short flag;															/*! スケジューラが使用するタイマブロックかその他が使用するタイマブロックかを記録(ソフトタイマは1つしかないため) */
	int msec;																/*! 要求タイマ値*/
	TMRRQ_OBJP rqobjp;											/*! タイマを要求したオブジェクトのポインタ(ソフトタイマで周期機能を使用したい時のみ設定) */
	TMR_CALLRTE func;												/*! コールバックルーチン */
	void *argv;															/*! コールバックルーチンへのポインタ */
} TMRCB;

/* リスト管理するタイマは一つなので配列化はしない．先頭から探していき挿入する事を前提とするのでtailポインタはいらない */
/*! タイマキュー型構造体 */
struct _timer_queue {
	TMRCB *tmrhead;													/*! タイマコントロールブロックの先頭ポインタ */
	int index;															/*! タイマ番号(ソフトタイマは1) */
} mg_timerque;


/*ターゲット依存部 */
/*! タイマ開始処理 */
void start_timer(int index, int msec, int flags);

/*! タイマキャンセルする関数 */
void cancel_timer(int index);

/*! タイマの現在値を取得する関数 */
ER_VLE get_timervalue(int index);

/* ターゲット非依存部 */
/*! タイマ割込みハンドラ */
void tmrdriver_intr(void);

/*! タイマドライバの初期化 */
void tmrdriver_init(void);

/*! 差分のキューのノードを作成 */
OBJP create_tmrcb_diffque(short flag, int request_sec, TMRRQ_OBJP rqobjp, TMR_CALLRTE func, void *argv);

/*! 引数で指定されたタイマコントロールブロックを排除する */
ER delete_tmrcb_diffque(TMRCB *deltbf);


#endif
