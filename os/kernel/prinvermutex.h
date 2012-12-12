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


#ifndef _PRINVERMUTEX_H_INCLUDE_
#define _PRINVERMUTEX_H_INCLUDE_

#include "defines.h"


/*! システムコール処理(mz_loc_mtx():mutexロック処理(遅延型Higest Lockerプロトコル)) */
ER loc_dyhighmtx_isr(MTXCB *mcb);

/*! システムコール処理(mz_loc_mtx():mutexロック処理(遅延型Higest Lockerプロトコル)) */
ER unl_dyhighmtx_isr(MTXCB *mcb);

/*! システムコール処理(kz_loc_mtx():mutexロック処理(優先度継承プロトコル)) */
ER loc_inhermtx_isr(MTXCB *mcb);

/*! システムコール処理(kz_unl_mtx():mutexアンロック処理(優先度継承プロトコル)) */
ER unl_inhermtx_isr(MTXCB *mcb);

/*! システムコール処理(mz_loc_mtx():mutexロック処理(即時型Higest Lockerプロトコル)) */
ER loc_imhighmtx_isr(MTXCB *mcb);

/*! システムコール処理(mz_loc_mtx():mutexロック処理(即時型Higest Lockerプロトコル)) */
ER unl_imhighmtx_isr(MTXCB *mcb);

/*! システムコール処理(mz_loc_mtx():mutexロック処理(優先度上限プロトコル)) */
ER loc_ceilmtx_isr(MTXCB *mcb);

/*! システムコール処理(mz_loc_mtx():mutexロック処理(優先度上限プロトコル)) */
ER unl_ceilmtx_isr(MTXCB *mcb);

/*! virtual mutex待ちキューからスリープTCBを抜き取る関数 */
void get_tsk_virtual_mtx_waitque(MTXCB *mcb, TCB *maxtcb);

/*! システムコール処理(mz_loc_mtx():mutexロック処理(仮想優先度継承プロトコル)) */
ER loc_vinhermtx_isr(MTXCB *mcb);

/*! システムコール処理(mz_unl_mtx():mutexアンロック処理(仮想優先度継承プロトコル)) */
ER unl_vinhermtx_isr(MTXCB *mcb);


#endif
