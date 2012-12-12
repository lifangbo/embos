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


#ifndef _TIMER_MANAGE_H_INCLUDE_
#define _TIMER_MANAGE_H_INCLUDE_


#include "defines.h"


/*! 周期ハンドラコントロールブロック */
typedef struct _cycle_struct {
	struct _cycle_struct *next;		/*! 次ポインタ */
	struct _cycle_struct *prev;		/*! 前ポインタ */
	int cycid;										/*! 周期ハンドラID */
	char cycatr;									/*! 周期ハンドラ属性 */
	void *exinf;									/*! 拡張情報(周期ハンドラを呼び出す時に渡す引数) */
	int cyctim;										/*! 起動周期 */
	TMR_CALLRTE func;							/*! 周期ハンドラ起動ポインタ */
	TMR_OBJP tobjp;								/*! タイマコントロールブロックのポインタ */
} CYCCB;


/*! 周期ハンドラ情報 */
struct _cycle_handler_infomation {
	CYCCB **id_table;							/*! cycle handler ID変換テーブルへのheadポインタ(可変長配列として使用) */
	CYCCB *array;									/*! 静的型cycle handlerへのheadポインタ(可変長配列として使用) */
	CYCCB *freehead;							/*! 動的型cycle handlerリンクドfreeリストのキュー(freeheadポインタを記録) */
	CYCCB *alochead;							/*! 動的型cycle handlerリンクドalocリストのキュー(alocheadポインタを記録) */
/* インクリメントするのでint型で定義(short型は遅いし，ER_IDはシステムコール返却値型なのでER_IDでOS内部の領域を何か指したくないな～) */
	int counter;									/*! 次の割付可能ID  */
	int power_count;							/*! cycle handler IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
} mg_cyc_info;


/*! アラームハンドラコントロールブロック */
typedef struct _alarm_struct {
	struct _alarm_struct *next;		/*! 次ポインタ */
	struct _alarm_struct *prev;		/*! 前ポインタ */
	int almid;										/*! アラームハンドラID */
	char almatr;									/*! アラームハンドラ属性 */
	void *exinf;									/*! 拡張情報(アラームハンドラを呼び出す時に渡す引数) */
	TMR_CALLRTE func;							/*! アラームハンドラ起動ポインタ */
	TMR_OBJP tobjp;								/*! タイマコントロールブロックのポインタ */
} ALMCB;


/*! アラームハンドラ情報 */
struct _alarm_handler_infomation {
	ALMCB **id_table;							/*! alarm handler ID変換テーブルへのheadポインタ(可変長配列として使用) */
	ALMCB *array;									/*! 静的型alarm handlerへのheadポインタ(可変長配列として使用) */
	ALMCB *freehead;							/*! 動的型alarm handlerリンクドfreeリストのキュー(freeheadポインタを記録) */
	ALMCB *alochead;							/*! 動的型alarm handlerリンクドalocリストのキュー(alocheadポインタを記録) */
/* インクリメントするのでint型で定義(short型は遅いし，ER_IDはシステムコール返却値型なのでER_IDでOS内部の領域を何か指したくないな～) */
	int counter;									/*! 次の割付可能ID  */
	int power_count;							/*! alarm handler IDが足らなくなった回数(<<で倍増やしていく).この変数で可変長配列のサイズが求められる */
} mg_alm_info;


/*! 周期ハンドラの初期化(cycle handler ID変換テーブルの領域確保と初期化) */
ER cyc_init(void);

/*! システムコール処理(acre_cyc():周期ハンドラコントロールブロックの作成(ID自動割付)) */
OBJP acre_cyc_isr(CYC_TYPE type, void *exinf, int cyctim, TMR_CALLRTE func);

/*! システムコール処理(del_cyc():周期ハンドラコントロールブロックの排除) */
ER del_cyc_isr(CYCCB *cycb);

/*! システムコール処理(sta_cyc():周期ハンドラの動作開始) */
ER sta_cyc_isr(CYCCB *cycb);

/*! システムコール処理(stp_cyc():周期ハンドラの動作停止) */
ER stp_cyc_isr(CYCCB *cycb);

/*! アラームハンドラの初期化(alarm handler ID変換テーブルの領域確保と初期化) */
ER alm_init(void);

/*! システムコール処理(acre_alm():アラームハンドラコントロールブロックの作成(ID自動割付)) */
OBJP acre_alm_isr(ALM_TYPE type, void *exinf, TMR_CALLRTE func);

/*! システムコール処理(del_dalm():動的アラームハンドラコントロールブロックの排除) */
ER del_alm_isr(ALMCB *acb);

/*!  システムコール処理(sta_dalm():アラームハンドラの動作開始) */
ER sta_alm_isr(ALMCB *acb, int msec);

/*! システムコール処理(stp_dalm():動的アラームの動作停止) */
ER stp_alm_isr(ALMCB *acb);


#endif
