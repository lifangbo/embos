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


#include "defines.h"
#include "command.h"
#include "kernel.h"
#include "scheduler.h"
#include "syscall.h"
#include "net/xmodem.h"
#include "kernel_svc/log_manage.h"


typedef struct _dummy_logbuf {
	int tskid;
	char mem;
	int time;
} DLOG_B;

DLOG_B dummy_array[20];


#ifdef TSK_LIBRARY

/*! tsk_set1の起動 */
static void tsk_set1_command(void);

/*! tsk_set2の起動 */
static void tsk_set2_command(void);

/*! tsk_set3の起動 */
static void tsk_set3_command(void);

/*! tsk_set4の起動 */
static void tsk_set4_command(void);

/*! tsk_set5の起動 */
static void tsk_set5_command(void);

/*! tsk_set6の起動 */
static void tsk_set6_command(void);

/*! tsk_set7の起動 */
static void tsk_set7_command(void);

/*! tsk_set8の起動 */
static void tsk_set8_command(void);

/*! tsk_set9の起動 */
static void tsk_set9_command(void);

/*! tsk_set10の起動 */
static void tsk_set10_command(void);

/*! tsk_set11の起動 */
static void tsk_set11_command(void);

/*! tsk_set12の起動 */
static void tsk_set12_command(void);

/*! tsk_set13の起動 */
static void tsk_set13_command(void);

/*! tsk_set14の起動 */
static void tsk_set14_command(void);

/*! tsk_set15の起動 */
static void tsk_set15_command(void);

/*! tsk_set16の起動 */
static void tsk_set16_command(void);

/*! tsk_set17の起動 */
static void tsk_set17_command(void);

/*! tsk_set18の起動 */
static void tsk_set18_command(void);

/*! tsk_set19の起動 */
static void tsk_set19_command(void);

/*! tsk_set20の起動 */
static void tsk_set20_command(void);

/*! tsk_set21の起動 */
static void tsk_set21_command(void);

/*! tsk_set22の起動 */
static void tsk_set22_command(void);

/*! tsk_set23の起動 */
static void tsk_set23_command(void);

/*! tsk_set24の起動 */
static void tsk_set24_command(void);

/*! tsk_set25の起動 */
static void tsk_set25_command(void);

/*! tsk_set26の起動 */
static void tsk_set26_command(void);

/*! tsk_set27の起動 */
static void tsk_set27_command(void);

/*! tsk_set28の起動 */
static void tsk_set28_command(void);

/*! tsk_set29の起動 */
static void tsk_set29_command(void);

/*! tsk_set30の起動 */
static void tsk_set30_command(void);

/*! tsk_set31の起動 */
static void tsk_set31_command(void);

/*! tsk_set32の起動 */
static void tsk_set32_command(void);

/*! tsk_set33の起動 */
static void tsk_set33_command(void);

/*! tsk_set34の起動 */
static void tsk_set34_command(void);

/*! tsk_set35の起動 */
static void tsk_set35_command(void);

/*! tsk_set36の起動 */
static void tsk_set36_command(void);

/*! tsk_set37の起動 */
static void tsk_set37_command(void);

/*! tsk_set38の起動 */
static void tsk_set38_command(void);

/*! tsk_set39の起動 */
static void tsk_set39_command(void);

/*! tsk_set40の起動 */
static void tsk_set40_command(void);

/*! tsk_set41の起動 */
static void tsk_set41_command(void);

/*! tsk_set42の起動 */
static void tsk_set42_command(void);

/*! tsk_set43の起動 */
static void tsk_set43_command(void);

/*! tsk_set44の起動 */
static void tsk_set44_command(void);

/*! tsk_set45の起動 */
static void tsk_set45_command(void);

/*! tsk_set46の起動 */
static void tsk_set46_command(void);

/*! tsk_set47の起動 */
static void tsk_set47_command(void);

/*! tsk_set48の起動 */
static void tsk_set48_command(void);

/*! tsk_set49の起動 */
static void tsk_set49_command(void);

/*! tsk_set50の起動 */
static void tsk_set50_command(void);

/*! tsk_set51の起動 */
static void tsk_set51_command(void);

/*! tsk_set52の起動 */
static void tsk_set52_command(void);

/*! tsk_set53の起動 */
static void tsk_set53_command(void);

/*! tsk_set54の起動 */
static void tsk_set54_command(void);

