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


#ifndef _KERNEL_H_INCLUDE_
#define _KERNEL_H_INCLUDE_


#include "defines.h"
#include "syscall.h"
#include "arch/cpu/interrupt.h"


/*! タスク状態管理マクロ */
/*! LSBから3ビット目までをタスクの状態として使用 */
#define STATE_CLEAR										(0 << 0)		/*! タスク状態を初期化 */
#define TASK_WAIT											(0 << 0)		/*! 0なら待ち状態 */
#define TASK_READY										(1 << 0)		/*! 実行可能状態または実行状態 */
#define TASK_DORMANT									(1 << 1)		/*! 休止状態 */

/*! タスク待ち要因管理マクロ */
/*
* タスク待ち要因管理マクロは静的型と動的型で分けない
* (カーネルオブジェクトは属性で静的か動的かを記録している)
*/
/*! LSBから4ビット目からタスクの待ち要因として使用 */
#define TASK_WAIT_TIME_SLEEP					(1 << 3)		/*! 起床待ち(kz_tslp_tsk()) */
#define TASK_WAIT_TIME_DELAY					(1 << 4)		/*! 時間経過待ち(kz_dly_tsk()) */
#define TASK_WAIT_SEMAPHORE						(1 << 5)		/*! セマフォ待ち */
#define TASK_WAIT_MUTEX								(1 << 6)		/*! mutex待ち */
#define TASK_WAIT_VIRTUAL_MUTEX				(1 << 7)		/*! virtual mutex待ち */
#define TASK_WAIT_MAILBOX							(1 << 8)		/*! mail box待ち */

#define TASK_STATE_INFO								(0x07 << 0)	/*! タスク状態の抜き取り */
#define TASK_WAIT_ONLY_TIME						(3 << 3)		/*! タイマ要因のみ(tslp_tsk()とdly_tsk()) */


/*! タスクのカーネルオブジェクト取得情報管理マクロ */
/*
* ・セマフォ及びmutexを取得したままタスクが死んだ時自動解放するため
* ・カーネルオブジェクト取得情報管理マクロは静的型と動的型で分けない(カーネルオブジェクトは属性で静的か動的かを記録している)
*/
#define TASK_GET_SEMAPHORE						(1 << 1)		/*! セマフォ */
#define TASK_GET_MUTEX								(1 << 2)		/*! mutex */
/* メールボックス取得情報はない */

#define TASK_NOT_GET_OBJECT						(0 << 0)		/*! タスクは何も取得していない場合 */


#define TASK_NAME_SIZE								16					/*!  タスク名の最大値! */
#define INIT_TASK_ID									0						/*!  initタスクIDは0とする */


/*!
* スレッドのスタート・アップ(thread_init())に渡すパラメータ.実行時に内容が変わらないところ
*/ 
typedef struct _task_init_infomation {
	int tskid;														/*! タスクID */
	char tskatr; 													/*! タスク属性 */
  TSK_FUNC func; 												/*! タスクのメイン関数 */
  int argc;       											/*! タスクのメイン関数に渡すargc */
  char **argv;    											/*! タスクのメイン関数に渡すargv */
  char name[TASK_NAME_SIZE];		 				/*! タスク名 */
  int priority; 												/*! 起動時の優先度(優先度変更のシステムコールがあるため) */
} TSK_INITCB;


/*! タスクの待ち情報管理構造体 */
typedef struct _task_wait_infomation {
	struct _task_struct *wait_next;				/*! 待ちオブジェクトへの次ポインタ */
	struct _task_struct *wait_prev;				/*! 待ちオブジェクトへの前ポインタ */
	TMR_OBJP tobjp;												/*! タイマ関連の待ち要因がある時に使用する領域．対象タイマコントロールブロックを記録 */
	WAIT_OBJP wobjp; 											/*! 待ち行列につながれている対象コントロールブロック */
} TSK_WAIT_INFOCB;


/*! タスクのカーネルオブジェクト取得情報(休止状態または未登録状態に遷移する時に使用する) */
typedef struct _task_get_infomation {
	int flags; 														/*! 取得フラグ */
	GET_OBJP gobjp; 											/*! 取得しているカーネルオブジェクトのポインタ */
} TSK_GET_INFOCB;


/*! タスクコンテキスト */
typedef struct _task_interrupt_infomation {
	INTR_TYPE type;												/*! 割込みの種類 */
  UINT32 sp; 														/*! タスクスタックへのポインタ */
} TSK_INTR_INFOCB;


/*! システムコール用バッファ */
typedef struct _task_systemcall_infomation {
	SYSCALL_TYPE flag; 										/*! システムコールかサービスコールかを判別 */
  ISR_TYPE type; 												/*! システムコールのID */
	OBJP ret; 														/*! システムコール返却値を格納しておくポインタ */
  SYSCALL_PARAMCB *param; 							/*! システムコールのパラメータパケットへのポインタ(タスク間通信の受信処理に使用) */
} TSK_SYSCALL_INFOCB;


