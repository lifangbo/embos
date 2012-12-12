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


#ifndef _SERIAL_DRIVER_H_INCLUDE_
#define _SERIAL_DRIVER_H_INCLUDE_

#include "kernel/defines.h"


/*! デバイス初期化 */
void serial_init(int index);

/*! １文字送信 */
int serial_send_byte(int index, unsigned char b);

/*! １文字受信 */
unsigned char serial_recv_byte(int index);

/*! 送信割込み有効か？ */
BOOL serial_intr_is_send_enable(int index);

/*! 送信割込み有効化 */
void serial_intr_send_enable(int index);

/*! 送信割込み無効化 */
void serial_intr_send_disable(int index);

/*! 受信割込み有効か？ */
BOOL serial_intr_is_recv_enable(int index);

/*! 受信割込み有効化 */
void serial_intr_recv_enable(int index);

/*! 受信割込み無効化 */
void serial_intr_recv_disable(int index);


#endif
