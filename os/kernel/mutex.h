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


#ifndef _MUTEX_H_INCLUDE_
#define _MUTEX_H_INCLUDE_

#include "defines.h"


/*
* ／優先度逆転機構はシステム全体で複数とることができるので，unionでは定義しない
* ／1つのmutexに同時に2つのプロトコルは選択できないので，各プロトコルが使用する．
*   使用するプロトコルは即時型最高値固定プロトコル，遅延型最高値固定プロトコル，
*   優先度上限プロトコルとなる．
*   優先度継承プロトコル，スタックリソースポリシー，virtual priority inheritanceは使用しない．
* ／pcl_paramが各mutexの優先度上限プロトコルの上限値，即時型最高値固定プロトコルの最高値固定，
*   遅延型最高値固定プロトコルの最高値固定となる．
* ／*pcl_next，*pcl_prevが各mutexの優先度上限プロトコルとなっているmutexのポインタ，
*    即時型最高値固定プロトコルとなっているmutexのポインタ，遅延型最高値固定プロトコルとなっているmutexポインタとなる．
*/
/*! 優先度逆転機構コントロールブロック */
typedef struct _mutex_protocol_infomation {
	int pcl_param; 										/*! プロトコルパラメータ */
	struct _mutex_struct *pcl_next;		/*! プロトコル適用時のmutex次ポインタ */
	struct _mutex_struct *pcl_prev;		/*! プロトコル適用時のmutex前ポインタ */
} MTX_PCL_INFOCB;


/*!
* mutexコントロールブロック
* owner_priorityは使用しない.そのかわりTCBのinit.priorityを使用する
*/
typedef struct _mutex_struct {
	struct _mutex_struct *next;				/*! 次ミューテックスへのポインタ */
	struct _mutex_struct *prev;				/*! 次ミューテックスへのポインタ */
	int mtxid;												/*! mutexID */
	MTX_TYPE mtx_type;								/*! mutexのタイプ(静的か動的か) */
	MTX_ATR atr;											/*! mutex属性 */
	PIVER_TYPE piver_type;						/*! 優先度逆転機構 */
	int mtxvalue;											/*! mutexの値 */
	int locks;												/*! mutex再帰ロック数 */
	int maxlocks;											/*! mutex再帰ロック数の上限値 */
	int ownerid;											/*! muetxオーナータスクID */
	MTX_PCL_INFOCB pcl;								/*! 各プロトコル情報エリア */
	TCB *waithead;										/*! mutex待ちタスクへの先頭ポインタ */
	TCB *waittail;										/*! mutex待ちタスクへの最尾ポインタ */
} MTXCB;


/*! mutex情報 */
struct _mutex_infomation {
	MTXCB **id_table;									/*! mutex ID変換テーブルへのheadポインタ(可変長配列として使用) */
	MTXCB *array;											/*! 静的型mutexへのheadポインタ(可変長配列として使用) */
	MTXCB *freehead;									/*! 動的型mutexリンクドfreeリストのキュー(freeheadポインタを記録) */
	MTXCB *alochead;									/*! 動的型mutexリンクドalocリストのキュー(alocheadポインタを記録) */
/* インクリメントするのでint型で定義(short型は遅いし，ER_IDはシステムコール返却値型なのでER_IDでOS内部の領域を何か指したくないな～) */
	int counter;											/*! 次の割付可能ID  */
	int power_count;									/*! mutex IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
	MTXCB *dyhigh_loc_head;						/*! 遅延型最高値固定プロトコルの最高値固定となったmutexへのhead */
	MTXCB *imhigh_loc_head;						/*! 即時型最高値固定プロトコルの最高値固定となったmutexへのhead */
	MTXCB *ceil_pri_head;							/*! 優先度上限プロトコルのシーリング優先度となったmutexへのhead */
	MTXCB *virtual_mtx;								/*! 仮想mutex */
} mg_mtx_info;


/*! 優先度逆転機構の初期化とmutexの初期化 */
ER mtx_priver_init(void);

/*! ミューテックスの初期化(mutex ID変換テーブルの領域確保と初期化) */
ER mtx_init(void);

/*! 優先度逆転問題解消プロトコルによってロック処理を切り替える */
ER check_loc_mtx_protocol(MTXCB *mcb);

/*! 優先度逆転問題解消プロトコルによってポーリングロック処理を切り替える */
ER check_ploc_mtx_protocol(MTXCB *mcb);

/*! 優先度逆転問題解消プロトコルによってタイムアウト付きロック処理を切り替える */
ER check_tloc_mtx_protocol(MTXCB *mcb, int msec);

/*! 優先度逆転問題解消プロトコルによってアンロック処理を切り替える */
ER check_unl_mtx_protocol(MTXCB *mcb);

/*! mutexを無条件で解放する関数への分岐(プロトコルによって処理を切り替える) */
ER check_mtx_uncondy_protocol(MTXCB *mcb);

/*! システムコール処理(acre_mtx():mutexコントロールブロックの作成(ID自動割付)) */
OBJP acre_mtx_isr(MTX_TYPE type, MTX_ATR atr, PIVER_TYPE piver_type, int maxlocks, int pcl_param);

/*! システムコール処理(del_mtx():mutexコントロールブロックの排除) */
ER del_mtx_isr(MTXCB *mcb);

/*! mutex初期ロックをする関数 */
void loc_first_mtx(int tskid, MTXCB *mcb);

/*! mutex再帰ロックをする関数 */
ER loc_multipl_mtx(MTXCB *mcb);

/*! mutex待ちタスクの追加 */
void wait_mtx_tsk(MTXCB *mcb, int msec);

/*! mutex初期ロック状態(mxvalueが1)以外の場合の場合の処理 */
ER not_lock_first_mtx(MTXCB *mcb);

/*! mutexオーナーシップのクリア */
void clear_mtx_ownership(MTXCB *mcb);

/*! mutex待ちからタスクをレディーへ戻す関数の分岐 */
void put_mtx_waittsk(MTXCB *mcb);

/*! mutex待ちキューからスリープTCBを抜き取る関数 */
void get_tsk_mtx_waitque(MTXCB *mcb, TCB *maxtcb);

/*! システムコール処理(loc_mtx():mutexロック処理) */
ER loc_voidmtx_isr(MTXCB *mcb);

/*! システムコール処理(ploc_mtx():mutexポーリングロック処理(プロトコルなし)) */
ER ploc_voidmtx_isr(MTXCB *mcb);

/*! システムコール処理(tloc_mtx():mutexタイムアウト付きロック処理(プロトコルなし)) */
ER tloc_voidmtx_isr(MTXCB *mcb, int msec);

/*! システムコール処理(unl_mtx():mutexアンロック処理) */
ER unl_voidmtx_isr(MTXCB *mcb);

/*! mutex(プロトコルなし)を無条件解放する(ext_tsk()またはexd_tsk()の延長で呼ばれる関数) */
ER unl_voidmtx_condy(MTXCB *mcb);


#endif