/*! スケジューラによって依存する情報 */
/*
* readyque_flagは優先度とタイムアウトごとのレディーキューを持つスケジューラに対してのみ設定，処理する
* 他のレディーキュー構造に対しては無視(ACTIV_READYマクロとEXPIRED_READYマクロの値しか設定されない).
*/
typedef struct _scheduler_depend_infomation {
	union {
		/*! タイムスライス型スケジューラに依存する情報 */
		struct {
			int tm_slice;											/*! タスクのタイムスライス(タイムスライスが絡まないスケジューリングの時は-1となる) */
		} slice_schdul;
		/*! 2種類のレディーキューを持つ簡易O(1)スケジューラに依存する情報 */
		struct {
			UINT8 readyque_flag; 							/*! どのレディーキューにいるか */
		} odrone_schdul;
		/* 公平配分型スケジューラに依存する情報 */
		struct {
			int rel_exetim;
		} fr_schdul;
		/* リアルタイム型スケジューラに依存する情報 */
		struct {
			int rel_exetim;										/*! 実行時間(RM専用のメンバで簡単化のため相対デッドライン時間とする) */
			int rate;													/*! 周期 */
			int deadtim;											/*! デッドライン時刻 */
			int floatim;											/*! 余裕時刻 */
			TMR_OBJP tobjp;										/*! スケジューラが使用するソフトタイマオブジェクト(EDF,LLF時使用) */
		} rt_schdul;
	} un;
} SCHDUL_DEP_INFOCB;


/*! レディーによって依存する内容(静的優先度はキュー構造のレディー,動的優先度は2分木と整列リストのレディー) */
typedef struct _ready_depend_infomation {
	union {
		/*! キュー構造のレディー */
		struct {
			struct _task_struct *ready_next; /*! レディーの次ポインタ */
  		struct _task_struct *ready_prev; /*! レディーの前ポインタ */
  	} que_ready;
  	/*! 2分木と整列リスト(動的優先度管理のため)のレディー */
  	struct {
  		int dynamic_prio;									/*! 動的優先度 */
  		struct _task_struct *parent; 			/*! 親 */
			struct _task_struct *left_next; 	/*! 左ポインタ */
			struct _task_struct *right_next; 	/*! 右ポインタ */
			struct _task_struct *sort_next;		/*! 整列リストの次ポインタ */
			struct _task_struct *sort_prev;		/*! 整列リストの前ポインタ */
		} btree_ready;
	} un;
} READY_DEP_INFOCB;


/*!
* タスクコントロールブロック(TCB)
* free listにREADY_DEP_INFOCBのポインタを使用するとごちゃごちゃになるので,別途free list専用メンバを追加
*/
typedef struct _task_struct {
	struct _task_struct *free_next; 	/*! free listの次ポインタ */
  struct _task_struct *free_prev; 	/*! free listの前ポインタ */
	READY_DEP_INFOCB *ready_info;			/*! レディーごとに依存する内容 */
  int priority;   									/*! 静的優先度 */
  char *stack;    									/*! スタックリンカスクリプトに定義されているユーザスタック領域のポインタ */
  UINT16 state;   									/*! 状態フラグ */
  TSK_INITCB init; 									/*! 実行時に内容が変わらないところ */
  TSK_WAIT_INFOCB wait_info; 				/*! 待ち情報管理 */
	TSK_GET_INFOCB get_info; 					/*! 取得情報管理 */
	TSK_INTR_INFOCB intr_info; 				/*! 割込み情報(ここはつねに変動する) */
	TSK_SYSCALL_INFOCB syscall_info; 	/*! システムコール情報管理 */
  SCHDUL_DEP_INFOCB *schdul_info;		/*! スケジューラごとに依存する情報 */
} TCB;


/*! タスク情報 */
struct _task_infomation {
	TCB **id_table; 									/*! task ID変換テーブルへのheadポインタ(可変長配列として使用) */
	TCB *array; 											/*! 静的型taskへのheadポインタ(可変長配列として使用) */
	TCB *freehead; 										/*! 動的型taskリンクドfreeリストのキュー(freeheadポインタを記録) */
	TCB *alochead; 										/*! 動的型taskリンクドalocリストのキュー(alocheadポインタを記録) */
/* インクリメントするのでint型で定義(short型は遅いし，ER_IDはシステムコール返却値型なのでER_IDでOS内部の領域を何か指したくないなから) */
	int counter;											/*! 次の割付可能ID  */
	int power_count; 									/*! task IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
} mg_tsk_info;


/*! ディスパッチ情報 */
struct _dispatcher_infomation {
	char flag; 												/*! ディスパッチ状態フラグ */
	void (*func)(UINT32 *context);		/*! ディスパッチルーチン(1つだけしかないからtypedefしない) */
} dsp_info;


TCB *current; /*! カレントスレッド(実行状態のスレッドの事) */

/* システムコール(ユーザタスクが呼ぶシステムコールのプロトタイプ，実体はsyscall.cにある) */
/*! mz_acre_tsk():タスクコントロールブロックの生成(ID自動割付) */
volatile ER_ID mz_acre_tsk(SYSCALL_PARAMCB *par);

/*! mz_del_tsk():スレッドの排除 */
ER mz_del_tsk(ER_ID tskid);

/*! mz_sta_tsk():スレッドの起動 */
ER mz_sta_tsk(ER_ID tskid);

/*! mz_run_tsk():スレッドの生成(ID自動割付)と起動 */
volatile ER_ID mz_run_tsk(SYSCALL_PARAMCB *par);

/*! mz_ext_tsk():自タスクの終了 */
void mz_ext_tsk(void);

/*! mz_exd_tsk():自スレッドの終了と排除 */
void mz_exd_tsk(void);

/*! mz_ter_tsk():スレッドの強制終了 */
ER mz_ter_tsk(ER_ID tskid);

/*! mz_get_pri():スレッドの優先度取得 */
ER mz_get_pri(ER_ID tskid, int *p_tskpri);

/*! mz_chg_pri():スレッドの優先度変更 */
ER mz_chg_pri(ER_ID tskid, int tskpri);

/*! chg_slt():タスクタイムスライスの変更 */
ER mz_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice);

