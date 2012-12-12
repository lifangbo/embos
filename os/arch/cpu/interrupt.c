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


#include "intr.h"
#include "interrupt.h"


/*!
* ソフトウエア割込みベクタの設定
* type : ソフトウェア割込みベクタの種類
* handler : ソフトウェア割込みベクタに登録する関数ポインタ
*/
void softvec_setintr(SOFTVEC type, SOFTVEC_HANDL handler)
{
  SOFTVECS[type] = handler;
}
