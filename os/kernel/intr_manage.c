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


#include "kernel.h"
#include "c_lib/lib.h"
#include "arch/cpu/interrupt.h"
#include "arch/cpu/intr.h"
#include "intr_manage.h"


/*!
* システムコールの処理(def_inh():割込みハンドラの定義)
* 属性はない．また登録できる割込みハンドラはシリアル割込みハンドラのみ
* ただし，initタスクだけはシステムコールハンドラ及びソフトウェアエラー(例外)，
* タイマ割込みハンドラ登録ができる．そのためE_ILUSEエラーを追加した
* type : ソフトウェア割込みベクタ番号
* handler : 登録するハンドラポインタ
* (返却値)E_ILUSE : 不正使用
* (返却値)E_PAR : パラメータエラー
* (返却値)E_OK : 登録完了
*/
ER def_inh_isr(SOFTVEC type, IR_HANDL handler)
{
	/* パラメータは正しいか */
	if (type < 0 || SOFTVEC_TYPE_NUM <= type || handler == NULL) {
		return E_PAR;
	}
	/* initタスク時でのソフトウェア割込みベクタへ割込みハンドラ定義 */
	else if (mg_tsk_info.id_table[INIT_TASK_ID] == NULL) {
		softvec_setintr(type, thread_intr);
  	handlers[type] = handler;
		return E_OK;
	}
	/* initタスク以外でのソフトウェア割込みベクタへ割込みハンドラを定義 */
	else {
		/* シリアル割込みハンドラか */
		if (type == SOFTVEC_TYPE_SERINTR) {
  		softvec_setintr(type, thread_intr);
  		handlers[type] = handler;
			return E_OK;
		}
		/* それ以外は不正使用 */
		else {
			return E_ILUSE;
		}
	}
}