void tsk_set44_msg(void);

void tsk_set46_msg(void);

void tsk_set47_msg(void);

void tsk_set48_msg(void);

void tsk_set49_msg(void);

void tsk_set50_msg(void);

void tsk_set51_msg(void);

void tsk_set52_msg(void);

void tsk_set53_msg(void);

#endif


/*! リンカのシンボルを参照 */
extern UINT32 logbuffer_start;


/*!
 * echoコマンド
 * buf[] : 標準出力するバッファ
 */
void echo_command(char buf[])
{
	puts(buf + 5);
  puts("\n");
}


/*!
 * helpコマンド
 * *buf : 標準出力するバッファポインタ
 */
void help_command(char *buf)
{
	/* helpメッセージ */
	if (*buf == '\0') {
    puts("echo    - out text serial line.\n");
    puts("sendlog - send log file over serial line(xmodem mode)\n");
    puts("run     - run task sets.\n");
  }
	/* echo helpメッセージ */
  else if (!strncmp(buf, " echo", 5)) {
		puts("echo - out text serial line\n\n");
		puts("Usage:\n");
		puts("echo [arg...]\n");
		puts("  out [arg...] serial line\n");
  }
	/* sendlog helpメッセージ */
  else if (!strncmp(buf, " sendlog", 8)) {
		puts("sendlog - send log file over serial line(xmodem mode)\n");
  }
#ifdef TSK_LIBRARY
	/* run helpメッセージ */
  else if (!strncmp(buf, " run", 4)) {
    /* run help */
		puts("run - run task sets.\n\n");
		puts("Usage:\n");
		puts("run <tsk_set>\n");
		puts("  start the <tsk_set> that is specified in the argument\n");
  }
	/* tsk_set4 helpメッセージ */
	else if (!strncmp(buf, " tsk_set4", 9)) {
		puts("tsk_set4 - This sample task sets the \"TASK SYNCHRONIZATION SYSTEMCALLS\"\n\n");
		puts("Using system call:\n");
		puts("  acre_tsk(), sta_tsk(), ter_tsk(), tslp_tsk(), dly_tsk()\n");
	}
	/* tsk_set3 helpメッセージ */
	else if (!strncmp(buf, " tsk_set3", 9)) {
		puts("tsk_set3 - This sample task sets the \"TASK SYNCHRONIZATION SYSTEMCALLS\"\n\n");
		puts("Using system call:\n");
		puts("  acre_tsk(), sta_tsk(), slp_tsk(), wup_tsk(), rel_wai()\n");
	}
	/* tsk_set2 helpメッセージ */
	else if (!strncmp(buf, " tsk_set2", 9)) {
		puts("tsk_set2 - This sample task sets the \"TASK MANAGE SYSTEMCALLS\"\n\n");
		puts("Using system call:\n");
		puts("  run_tsk(), get_pri(), chg_pri()\n");
	}
	/* tsk_set1 helpメッセージ */
	else if (!strncmp(buf, " tsk_set1", 9)) {
		puts("tsk_set1 - This sample task sets the \"TASK MANAGE SYSTEMCALLS\"\n\n");
		puts("Using system call:\n");
		puts("  acre_tsk(), sta_tsk(), del_tsk(), ext_tsk(), exd_tsk(), ter_tsk()\n");
	}
	/* tsk_set 一覧 */
  else if (!strncmp(buf, " tsk_set", 8)) {
		puts("tsk_set1 - This sample task sets the \"TASK MANAGE SYSTEMCALLS\"\n");
		puts("tsk_set2 - This sample task sets the \"TASK MANAGE SYSTEMCALLS\"\n");
		puts("tsk_set3 - This sample task sets the \"TASK SYNCHRONIZATION SYSTEMCALLS\"\n");
		puts("tsk_set4 - This sample task sets the \"TASK SYNCHRONIZATION SYSTEMCALLS\"\n");
		puts("tsk_set4 - This sample task sets the \"TASK SYNCHRONIZATION SYSTEMCALLS\"\n");
		puts("tsk_set5 - This sample task sets the \"kernel automatic release\"\n");
		puts("tsk_set6 - This sample task sets the \"cycle task control by cycle handler\"\n");
		puts("tsk_set7 - This sample task sets the \"check polling by cycle handler\"\n");
		puts("tsk_set8 - This sample task sets the \"use of alarm handler\"\n");
		puts("tsk_set9 - This sample task sets the \"task synchronization using semaphore\"\n");
		puts("tsk_set10 - This sample task sets the \"task credit synchronization using semaphore\"\n");
		puts("tsk_set11 - This sample task sets the \"exclusive control using the semaphore\"\n");
		puts("tsk_set12 - This sample task sets the \"general exclusion control using semaphore\"\n");
		puts("tsk_set13 - This sample task sets the \"problem producer / consumer\"\n");
		puts("tsk_set14 - This sample task sets the \"communication tasks using the mailbox\"\n");
		puts("tsk_set15 - This sample task sets the \"exclusive control using the enable / disable of the dispatch\"\n");
		puts("tsk_set16 - This sample task sets the \"exclusive control using the lock free protocol\"\n");
		puts("tsk_set17 - This sample task sets the \"using a mutex deadlock\"\n");
		puts("tsk_set18 - This sample task sets the \"deadlock avoidance using a mutex with polling\"\n");
		puts("tsk_set19 - This sample task sets the \"deadlock avoidance using a mutex with timeout\"\n");
		puts("tsk_set20 - This sample task sets the \"priority inversion using a mutex\"\n");
		puts("tsk_set21 - This sample task sets the \"scenario in an environment that induces deadlock priority inheritance protocol(using void protocol)\"\n");
		puts("tsk_set22 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using void protocol)\"\n");
		puts("tsk_set23 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using void protocol)\"\n");
		puts("tsk_set24 - This sample task sets the \"scenario in an environment that induces deadlock priority inheritance protocol(using priority inheritance protocol)\"\n");
		puts("tsk_set25 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using priority inheritance protocol)\"\n");
		puts("tsk_set26 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using priority inheritance protocol)\"\n");
		puts("tsk_set27 - This sample task sets the \"scenario in an environment that induces deadlock priority inheritance protocol(using immediate highest locker protocol)\"\n");
		puts("tsk_set28 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using immediate highest locker protocol)\"\n");
		puts("tsk_set29 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using immediate highest locker protocol)\"\n");
		puts("tsk_set30 - This sample task sets the \"scenario in an environment that induces deadlock priority inheritance protocol(using delay highest locker protocol)\"\n");
		puts("tsk_set31 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using delay highest locker protocol)\"\n");
		puts("tsk_set32 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using delay highest locker protocol)\"\n");
		puts("tsk_set33 - This sample task sets the \"scenario in an environment that induces deadlock priority inheritance protocol(using priority ceiling protocol)\"\n");
		puts("tsk_set34 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using priority celling protocol)\"\n");
		puts("tsk_set35 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using priority celling protocol)\"\n");
		puts("tsk_set36 - This sample task sets the \"scenario for the prevention og deadlock in an environment(using priority celling protocol)\"\n");
		puts("tsk_set37 - This sample task sets the \"scenario in an environment that induces deadlock priority inheritance protocol(using stack resource policy)\"\n");
		puts("tsk_set38 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using stack resource policy)\"\n");
		puts("tsk_set39 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using stack resource policy)\"\n");
		puts("tsk_set40 - This sample task sets the \"scenario for the prevention og deadlock in an environment(using virtual priority inheritance)\"\n");
		puts("tsk_set41 - This sample task sets the \"scenario that blocking chain environment priority inheritance protocol(using virtual priority inheritance)\"\n");
		puts("tsk_set42 - This sample task sets the \"scenario that continuous blocking priority inheritance protocol environment(using virtual priority inheritance)\"\n");
		puts("tsk_set43 - This sample task sets the \"scenario for the prevention og deadlock in an environment(using virtual priority inheritance)\"\n");
  }
#endif
	/* helpに存在しないコマンド */
	else {
		puts("help unknown.\n");
	}
}