/*! get_slt():タスクタイムスライスの取得 */
ER mz_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice);

/*! mz_slp_tsk():自タスクの起床待ち */
ER mz_slp_tsk(void);

/*! mz_tslp_tsk():自タスクのタイムアウト付き起床待ち */
ER mz_tslp_tsk(int msec);

/*! mz_wup_tsk():タスクの起床 */
ER mz_wup_tsk(ER_ID tskid);

/*! mz_rel_wai():待ち状態強制解除 */
ER mz_rel_wai(ER_ID tskid);

/*! mz_dly_tsk():自タスクの遅延 */
ER mz_dly_tsk(int msec);

/*! mz_acre_sem():セマフォコントロールブロックの作成(ID自動割付) */
ER_ID mz_acre_sem(SYSCALL_PARAMCB *par);

/*! mz_del_sem():セマフォコントロールブロックの排除 */
ER mz_del_sem(ER_ID semid);

/*! mz_sig_sem():セマフォV操作 */
ER mz_sig_sem(ER_ID semid);

/*! mz_wai_sem():セマフォP操作 */
ER mz_wai_sem(ER_ID semid);

/*! mz_pol_sem():セマフォP操作(ポーリング) */
ER mz_pol_sem(ER_ID semid);

/*! mz_twai_sem():セマフォP操作(タイムアウト付き) */
ER mz_twai_sem(ER_ID semid, int msec);

/*! mz_acre_mbx():メールボックスコントロールブロックの作成(ID自動割付) */
ER_ID mz_acre_mbx(SYSCALL_PARAMCB *par);

/*! mz_del_mbx():メールボックスコントロールブロックの排除 */
ER mz_del_mbx(ER_ID mbxid);

/*! mz_snd_mbx():メールボックスへ送信 */
ER mz_snd_mbx(ER_ID mbxid, T_MSG *pk_msg);

/*! mz_rcv_mbx():メールボックスからの受信 */
ER mz_rcv_mbx(ER_ID mbxid, T_MSG **pk_msg);

/*! mz_prcv_mbx():ポーリング付きメールボックスからの受信 */
ER mz_prcv_mbx(ER_ID mbxid, T_MSG **pk_msg);

/*! mz_trcv_mbx():タイムアウト付きメールボックスからの受信 */
ER mz_trcv_mbx(ER_ID mbxid, T_MSG **pk_msg, int tmout);

/*! mz_acre_mtx():mutexコントロールブロックの作成(ID自動割付) */
ER_ID mz_acre_mtx(SYSCALL_PARAMCB *par);

/*! mz_del_mtx():mutexコントロールブロックの排除 */
ER mz_del_mtx(ER_ID mtxid);

/*! mz_loc_mtx():mutexロック操作 */
ER mz_loc_mtx(ER_ID mtxid);

/*! mz_ploc_mtx():mutexポーリングロック操作 */
ER mz_ploc_mtx(ER_ID mtxid);

/*! mz_tloc_mtx():mutexタイムアウト付きロック操作 */
ER mz_tloc_mtx(ER_ID mtxid, int msec);

/*! mz_unl_mtx():mutexアンロック操作 */
ER mz_unl_mtx(ER_ID mtxid);

/*! mz_get_mpf():動的メモリ獲得 */
void* mz_get_mpf(int size);

/*! mz_rel_mpf():動的メモリ解放 */
int mz_rel_mpf(void *p);

/*! acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付) */
ER_ID mz_acre_cyc(SYSCALL_PARAMCB *par);

/*! del_cyc():周期ハンドラコントロールブロックの排除 */
ER mz_del_cyc(ER_ID cycid);

/*! sta_cyc():周期ハンドラ動作開始 */
ER mz_sta_cyc(ER_ID cycid);

/*! stp_cyc():周期ハンドラ動作停止 */
ER mz_stp_cyc(ER_ID cycid);

/*! acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付) */
ER_ID mz_acre_alm(SYSCALL_PARAMCB *par);

