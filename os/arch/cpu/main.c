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


#include "kernel/defines.h"
#include "kernel/command.h"
#include "kernel/kernel.h"
#include "interrupt.h"
#include "c_lib/lib.h"
#include "target/driver/serial_driver.h"
#include "intr.h"


/*! シリアル割込みハンドラ(ユーザは定義できる) */
static void serial_intr(void);


/*! 資源ID */
ER_ID idle_id;
#ifdef TSK_LIBRARY
ER_ID sample_tsk1_id;
ER_ID sample_tsk2_id;
ER_ID sample_tsk3_id;
ER_ID sample_tsk4_id;
ER_ID sample_tsk5_id;
ER_ID sample_tsk6_id;
ER_ID sample_tsk7_id;
ER_ID sample_tsk8_id;
ER_ID sample_tsk9_id;
ER_ID sample_tsk10_id;
ER_ID sample_tsk11_id;
ER_ID sample_tsk12_id;
ER_ID sample_tsk13_id;
ER_ID sample_tsk14_id;
ER_ID sample_tsk15_id;
ER_ID sample_tsk16_id;
ER_ID sample_tsk17_id;
ER_ID sample_tsk18_id;
ER_ID sample_tsk19_id;
ER_ID sample_tsk20_id;
ER_ID sample_tsk21_id;
ER_ID sample_tsk22_id;
ER_ID sample_tsk23_id;
ER_ID sample_tsk24_id;
ER_ID sample_tsk25_id;
ER_ID sample_tsk26_id;
ER_ID sample_tsk27_id;
ER_ID sample_tsk28_id;
ER_ID sample_tsk29_id;
ER_ID sample_tsk30_id;
ER_ID sample_tsk31_id;
ER_ID sample_tsk32_id;
ER_ID sample_tsk33_id;
ER_ID sample_tsk34_id;
ER_ID sample_tsk35_id;
ER_ID sample_tsk36_id;
ER_ID sample_tsk37_id;
ER_ID sample_tsk38_id;
ER_ID sample_tsk39_id;
ER_ID sample_tsk40_id;
ER_ID sample_tsk41_id;
ER_ID sample_tsk42_id;
ER_ID sample_tsk43_id;
ER_ID sample_tsk44_id;
ER_ID sample_tsk45_id;
ER_ID sample_tsk46_id;
ER_ID sample_tsk47_id;
ER_ID sample_tsk48_id;
ER_ID sample_tsk49_id;
ER_ID sample_tsk50_id;
ER_ID sample_tsk51_id;
ER_ID sample_tsk52_id;
ER_ID sample_tsk53_id;
ER_ID sample_tsk54_id;
ER_ID sample_tsk55_id;
ER_ID sample_tsk56_id;
ER_ID sample_tsk57_id;
ER_ID sample_tsk58_id;
ER_ID sample_tsk59_id;
ER_ID sample_tsk60_id;
ER_ID sample_tsk61_id;
ER_ID sample_tsk62_id;
ER_ID sample_tsk63_id;
ER_ID sample_tsk64_id;
ER_ID sample_tsk65_id;
ER_ID sample_tsk66_id;
ER_ID sample_tsk67_id;
ER_ID sample_tsk68_id;
ER_ID sample_tsk69_id;
ER_ID sample_tsk70_id;
ER_ID sample_tsk71_id;
ER_ID sample_tsk72_id;
ER_ID sample_tsk73_id;
ER_ID sample_tsk74_id;
ER_ID sample_tsk75_id;
ER_ID sample_tsk76_id;
ER_ID sample_tsk77_id;
ER_ID sample_tsk78_id;
ER_ID sample_tsk79_id;
ER_ID sample_tsk80_id;
ER_ID sample_tsk81_id;
ER_ID sample_tsk82_id;
ER_ID sample_tsk83_id;
ER_ID sample_tsk84_id;
ER_ID sample_tsk85_id;
ER_ID sample_tsk86_id;
ER_ID sample_tsk87_id;
ER_ID sample_tsk88_id;
ER_ID sample_tsk89_id;
ER_ID sample_tsk90_id;
ER_ID sample_tsk91_id;
ER_ID sample_tsk92_id;
ER_ID sample_tsk93_id;
ER_ID sample_tsk94_id;
ER_ID sample_tsk95_id;
ER_ID sample_tsk96_id;
ER_ID sample_tsk97_id;
ER_ID sample_tsk98_id;
ER_ID sample_tsk99_id;
ER_ID sample_tsk100_id;
ER_ID sample_tsk101_id;
ER_ID sample_tsk102_id;
ER_ID sample_tsk103_id;
ER_ID sample_tsk104_id;
ER_ID sample_tsk105_id;
ER_ID sample_tsk106_id;
ER_ID sample_tsk107_id;
ER_ID sample_tsk108_id;
ER_ID sample_tsk109_id;
ER_ID sample_tsk110_id;
ER_ID sample_tsk111_id;
ER_ID sample_tsk112_id;
ER_ID sample_tsk113_id;
ER_ID sample_tsk138_id;
ER_ID sem0_id;
ER_ID mtx0_id;
ER_ID mtx1_id;
ER_ID cyc0_id;
ER_ID cyc1_id;
ER_ID cyc2_id;
ER_ID cyc3_id;
ER_ID cyc4_id;
ER_ID cyc5_id;
ER_ID alm0_id;
ER_ID alm1_id;
ER_ID alm2_id;

#endif


/*! initタスク */
int start_threads(int argc, char *argv[])
{
	KERNEL_OUTMSG("init task started.\n");

  serial_intr_recv_enable(SERIAL_DEFAULT_DEVICE);
  mz_def_inh(SOFTVEC_TYPE_SERINTR, serial_intr);

	INTR_ENABLE;
	while (1) {
		;
  }
  
  return 0;
}


/*! OSメイン関数 */
int main(void)
{
  INTR_DISABLE; /* 割込み無効にする */

  KERNEL_OUTMSG("kernel boot OK!\n");

  /* OSの動作開始 */
	start_init_tsk(start_threads, "idle", 0, 0x100, 0, NULL); /* initタスク起動 */
  
  /* 正常ならばここには戻ってこない */

  return 0;
}



/*! シリアル割込みハンドラ(ユーザは定義できる) */
static void serial_intr(void)
{
  int c;
  static char buf[32];
  static int len;

  c = getc();

  if (c != '\n') {
    buf[len++] = c;
  }
  else {
    buf[len++] = '\0';
		/* echoコマンドの場合 */
    if (!strncmp(buf, "echo ", 5)) {
      echo_command(buf); /* echoコマンド(標準出力にテキストを出力する)呼び出し */
    }
		/* helpコマンドの場合 */
    else if (!strncmp(buf, "help", 4)) {
     	help_command(&buf[4]); /* helpコマンド呼び出し */
    }
#ifdef TSK_LIBRARY
		/* runコマンドの場合 */
		else if (!strncmp(buf, "run", 3)) {
			run_command(&buf[3]); /* runコマンド(タスクセットの起動)呼び出し */
		}
#endif
		/* sendlogの場合 */
		else if (!strncmp(buf, "sendlog", 7)) {
      sendlog_command(); /* sendlogコマンド(xmodem送信モード)呼び出し */
		}
		else if (! strncmp(buf, "set", 3)) {
			set_command(&buf[3]);
		}
		/* 本システムに存在しないコマンド */
    else {
      puts("command unknown.\n");
    }
    puts("> ");
    len = 0;
  }
}