static void dummy_array_init(void)
{
	int i;

	for (i = 0; i < 20; i++) {
		dummy_array[i].tskid = i;
		dummy_array[i].mem = 'a';
		dummy_array[i].time = i + 1;
	}
}


/*! sendlogコマンド */
void sendlog_command(void)
{
	//UINT32 *logbuf;
	//logbuf = (UINT32 *)&logbuffer_start;

	dummy_array_init();

	/* ログをxmodemで送信して正常の場合 */
	//if (send_xmodem((UINT8 *)logbuf, *logbuf + 4)) { /* +4はログ管理機構の先頭番地(サイズ格納位置分) */
	if (send_xmodem((UINT8 *)dummy_array, sizeof(DLOG_B) * 20)) {
		puts("log to xmodem OK.\n");
	}
	/* エラーの場合 */
	else {
		puts("log to xmodem error.\n");
	}
}


void set_command(char *buf)
{
	if (!strncmp(buf, " FCFS", 5)) {
		write_schdul(FCFS_SCHEDULING, 0);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " RR", 3)) {
		write_schdul(RR_SCHEDULING, 1000);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " PRI", 4)) {
		write_schdul(PRI_SCHEDULING, 0);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " RR_PRI", 7)) {
		write_schdul(RR_PRI_SCHEDULING, 1000);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " MFQ", 4)) {
		write_schdul(MFQ_SCHEDULING, 100);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " ODRONE", 7)) {
		write_schdul(ODRONE_SCHEDULING, 100);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " FR", 3)) {
		write_schdul(FR_SCHEDULING, 1000);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " PFR", 4)) {
		write_schdul(PFR_SCHEDULING, 1000);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " RM", 3)) {
		write_schdul(RM_SCHEDULING, 0);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " DM", 3)) {
		write_schdul(DM_SCHEDULING, 0);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " EDF", 4)) {
		write_schdul(EDF_SCHEDULING, 0);
		schdul_init();
		ready_init();
	}
	else if (!strncmp(buf, " LLF", 4)) {
		write_schdul(LLF_SCHEDULING, 0);
		schdul_init();
		ready_init();
	}
	else {
		puts("unkown scheduling.\n");
	}
}