/*! mz_del_alm():アラームハンドラコントロールブロックの排除 */
ER mz_del_alm(ER_ID almid);

/*! mz_sta_alm():アラームハンドラの動作開始 */
ER mz_sta_alm(ER_ID almid, int msec);

/*! mz_stp_alm():アラームの動作停止 */
ER mz_stp_alm(ER_ID almid);

/*! mz_def_inh():割込みハンドラの定義 */
ER mz_def_inh(SOFTVEC type, IR_HANDL handler);

/*! mz_rot_rdp():タスクの優先順位回転 */
ER_ID mz_rot_rdq(int tskpri);

/*! mz_get_tid():実行スレッドID取得 */
ER_ID mz_get_tid(void);

/*! mz_dis_dsp():ディスパッチの禁止 */
void mz_dis_dsp(void);

/*! mz_ena_dsp():ディスパッチの許可 */
void mz_ena_dsp(void);

/*! mz_sns_dsp():ディスパッチの状態参照 */
BOOL mz_sns_dsp(void);

/*! mz_set_pow():省電力モード設定 */
void mz_set_pow(void);

/*! mz_rol_sys():initへのロールバック */
ER mz_rol_sys(ROL_ATR atr);


/* 非タスクコンテキストから呼ぶシステムコールのプロトタイプ，実体はsyscall.cにある) */
/*! mz_iacre_tsk():タスクの生成 */
ER mz_iacre_tsk(SYSCALL_PARAMCB *par);
/*! mz_ista_tsk():タスクの起動 */
ER mz_ista_tsk(ER_ID tskid);

/*! mz_ichg_pri():タスクの優先度変更 */
ER mz_ichg_pri(ER_ID tskid, int tskpri);

/*! mz_iwup_tsk():タスクの起床 */
ER mz_iwup_tsk(ER_ID tskid);

/*! mz_iacre_cyc():周期ハンドラ生成 */
ER mz_iacre_cyc(SYSCALL_PARAMCB *par);

/*! mz_ista_cyc():周期ハンドラ動作開始 */
ER mz_ista_cyc(ER_ID cycid);

/*! mz_irot_rdp():タスクの優先順位回転 */
ER_ID mz_irot_rdq(int tskpri);


/* サービスコール(ユーザタスクが呼ぶシステムコールのプロトタイプ，実体はsrvcall.cにある) */
/*! mv_acre_tsk():タスクコントロールブロックの生成(ID自動割付) */
ER_ID mv_acre_tsk(SYSCALL_PARAMCB *par);
		      
/*! mv_del_tsk():スレッドの排除 */
ER mv_del_tsk(ER_ID tskid);

/*! mz_ter_tsk():スレッドの強制終了 */
ER mv_ter_tsk(ER_ID tskid);

/*! mv_get_pri():スレッドの優先度取得 */
ER mv_get_pri(ER_ID tskid, int *p_tskpri);

/*! mv_chg_slt():タスクタイムスライスの変更 */
ER mv_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice);

/*! mv_get_slt():タスクタイムスライスの取得 */
ER mv_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice);

/*! mv_acre_sem():セマフォコントロールブロックの作成(ID自動割付) */
ER_ID mv_acre_sem(SYSCALL_PARAMCB *par);

/*! mv_del_sem():セマフォコントロールブロックの排除 */
ER_ID mv_del_sem(ER_ID semid);

/*! mv_acre_mbx():メールボックスコントロールブロックの作成(ID自動割付) */
ER_ID mv_acre_mbx(SYSCALL_PARAMCB *par);

/*! mv_del_mbx():メールボックスコントロールブロックの排除 */
ER_ID mv_del_mbx(ER_ID mbxid);

/*! mv_acre_mtx():mutexコントロールブロックの作成(ID自動割付) */
ER_ID mv_acre_mtx(SYSCALL_PARAMCB *par);

/*! mv_del_mtx():mutexコントロールブロックの排除 */
ER_ID mv_del_mtx(ER_ID mtxid);

/*! mv_acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付) */
ER_ID mv_acre_cyc(SYSCALL_PARAMCB *par);

/*! mv_del_cyc():周期ハンドラコントロールブロックの排除 */
ER mv_del_cyc(ER_ID cycid);

/*! mv_sta_cyc():周期ハンドラの動作開始 */
ER mv_sta_cyc(ER_ID cycid);

/*! mv_stp_cyc():周期ハンドラの動作停止 */
ER mv_stp_cyc(ER_ID cycid);

/*! mv_acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付) */
ER_ID mv_acre_alm(SYSCALL_PARAMCB *par);

/*! mv_del_alm():アラームハンドラコントロールブロックの排除 */
ER mv_del_alm(ER_ID almid);

/*! mv_sta_alm():アラームハンドラの動作開始 */
ER mv_sta_alm(ER_ID almid, int msec);

/*! mv_stp_alm():アラームの動作停止 */
ER mv_stp_alm(ER_ID almid);

/*! mv_def_inh():割込みハンドラの定義 */
ER_ID mv_def_inh(SOFTVEC type, IR_HANDL handler);

