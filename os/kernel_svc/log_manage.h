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


#ifndef _LOG_MANAGE_H_INCLUDED_
#define _LOG_MANAGE_H_INCLUDED_


#include "kernel/defines.h"


/* ログの数 */
UINT32 log_counter;

/*! ログ管理機構の初期化 */
void log_mechanism_init(void);

/*! コンテキストスイッチングのログの出力(Linux jsonログを参考(フォーマット変換は行わない)) */
void get_log(OBJP log_tcb);


#endif