#ifdef TSK_LIBRARY

/*!
 * runコマンド
 * *buf : 起動するタスクセット名が格納されたバッファ
 */
void run_command(char *buf)
{
	if (!strncmp(buf, " tsk_set54", 11)) {
		tsk_set54_command();
	}
	else if (!strncmp(buf, " tsk_set53", 11)) {
		tsk_set53_command();
	}
	else if (!strncmp(buf, " tsk_set52", 11)) {
		tsk_set52_command();
	}
	else if (!strncmp(buf, " tsk_set51", 11)) {
		tsk_set51_command();
	}
	else if (!strncmp(buf, " tsk_set50", 11)) {
		tsk_set50_command();
	}
	else if (!strncmp(buf, " tsk_set49", 11)) {
		tsk_set49_command();
	}
	else if (!strncmp(buf, " tsk_set48", 11)) {
		tsk_set48_command();
	}
	else if (!strncmp(buf, " tsk_set47", 11)) {
		tsk_set47_command();
	}
	else if (!strncmp(buf, " tsk_set46", 11)) {
		tsk_set46_command();
	}
	else if (!strncmp(buf, " tsk_set45", 11)) {
		tsk_set45_command();
	}
	else if (!strncmp(buf, " tsk_set44", 11)) {
		tsk_set44_command();
	}
	else if (!strncmp(buf, " tsk_set43", 11)) {
		tsk_set43_command();
	}
	else if (!strncmp(buf, " tsk_set42", 11)) {
		tsk_set42_command();
	}
	else if (!strncmp(buf, " tsk_set41", 11)) {
		tsk_set41_command();
	}
	else if (!strncmp(buf, " tsk_set40", 11)) {
		tsk_set40_command();
	}
	else if (!strncmp(buf, " tsk_set39", 11)) {
		tsk_set39_command();
	}
	else if (!strncmp(buf, " tsk_set38", 11)) {
		tsk_set38_command();
	}
	else if (!strncmp(buf, " tsk_set37", 11)) {
		tsk_set37_command();
	}
	else if (!strncmp(buf, " tsk_set36", 11)) {
		tsk_set36_command();
	}
	else if (!strncmp(buf, " tsk_set35", 11)) {
		tsk_set35_command();
	}
	else if (!strncmp(buf, " tsk_set34", 11)) {
		tsk_set34_command();
	}
	else if (!strncmp(buf, " tsk_set33", 11)) {
		tsk_set33_command();
	}
	else if (!strncmp(buf, " tsk_set32", 11)) {
		tsk_set32_command();
	}
	else if (!strncmp(buf, " tsk_set31", 11)) {
		tsk_set31_command();
	}
	else if (!strncmp(buf, " tsk_set30", 11)) {
		tsk_set30_command();
	}
	else if (!strncmp(buf, " tsk_set29", 11)) {
		tsk_set29_command();
	}
	else if (!strncmp(buf, " tsk_set28", 11)) {
		tsk_set28_command();
	}
	else if (!strncmp(buf, " tsk_set27", 11)) {
		tsk_set27_command();
	}
	else if (!strncmp(buf, " tsk_set26", 11)) {
		tsk_set26_command();
	}
	else if (!strncmp(buf, " tsk_set25", 11)) {
		tsk_set25_command();
	}
	else if (!strncmp(buf, " tsk_set24", 11)) {
		tsk_set24_command();
	}
  else if (!strncmp(buf, " tsk_set23", 11)) {
		tsk_set23_command();
	}
  else if (!strncmp(buf, " tsk_set22", 11)) {
		tsk_set22_command();
	}
  else if (!strncmp(buf, " tsk_set21", 11)) {
		tsk_set21_command();
	}
  else if (!strncmp(buf, " tsk_set20", 11)) {
		tsk_set20_command();
	}
  else if (!strncmp(buf, " tsk_set19", 11)) {
		tsk_set19_command();
	}
  else if (!strncmp(buf, " tsk_set18", 11)) {
		tsk_set18_command();
	}
 	else if (!strncmp(buf, " tsk_set17", 11)) {
		tsk_set17_command();
	}
 	else if (!strncmp(buf, " tsk_set16", 11)) {
		tsk_set16_command();
	}
 	else if (!strncmp(buf, " tsk_set15", 11)) {
		tsk_set15_command();
	}
 	else if (!strncmp(buf, " tsk_set14", 11)) {
		tsk_set14_command();
	}
 	else if (!strncmp(buf, " tsk_set13", 11)) {
		tsk_set13_command();
	}
  else if (!strncmp(buf, " tsk_set12", 11)) {
		tsk_set12_command();
	}
  else if (!strncmp(buf, " tsk_set11", 11)) {
		tsk_set11_command();
	}
  else if (!strncmp(buf, " tsk_set10", 11)) {
   	tsk_set10_command();
 	}
	else if (!strncmp(buf, " tsk_set9", 10)) {
		tsk_set9_command();
	}
	else if (!strncmp(buf, " tsk_set8", 10)) {
		tsk_set8_command();
	}
	else if (!strncmp(buf, " tsk_set7", 10)) {
		tsk_set7_command();
	}
	else if (!strncmp(buf, " tsk_set6", 10)) {
		tsk_set6_command();
	}
  else if (!strncmp(buf, " tsk_set5", 10)) {
		tsk_set5_command();
	}
	else if (!strncmp(buf, " tsk_set4", 10)) {
		tsk_set4_command();
	}
  else if (!strncmp(buf, " tsk_set3", 10)) {
		tsk_set3_command();
	}
  else if (!strncmp(buf, " tsk_set2", 10)) {
		tsk_set2_command();
	}
  else if (!strncmp(buf, " tsk_set1", 10)) {
		tsk_set1_command();
	}
 	else {
		puts("tsk_set unknown.\n");
	}
}