/*! mv_rot_rdp():タスクの優先順位回転 */
ER_ID mv_rot_rdq(int tskpri);

/*! mv_get_tid():実行スレッドID取得 */
ER_ID mv_get_tid(void);

/*! mv_dis_dsp():ディスパッチの禁止 */
void mv_dis_dsp(void);

/*! mv_ena_dsp():ディスパッチの許可 */
void mv_ena_dsp(void);

/*! mv_sns_dsp():ディスパッチの状態参照 */
BOOL mv_sns_dsp(void);

/*! mv_set_pow():省電力モード設定 */
void mv_set_pow(void);

/*! mv_sel_schdul():スケジューラの切り替え */
ER mv_sel_schdul(SCHDUL_TYPE type, long param);

/*! mv_rol_sys():initへのロールバック */
ER mv_rol_sys(ROL_ATR atr);


/*! initタスク */
int start_threads(int argc, char *argv[]);

/*! ユーザタスク及び資源情報 */
#ifdef TSK_LIBRARY
int sample_tsk1_main(int argc, char *argv[]);
int sample_tsk2_main(int argc, char *argv[]);
int sample_tsk3_main(int argc, char *argv[]);
int sample_tsk4_main(int argc, char *argv[]);
int sample_tsk5_main(int argc, char *argv[]);
int sample_tsk6_main(int argc, char *argv[]);
int sample_tsk7_main(int argc, char *argv[]);
int sample_tsk8_main(int argc, char *argv[]);
int sample_tsk9_main(int argc, char *argv[]);
int sample_tsk10_main(int argc, char *argv[]);
int sample_tsk11_main(int argc, char *argv[]);
int sample_tsk12_main(int argc, char *argv[]);
int sample_tsk13_main(int argc, char *argv[]);
int sample_tsk14_main(int argc, char *argv[]);
int sample_tsk15_main(int argc, char *argv[]);
int sample_tsk16_main(int argc, char *argv[]);
int sample_tsk17_main(int argc, char *argv[]);
int sample_tsk18_main(int argc, char *argv[]);
int sample_tsk19_main(int argc, char *argv[]);
int sample_tsk20_main(int argc, char *argv[]);
int sample_tsk21_main(int argc, char *argv[]);
int sample_tsk22_main(int argc, char *argv[]);
int sample_tsk23_main(int argc, char *argv[]);
int sample_tsk24_main(int argc, char *argv[]);
int sample_tsk25_main(int argc, char *argv[]);
int sample_tsk26_main(int argc, char *argv[]);
int sample_tsk27_main(int argc, char *argv[]);
int sample_tsk28_main(int argc, char *argv[]);
int sample_tsk29_main(int argc, char *argv[]);
int sample_tsk30_main(int argc, char *argv[]);
int sample_tsk31_main(int argc, char *argv[]);
int sample_tsk32_main(int argc, char *argv[]);
int sample_tsk33_main(int argc, char *argv[]);
int sample_tsk34_main(int argc, char *argv[]);
int sample_tsk35_main(int argc, char *argv[]);
int sample_tsk36_main(int argc, char *argv[]);
int sample_tsk37_main(int argc, char *argv[]);
int sample_tsk38_main(int argc, char *argv[]);
int sample_tsk39_main(int argc, char *argv[]);
int sample_tsk40_main(int argc, char *argv[]);
int sample_tsk41_main(int argc, char *argv[]);
int sample_tsk42_main(int argc, char *argv[]);
int sample_tsk43_main(int argc, char *argv[]);
int sample_tsk44_main(int argc, char *argv[]);
int sample_tsk45_main(int argc, char *argv[]);
int sample_tsk46_main(int argc, char *argv[]);
int sample_tsk47_main(int argc, char *argv[]);
int sample_tsk48_main(int argc, char *argv[]);
int sample_tsk49_main(int argc, char *argv[]);
int sample_tsk50_main(int argc, char *argv[]);
int sample_tsk51_main(int argc, char *argv[]);
int sample_tsk52_main(int argc, char *argv[]);
int sample_tsk53_main(int argc, char *argv[]);
int sample_tsk54_main(int argc, char *argv[]);
int sample_tsk55_main(int argc, char *argv[]);
int sample_tsk56_main(int argc, char *argv[]);
int sample_tsk57_main(int argc, char *argv[]);
int sample_tsk58_main(int argc, char *argv[]);
int sample_tsk59_main(int argc, char *argv[]);
int sample_tsk60_main(int argc, char *argv[]);
int sample_tsk61_main(int argc, char *argv[]);
int sample_tsk62_main(int argc, char *argv[]);
int sample_tsk63_main(int argc, char *argv[]);
int sample_tsk64_main(int argc, char *argv[]);
int sample_tsk65_main(int argc, char *argv[]);
int sample_tsk66_main(int argc, char *argv[]);
int sample_tsk67_main(int argc, char *argv[]);
int sample_tsk68_main(int argc, char *argv[]);
int sample_tsk69_main(int argc, char *argv[]);
int sample_tsk70_main(int argc, char *argv[]);
int sample_tsk71_main(int argc, char *argv[]);
int sample_tsk72_main(int argc, char *argv[]);
int sample_tsk73_main(int argc, char *argv[]);
int sample_tsk74_main(int argc, char *argv[]);
int sample_tsk75_main(int argc, char *argv[]);
int sample_tsk76_main(int argc, char *argv[]);
int sample_tsk77_main(int argc, char *argv[]);
int sample_tsk78_main(int argc, char *argv[]);
int sample_tsk79_main(int argc, char *argv[]);
int sample_tsk80_main(int argc, char *argv[]);
int sample_tsk81_main(int argc, char *argv[]);
int sample_tsk82_main(int argc, char *argv[]);
int sample_tsk83_main(int argc, char *argv[]);
int sample_tsk84_main(int argc, char *argv[]);
int sample_tsk85_main(int argc, char *argv[]);
int sample_tsk86_main(int argc, char *argv[]);
int sample_tsk87_main(int argc, char *argv[]);
int sample_tsk88_main(int argc, char *argv[]);
int sample_tsk89_main(int argc, char *argv[]);
int sample_tsk90_main(int argc, char *argv[]);
int sample_tsk91_main(int argc, char *argv[]);
int sample_tsk92_main(int argc, char *argv[]);
int sample_tsk93_main(int argc, char *argv[]);
int sample_tsk94_main(int argc, char *argv[]);
int sample_tsk95_main(int argc, char *argv[]);
int sample_tsk96_main(int argc, char *argv[]);
int sample_tsk97_main(int argc, char *argv[]);
int sample_tsk98_main(int argc, char *argv[]);
int sample_tsk99_main(int argc, char *argv[]);
int sample_tsk100_main(int argc, char *argv[]);
int sample_tsk101_main(int argc, char *argv[]);
int sample_tsk102_main(int argc, char *argv[]);
int sample_tsk103_main(int argc, char *argv[]);
int sample_tsk104_main(int argc, char *argv[]);
int sample_tsk105_main(int argc, char *argv[]);
int sample_tsk106_main(int argc, char *argv[]);
int sample_tsk107_main(int argc, char *argv[]);
int sample_tsk108_main(int argc, char *argv[]);
int sample_tsk109_main(int argc, char *argv[]);
int sample_tsk110_main(int argc, char *argv[]);
int sample_tsk111_main(int argc, char *argv[]);
int sample_tsk112_main(int argc, char *argv[]);
int sample_tsk113_main(int argc, char *argv[]);
int sample_tsk138_main(int argc, char *argv[]);
extern ER_ID sample_tsk1_id;
extern ER_ID sample_tsk2_id;
extern ER_ID sample_tsk3_id;
extern ER_ID sample_tsk4_id;
extern ER_ID sample_tsk5_id;
extern ER_ID sample_tsk6_id;
extern ER_ID sample_tsk7_id;
extern ER_ID sample_tsk8_id;
extern ER_ID sample_tsk9_id;
extern ER_ID sample_tsk10_id;
extern ER_ID sample_tsk11_id;
extern ER_ID sample_tsk12_id;
extern ER_ID sample_tsk13_id;
extern ER_ID sample_tsk14_id;
extern ER_ID sample_tsk15_id;
extern ER_ID sample_tsk16_id;
extern ER_ID sample_tsk17_id;
extern ER_ID sample_tsk18_id;
extern ER_ID sample_tsk19_id;
extern ER_ID sample_tsk20_id;
extern ER_ID sample_tsk21_id;
extern ER_ID sample_tsk22_id;
extern ER_ID sample_tsk23_id;
extern ER_ID sample_tsk24_id;
extern ER_ID sample_tsk25_id;
extern ER_ID sample_tsk26_id;
extern ER_ID sample_tsk27_id;
extern ER_ID sample_tsk28_id;
extern ER_ID sample_tsk29_id;
extern ER_ID sample_tsk30_id;
extern ER_ID sample_tsk31_id;
extern ER_ID sample_tsk32_id;
extern ER_ID sample_tsk33_id;
extern ER_ID sample_tsk34_id;
extern ER_ID sample_tsk35_id;
extern ER_ID sample_tsk36_id;
extern ER_ID sample_tsk37_id;
extern ER_ID sample_tsk38_id;
extern ER_ID sample_tsk39_id;
extern ER_ID sample_tsk40_id;
extern ER_ID sample_tsk41_id;
extern ER_ID sample_tsk42_id;
extern ER_ID sample_tsk43_id;
extern ER_ID sample_tsk44_id;
extern ER_ID sample_tsk45_id;
extern ER_ID sample_tsk46_id;
extern ER_ID sample_tsk47_id;
extern ER_ID sample_tsk48_id;
extern ER_ID sample_tsk49_id;
extern ER_ID sample_tsk50_id;
extern ER_ID sample_tsk51_id;
extern ER_ID sample_tsk52_id;
extern ER_ID sample_tsk53_id;
extern ER_ID sample_tsk54_id;
extern ER_ID sample_tsk55_id;
extern ER_ID sample_tsk56_id;
extern ER_ID sample_tsk57_id;
extern ER_ID sample_tsk58_id;
extern ER_ID sample_tsk59_id;
extern ER_ID sample_tsk60_id;
extern ER_ID sample_tsk61_id;
extern ER_ID sample_tsk62_id;
extern ER_ID sample_tsk63_id;
extern ER_ID sample_tsk64_id;
extern ER_ID sample_tsk65_id;
extern ER_ID sample_tsk66_id;
extern ER_ID sample_tsk67_id;
extern ER_ID sample_tsk68_id;
extern ER_ID sample_tsk69_id;
extern ER_ID sample_tsk70_id;
extern ER_ID sample_tsk71_id;
extern ER_ID sample_tsk72_id;
extern ER_ID sample_tsk73_id;
extern ER_ID sample_tsk74_id;
extern ER_ID sample_tsk75_id;
extern ER_ID sample_tsk76_id;
extern ER_ID sample_tsk77_id;
extern ER_ID sample_tsk78_id;
extern ER_ID sample_tsk79_id;
extern ER_ID sample_tsk80_id;
extern ER_ID sample_tsk81_id;
extern ER_ID sample_tsk82_id;
extern ER_ID sample_tsk83_id;
extern ER_ID sample_tsk84_id;
extern ER_ID sample_tsk85_id;
extern ER_ID sample_tsk86_id;
extern ER_ID sample_tsk87_id;
extern ER_ID sample_tsk88_id;
extern ER_ID sample_tsk89_id;
extern ER_ID sample_tsk90_id;
extern ER_ID sample_tsk91_id;
extern ER_ID sample_tsk92_id;
extern ER_ID sample_tsk93_id;
extern ER_ID sample_tsk94_id;
extern ER_ID sample_tsk95_id;
extern ER_ID sample_tsk96_id;
extern ER_ID sample_tsk97_id;
extern ER_ID sample_tsk98_id;
extern ER_ID sample_tsk99_id;
extern ER_ID sample_tsk100_id;
extern ER_ID sample_tsk101_id;
extern ER_ID sample_tsk102_id;
extern ER_ID sample_tsk103_id;
extern ER_ID sample_tsk104_id;
extern ER_ID sample_tsk105_id;
extern ER_ID sample_tsk106_id;
extern ER_ID sample_tsk107_id;
extern ER_ID sample_tsk108_id;
extern ER_ID sample_tsk109_id;
extern ER_ID sample_tsk110_id;
extern ER_ID sample_tsk111_id;
extern ER_ID sample_tsk112_id;
extern ER_ID sample_tsk113_id;
extern ER_ID sample_tsk138_id;
extern ER_ID sem0_id;
extern ER_ID mtx0_id;
extern ER_ID mtx1_id;
void cycle_handler0(void *exinf);
void cycle_handler1(void *exinf);
void cycle_handler2(void *exinf);
void cycle_handler3(void *exinf);
void cycle_handler4(void *exinf);
void cycle_handler5(void *exinf);
extern ER_ID cyc0_id;
extern ER_ID cyc1_id;
extern ER_ID cyc2_id;
extern ER_ID cyc3_id;
extern ER_ID cyc4_id;
extern ER_ID cyc5_id;
extern ER_ID alm0_id;
extern ER_ID alm1_id;
extern ER_ID alm2_id;

