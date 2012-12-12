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


#ifndef _SEMAPHORE_H_INCLUDE_
#define _SEMAPHORE_H_INCLUDE_

#include "defines.h"


/*! セマフォコントロールブロック */
typedef struct _semaphore_struct {
  struct _semaphore_struct *next;		/*! 次セマフォへのポインタ */
  struct _semaphore_struct *prev;		/*! 前セマフォへのポインタ */
  int semid; 												/*! セマフォID */
	SEM_TYPE type;										/*! セマフォのタイプ */
	SEM_ATR atr; 											/*! セマフォ属性 */
  UINT16 semvalue; 									/*! セマフォ値 */
  UINT16 maxvalue; 									/*! セマフォ最大値 */
  TCB *waithead; 										/*! セマフォ待ちタスクへの先頭ポインタ */
  TCB *waittail; 										/*! セマフォ待ちタスクへの最尾ポインタ */
} SEMCB;


/*! セマフォ情報 */
struct _semaphore_infomation {
	SEMCB **id_table; 								/*! semaphore ID変換テーブルへのheadポインタ(可変長配列として使用) */
	SEMCB *array; 										/*! 静的型semaphoreへのheadポインタ(可変長配列として使用) */
	SEMCB *freehead; 									/*! 動的型semaphoreリンクドfreeリストのキュー(freeheadポインタを記録) */
	SEMCB *alochead; 									/*! 動的型semaphoreリンクドalocリストのキュー(alocheadポインタを記録) */
/* インクリメントするのでint型で定義(short型は遅いし，ER_IDはシステムコール返却値型なのでER_IDでOS内部の領域を何か指したくないな～) */
	int counter; 											/*! 次の割付可能ID  */
	int power_count; 									/*! semaphore IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
} mg_sem_info;


/*! セマフォの初期化(semaphore ID変換テーブルの領域確保と初期化) */
ER sem_init(void);

/*! セマフォ待ちキューからスリープTCBを抜き取る関数 */
void get_tsk_sem_waitque(SEMCB *scb, TCB *maxtcb);

/*!  システムコール処理(acre_sem():セマフォコントロールブロックの作成(ID自動割付)) */
OBJP acre_sem_isr(SEM_TYPE type, SEM_ATR atr, int semvalue, int maxvalue);

/*! システムコール処理(del_sem():セマフォコントロールブロックの排除) */
ER del_sem_isr(SEMCB *scb);

/*! システムコール処理(sig_sem():セマフォV操作) */
ER sig_sem_isr(SEMCB *scb);

/*! システムコール処理(wai_sem():セマフォP操作) */
ER wai_sem_isr(SEMCB *scb);

/*! セマフォP操作(ポーリング) */
ER pol_sem_isr(SEMCB *scb);

/*! セマフォP操作(タイムアウト付き) */
ER twai_sem_isr(SEMCB *scb, int msec);

/*! セマフォを無条件解放する(ext_tsk()またはexd_tsk()の延長などで呼ばれる関数) */
ER sig_sem_condy(SEMCB *scb);


#endif