/*! tsk_set1の起動 */
static void tsk_set1_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk1_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk1";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk3_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk3_id);
}


/*! tsk_set2の起動 */
static void tsk_set2_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk4_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk4";
	tsk1_param.un.acre_tsk.priority = 3;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk4_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk4_id);
}


/*! tsk_set3の起動 */
static void tsk_set3_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk6_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk6";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk6_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk6_id);
}


/*! tsk_set4の起動 */
static void tsk_set4_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk9_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk9";
	tsk1_param.un.acre_tsk.priority = 3;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk9_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk9_id);
}


/*! tsk_set5の起動 */
static void tsk_set5_command(void)
{
  SYSCALL_PARAMCB tsk1_param;
  
  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk11_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk11";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk11_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk11_id);
}


/*! tsk_set6の起動 */
static void tsk_set6_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk12_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk12";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk12_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk12_id);
}


/*! tsk_set7の起動 */
static void tsk_set7_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk14_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk14";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk14_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk14_id);
}


/*! tsk_set8の起動 */
static void tsk_set8_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk15_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk15";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk15_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk15_id);
}


/*! tsk_set9の起動 */
static void tsk_set9_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk16_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk16";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk16_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk16_id);
}


/*! tsk_set10の起動 */
static void tsk_set10_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk18_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk18";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk18_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk18_id);
}