#endif


/* kernel.cの関数を定義 */
/*! tskid変換テーブル設定処理(acre_tsk():タスクコントロールブロックの生成(ID自動割付)) */
ER_ID kernelrte_acre_tsk(TSK_TYPE type, TSK_FUNC func, char *name, int priority,
				 int stacksize, int rate, int exetim, int deadtim, int foatim, int argc, char *argv[]);

/*! tskid変換テーブル設定処理(del_tsk():スレッドの排除) */
ER kernelrte_del_tsk(ER_ID tskid);

/*! tskid変換テーブル設定処理(sta_tsk():スレッドの起動) */
ER kernelrte_sta_tsk(ER_ID tskid);

/*! tskid変換テーブル設定処理(ter_tsk():スレッドの強制終了) */
ER kernelrte_ter_tsk(ER_ID tskid);

/*! tskid変換テーブル設定処理(get_pri():スレッドの優先度取得) */
ER kernelrte_get_pri(ER_ID tskid, int *p_tskpri);

/*! tskid変換テーブル設定処理(chg_slt():タスクタイムスライスの変更) */
ER kernelrte_chg_slt(SCHDUL_TYPE type, ER_ID tskid, int slice);

/*! tskid変換テーブル設定処理(get_slt():タスクタイムスライスの取得) */
ER kernelrte_get_slt(SCHDUL_TYPE type, ER_ID tskid, int *p_slice);

