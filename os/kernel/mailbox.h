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


#ifndef _MAILBOX_H_INCLUDE_
#define _MAILBOX_H_INCLUDE_

#include "defines.h"
#include "kernel.h"


/*! メールボックスコントロールブロック */
typedef struct _mailbox_struct {
	struct _mailbox_struct *next;		/*! 次メールボックスのポインタ */
	struct _mailbox_struct *prev;		/*! 前メールボックスのポインタ */
	int mbxid;											/*! メールボックスID */
	MBX_TYPE type;									/*! メールボックスのタイプ */
	MBX_MATR msg_atr;								/*! 各メッセージ属性 */
	MBX_WATR wai_atr;								/*! 待ちタスクをレディーへ戻す属性 */
	TMR_OBJP tobjp;									/*! 対象タイマコントロールブロックを記録(trcv_mbx()時に使用) */
	T_MSG *mkhead;									/*! メッセージキューの先頭ポインタ */
	T_MSG *mktail;									/*! メッセージキューの最尾ポインタ */
	TCB *waithead;									/*! メッセージボックス待ちタスクの先頭ポインタ */
	TCB *waittail;									/*! メッセージボックス待ちタスクの最尾ポインタ */
} MBXCB;


/*! メールボックス情報 */
struct _mailbox_infomation {
	MBXCB **id_table;								/*! mailbox ID変換テーブルへのheadポインタ(可変長配列として使用) */
	MBXCB *array;										/*! 静的型mailboxへのheadポインタ(可変長配列として使用) */
	MBXCB *freehead;								/*! 動的型mailboxリンクドfreeリストのキュー(freeheadポインタを記録) */
	MBXCB *alochead;								/*! 動的型mailboxリンクドalocリストのキュー(alocheadポインタを記録) */
	int counter;										/*! 次の割付可能ID  */
	int power_count;								/*! mailbox IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
} mg_mbx_info;


/*! メールボックスの初期化(mailbox ID変換テーブルの領域確保と初期化) */
ER mbx_init(void);

/*! メールボックス待ちキューからスリープTCBを抜き取る関数 */
void get_tsk_mbx_waitque(MBXCB *mbcb, TCB *maxtcb);

/*! システムコール処理(acre_mbx():メールボックスコントロールブロックの作成(ID自動割付)) */
OBJP acre_mbx_isr(MBX_TYPE type, MBX_MATR msg_atr, MBX_WATR wai_atr, int max_msgpri);

/*! システムコール処理(del_mbx():メールボックスコントロールブロックの排除) */
ER del_mbx_isr(MBXCB *mbcb);

/*! システムコール処理(snd_mbx():メールボックスへの送信) */
ER snd_mbx_isr(MBXCB *mbxb, T_MSG *pk_msg);

/*! システムコール処理(rcv_mbx():メールボックスから受信) */
ER rcv_mbx_isr(MBXCB *mbcb, T_MSG **pk_msg);

/*! システムコール処理(prcv_mbx():ポーリング付きメールボックスからの受信) */
ER prcv_mbx_isr(MBXCB *mbcb, T_MSG **pk_msg);

/*! システムコール処理(trcv_mbx():タイムアウト付きメールボックスからの受信) */
ER trcv_mbx_isr(MBXCB *mbcb, T_MSG **pk_msg, int tmout);


#endif