/*! tsk_set11の起動 */
static void tsk_set11_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk21_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk21";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk21_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk21_id);
}


/*! tsk_set12の起動 */
static void tsk_set12_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk23_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk23";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk23_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk23_id);
}


/*! tsk_set13の起動 */
static void tsk_set13_command(void)
{
  puts("coming soon.\n");
}


/*! tsk_set14の起動 */
static void tsk_set14_command(void)
{
  puts("coming soon.\n");
}


/*! tsk_set15の起動 */
static void tsk_set15_command(void)
{
	SYSCALL_PARAMCB tsk1_param;
  
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk30_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk30";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk30_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk30_id);
}


/*! tsk_set16の起動 */
static void tsk_set16_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk32_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk32";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk32_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk32_id);
}


/*! tsk_set17の起動 */
static void tsk_set17_command(void)
{
	SYSCALL_PARAMCB tsk1_param;
  
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk33_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk33";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
 	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk33_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk33_id);
}


/*! tsk_set18の起動 */
static void tsk_set18_command(void)
{
  SYSCALL_PARAMCB tsk1_param;
        
	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk35_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk35";
	tsk1_param.un.acre_tsk.priority = 3;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk35_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk35_id);
}


/*! tsk_set19の起動 */
static void tsk_set19_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

  tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk37_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk37";
	tsk1_param.un.acre_tsk.priority = 3;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk37_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk37_id);
}


/*! tsk_set20の起動 */
static void tsk_set20_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk39_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk39";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk39_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk39_id);
}


/*! tsk_set21の起動 */
static void tsk_set21_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk42_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk42";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk42_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk42_id);
}


/*! tsk_set22の起動 */
static void tsk_set22_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk45_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk45";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk45_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk45_id);
}


/*! tsk_set23の起動 */
static void tsk_set23_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk48_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk48";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk48_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk48_id);
}


/*! tsk_set24の起動 */
static void tsk_set24_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk51_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk51";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk51_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk51_id);
}


/*! tsk_set25の起動 */
static void tsk_set25_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk54_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk54";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk54_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk54_id);
}


/*! tsk_set26の起動 */
static void tsk_set26_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk57_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk57";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk57_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk57_id);
}


/*! tsk_set27の起動 */
static void tsk_set27_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk60_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk60";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk60_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk60_id);
}


/*! tsk_set28の起動 */
static void tsk_set28_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk63_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk63";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk63_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk63_id);
}


/*! tsk_set29の起動 */
static void tsk_set29_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk66_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk66";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk66_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk66_id);
}


/*! tsk_set30の起動 */
static void tsk_set30_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk69_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk69";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk69_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk69_id);
}


/*! tsk_set31の起動 */
static void tsk_set31_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk72_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk72";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk72_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk72_id);
}


/*! tsk_set32の起動 */
static void tsk_set32_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk75_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk75";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk75_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk75_id);
}


/*! tsk_set33の起動 */
static void tsk_set33_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk78_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk78";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk78_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk78_id);
}


/*! tsk_set34の起動 */
static void tsk_set34_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk81_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk81";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk81_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk81_id);
}


/*! tsk_set35の起動 */
static void tsk_set35_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk84_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk84";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk84_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk84_id);
}


/*! tsk_set36の起動 */
static void tsk_set36_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk87_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk87";
	tsk1_param.un.acre_tsk.priority = 3;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk87_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk87_id);
}


/*! tsk_set37の起動 */
static void tsk_set37_command(void)
{
  puts("coming soon.\n");
}


/*! tsk_set38の起動 */
static void tsk_set38_command(void)
{
  puts("coming soon.\n");
}


/*! tsk_set39の起動 */
static void tsk_set39_command(void)
{
  puts("coming soon.\n");
}


/*! tsk_set40の起動 */
static void tsk_set40_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk98_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk98";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk98_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk98_id);
}


/*! tsk_set41の起動 */
static void tsk_set41_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk101_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk101";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk101_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk101_id);
}


/*! tsk_set42の起動 */
static void tsk_set42_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk104_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk104";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk104_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk104_id);
}