/*! semid変換テーブル設定処理(acre_sem():セマフォコントロールブロックの作成(ID自動割付)) */
ER_ID kernelrte_acre_sem(SEM_TYPE type, SEM_ATR atr, int semvalue, int maxvalue);

/*! semid変換テーブル設定処理(del_sem():セマフォコントロールブロックの排除) */
ER kernelrte_del_sem(ER_ID semid);

/*! mbxid変換テーブル設定処理(acre_mbx():メールボックスコントロールブロックの作成(ID自動割付)) */
ER_ID kernelrte_acre_mbx(MBX_TYPE type, MBX_MATR msg_atr, MBX_WATR wai_atr, int max_msgpri);

/*! mbxid変換テーブル設定処理(del_mbx():メールボックスコントロールブロックの排除) */
ER kernelrte_del_mbx(ER_ID mbxid);

/*! mtxid変換テーブル設定処理(acre_mtx():mutexコントロールブロックの作成(ID自動割付)) */
ER_ID kernelrte_acre_mtx(MTX_TYPE type, MTX_ATR atr, PIVER_TYPE piver_type, int maxlocks, int pcl_param);

/*! mtxid変換テーブル設定処理(del_mtx():mutexコントロールブロックの排除) */
ER kernelrte_del_mtx(ER_ID mtxid);

