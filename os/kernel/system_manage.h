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


#ifndef _SYSTEM_MANAGE_H_INCLUDE_
#define _SYSTEM_MANAGE_H_INCLUDE_


#include "defines.h"


/*! システム構造体 */
struct system_infomation {
	ROL_ATR atr; /*! システムロールバック属性 */
} mg_sys_info;


/*! システムコールの処理(rot_rdp():タスクの優先順位回転) */
ER rot_rdq_isr(int tskpri);

/*! システムコールの処理(kz_get_id():スレッドID取得) */
ER_ID get_tid_isr(void);

/*! システムコールの処理(kz_dis_dsp():ディスパッチの禁止) */
void dis_dsp_isr(void);

/*! システムコールの処理(kz_ena_dsp():ディスパッチの許可) */
void ena_dsp_isr(void);

/*! システムコールの処理(kz_sns_isr():ディスパッチの状態参照) */
BOOL sns_dsp_isr(void);

/*! システムコールの処理(kz_set_pow():省電力モード設定) */
void set_pow_isr(void);

/*! システムコールの処理(rol_sys():システムをinitタスク生成ルーチンへロールバック) */
ER rol_sys_isr(ROL_ATR atr);

/*! init生成ルーチンへロールバックさせる関数 */
void rollback_system(void);


#endif