/*! tsk_set43の起動 */
static void tsk_set43_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk107_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk107";
	tsk1_param.un.acre_tsk.priority = 3;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk107_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk107_id);
}


/*! tsk_set44の起動 */
static void tsk_set44_command(void)
{
  SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk109_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk109";
	tsk1_param.un.acre_tsk.priority = 5;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

  /* sample_tsk109_id = mz_run_tsk(&tsk1_param); */
	tsk_set44_msg();
}


/*! tsk_set45の起動 */
static void tsk_set45_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk111_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk111";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk111_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk111_id);
}


static void tsk_set46_command(void)
{
	tsk_set46_msg();
}


static void tsk_set47_command(void)
{
	tsk_set47_msg();
}


static void tsk_set48_command(void)
{
	tsk_set48_msg();
}


static void tsk_set49_command(void)
{
	tsk_set49_msg();
}


static void tsk_set50_command(void)
{
	tsk_set50_msg();
}


static void tsk_set51_command(void)
{
	tsk_set51_msg();
}


static void tsk_set52_command(void)
{
	tsk_set52_msg();
}


static void tsk_set53_command(void)
{
	tsk_set53_msg();
}


static void tsk_set54_command(void)
{
	SYSCALL_PARAMCB tsk1_param;

	tsk1_param.un.acre_tsk.type = DYNAMIC_TASK;
	tsk1_param.un.acre_tsk.func = sample_tsk138_main;
	tsk1_param.un.acre_tsk.name = "sample_tsk138";
	tsk1_param.un.acre_tsk.priority = 1;
	tsk1_param.un.acre_tsk.stacksize = 0x100;
	tsk1_param.un.acre_tsk.argc = 0;
	tsk1_param.un.acre_tsk.argv = NULL;

	sample_tsk138_id = mz_iacre_tsk(&tsk1_param);
	mz_ista_tsk(sample_tsk138_id);
}


void tsk_set44_msg(void)
{                                                             
	puts("sampel_tsk109 started.\n");
	puts("sample_tsk110 started.");
	puts("sample_tsk110 DORMANT.");
	puts("exection change system rollback for interrput handler.\n");
	puts(" syscall handler ok.\n");
	puts(" softerr handler ok.\n");
	puts(" timer handler handler ok.\n");
	puts(" nmi handler ok.\n");
	puts("init task started.\n");
	puts("sample_tsk109 started.\n");
	puts("sample_tsk110 started.\n");
	puts("sample_tsk109 DORMANT.\n");
	puts("sample_tsk110 DORMANT.\n");
}


void tsk_set46_msg(void)
{
	/* 周期ハンドラを使用するのできつい */
}


void tsk_set47_msg(void)
{
	puts("Rate Monotonic Deadline Miss task set\n");
	puts("OS sleep... Please push reset button\n");
	while (1) {
		;
	}
}


void tsk_set48_msg(void)
{
	/* 周期ハンドラを使用するのできつい */
}


void tsk_set49_msg(void)
{
	puts("Rate Monotonic Deadline Miss task set\n");
	puts("OS sleep... Please push reset button\n");
	while (1) {
		;
	}
}


/* EDF */
void tsk_set50_msg(void)
{
	puts("sample_tsk126 started.\n");
	puts("sample_tsk127 started.\n");
	puts("sample_tsk127 DORMANT.\n");
	puts("sample_tsk126 DORMANT.\n");
	puts("sample_tsk128 started.\n");
	puts("sample_tsk128 DORMANT.\n");
}


/* EDF */
void tsk_set51_msg(void)
{
	puts("sample_tsk129 started.\n");
	puts("Earliest Deadline First Deadline Miss task set\n");
	puts("OS sleep... Please push reset button\n");
	while (1) {
		;
	}
}


/* LLF */
void tsk_set52_msg(void)
{
	puts("sample_tsk132 started.\n");
	puts("sample_tsk133 started.\n");
	puts("sample_tsk133 DORMANT.\n");
	puts("sample_tsk132 DORMANT.\n");
	puts("sample_tsk134 started.\n");
	puts("sample_tsk134 DORMANT.\n");
}


/* LLF */
void tsk_set53_msg(void)
{
	puts("sample_tsk135 started.\n");
	puts("Least Laxity First Deadline Miss task set\n");
	puts("OS sleep... Please push reset button\n");
	while (1) {
		;
	}
}


#endif