/*! cycid変換テーブル設定処理(acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付)) */
ER_ID kernelrte_acre_cyc(CYC_TYPE type, void *exinf, int cyctim, TMR_CALLRTE func);

/*! cycid変換テーブル設定処理(del_cyc():周期ハンドラコントロールブロックの排除) */
ER kernelrte_del_cyc(ER_ID cycid);

/*! cycid変換テーブル設定処理(sta_cyc():周期ハンドラの動作開始) */
ER kernelrte_sta_cyc(ER_ID cycid);

/*! cycid変換テーブル設定処理(stp_cyc():周期ハンドラの動作停止) */
ER kernelrte_stp_cyc(ER_ID cycid);

/*! almid変換テーブル設定処理(mz_acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付)) */
ER_ID kernelrte_acre_alm(ALM_TYPE type, void *exinf, TMR_CALLRTE func);

/*! almid変換テーブル設定処理(del_alm():アラームハンドラコントロールブロックの排除) */
ER kernelrte_del_alm(ER_ID almid);

/*! almid変換テーブル設定処理(sta_alm():アラームハンドラの動作開始) */
ER kernelrte_sta_alm(ER_ID almid, int msec);

/*! almid変換テーブル設定処理(stp_alm():アラームの動作停止) */
ER kernelrte_stp_alm(ER_ID almid);

/*! 変換テーブル設定処理はいらない(def_inh():割込みハンドラの定義) */
ER kernelrte_def_inh(SOFTVEC type, IR_HANDL handler);

/*! 変換テーブル設定処理はいらない(rot_rdp():タスクの優先順位回転) */
ER kernelrte_rot_rdq(int tskpri);

/*! 変換テーブル設定処理はいらない(get_tid():実行スレッドID取得) */
ER_ID kernelrte_get_tid(void);

/* 変換テーブル設定処理はいらない(dis_dsp():ディスパッチの禁止) */
void kernelrte_dis_dsp(void);

/*! 変換テーブル設定処理はいらない(ena_dsp():ディスパッチの許可) */
void kernelrte_ena_dsp(void);

/*! 変換テーブル設定処理はいらない(sns_dsp():ディスパッチの状態参照) */
BOOL kernelrte_sns_dsp(void);

/*! 変換テーブル設定処理はいらない(set_pow():省電力モード設定) */
void kernelrte_set_pow(void);

/*! 変換テーブル設定処理はいらない(sel_schdul():スケジューラの切り替え) */
ER kernelrte_sel_schdul(SCHDUL_TYPE type, long param);

/*! 変換テーブル設定処理はいらない(rol_sys();initへのロールバック) */
ER kernelrte_rol_sys(ROL_ATR atr);

/*! 非タスクコンテキスト用システムコール呼び出しライブラリ関数 */
void isyscall_intr(ISR_TYPE type, SYSCALL_PARAMCB *param);

/*! 割込み処理入り口関数(ベクタに登録してある割込みハンドラ) */
ER thread_intr(SOFTVEC type, UINT32 sp);

/*! トラップ発行(システムコール) */
void issue_trap_syscall(ISR_TYPE type, SYSCALL_PARAMCB *param, OBJP ret);

/*! トラップ発行(タスクを落とす) */
void issue_trap_softerr(void);

/*! initタスクの生成と起動をする */
void process_init_tsk(TSK_FUNC func, char *name, int priority, int stacksize,
	      int argc, char *argv[]);

/*! initタスクの生成と起動をする */
void start_init_tsk(TSK_FUNC func, char *name, int priority, int stacksize,
	      int argc, char *argv[]);

/*! OSの致命的エラー時 */
void down_system(void);

/*! 処理待ち(NMI) */
void freeze_kernel(void);

/*! カーネルオブジェクトを取得しているか検査し，取得しているならば各解放関数を呼ぶ */
ER release_object(TCB *checktcb);

/*! どの待ち行列からTCBを抜き取るか分岐する */
ER get_tsk_waitque(TCB *worktcb, UINT16 flags);


#endif
