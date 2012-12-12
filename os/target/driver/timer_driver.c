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
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "c_lib/lib.h"
#include "timer_driver.h"
#include "kernel/time_manage.h"


#define TIMER_NUM 2 /*! タイマの資源数 */

/* 8ビットタイマをカスケード接続し，16ビットタイマとして利用する */
#define H8_3069F_TMR01 ((volatile struct h8_3069f_tmr *)0xffff80) /*! メモリマッピングアドレス，レジスタの先頭アドレスを指定しておく */
#define H8_3069F_TMR23 ((volatile struct h8_3069f_tmr *)0xffff90) /*! メモリマッピングアドレス，レジスタの先頭アドレスを指定しておく */


/*! タイマ各種レジスタ．タイムコンスタントレジスタA0とA1, B0とB1でワードアクセス(カスケード接続)する */
struct h8_3069f_tmr {
  volatile UINT8 tcr0; /*! チャネル0，タイマコントロールレジスタ0 */
  volatile UINT8 tcr1; /*! チャネル1，タイマコントロールレジスタ1 */
  volatile UINT8 tcsr0; /*! チャネル0，タイマコントロール/ステータスレジスタ0 */
  volatile UINT8 tcsr1; /*! チャネル1，タイマコントロール/ステータスレジスタ1 */
  volatile UINT8 tcora0; /*! チャネル0，タイマコンスタントレジスタA0 */
  volatile UINT8 tcora1; /*! チャネル1，タイマコンスタントレジスタA1 */
  volatile UINT8 tcorb0; /*! チャネル0，タイマコンスタントレジスタB0 */
  volatile UINT8 tcorb1; /*! チャネル1，タイマコンスタントレジスタB1 */
  volatile UINT16 tcnt; /*! チャネル0のタイマカウンタ*/
};


/* タイマコントロールレジスタ操作 */
/* CKS0~CLK2(クロックセレクトビット0, 1, 2番)操作．8TCNT(タイマカウンタ)に入力するクロックを内部クロックまたは外部クロックから選択 */
#define H8_3069F_TMR_TCR_DISCLK       (0<<0) /*! クロック入力の禁止*/
#define H8_3069F_TMR_TCR_CLK8         (1<<0) /*! 内部クロック．立ち上がりエッジカウント．8分周*/
#define H8_3069F_TMR_TCR_CLK64        (2<<0) /*! 内部クロック．立ち上がりエッジカウント．64分周*/
#define H8_3069F_TMR_TCR_CLK8192      (3<<0) /*! 内部クロック．立ち上がりエッジカウント．8192分周*/
#define H8_3069F_TMR_TCR_OVF          (4<<0) /*! 8TCNT1のオーバフロー信号でカウント*/
#define H8_3069F_TMR_TCR_CMFA         (4<<0) /*! 8TCNT0のコンペアマッチAでカウント*/
#define H8_3069F_TMR_TCR_CLKUP        (5<<0) /*! 外部クロック．立ち上がりエッジカウント．*/
#define H8_3069F_TMR_TCR_CLKDOWN      (6<<0) /*! 外部クロック．立ち下がりエッジカウント．*/
#define H8_3069F_TMR_TCR_CLKBOTH      (7<<0) /*! 外部クロック．立ち上がり/立ち下がり両エッジカウント*/
/* CCLR1~CCLR2(カウンタクリアビット3, 4番)操作．8TCNTクリア要因指定 */
#define H8_3069F_TMR_TCR_CCLR_DISCLR  (0<<3) /*! クロック入力禁止*/
#define H8_3069F_TMR_TCR_CCLR_CLRCMFA (1<<3) /*! CMFA(コンペアマッチフラグA)によりクリア*/
#define H8_3069F_TMR_TCR_CCLR_CLRCMFB (2<<3) /*! CMFB(コンペアマッチB/インプットキャプチャB)によりクリア*/
#define H8_3069F_TMR_TCR_CCLR_DISINPB (3<<3) /*! インプットキャプチャBによりクリア*/
/*
* OVIE(タイマオーバーフローインタラプトイネーブル)ビット5番操作．
* 8TCSR(タイマコントロールステータスレジスタ)にOVF(オーバーフローフラグ)セット時の割り込み要求
*/
#define H8_3069F_TMR_TCR_OVIE         (1<<5) /*! OVFによる割り込み要求許可 */
/* CMIEA(コンペアマッチインタラプトイネーブルA)ビット6番操作．8TCSRにCMFAセット時の割り込み要求 */
#define H8_3069F_TMR_TCR_CMIEA        (1<<6) /*! CMFAによる割り込み要求許可 */
/* CMIEB(コンペアマッチインタラプトイネーブルB)ビット7番操作．8TCSRにCMFBセット時の割り込み要求 */
#define H8_3069F_TMR_TCR_CMIEB        (1<<7) /*! CMFBによる割り込み許可 */


/* タイマコントロール/ステータスレジスタ操作 */
/*! OS1, OS0(アウトプットセレクト)ビット0番，1番操作．コンペアマッチA, Bによる出力レベルの選択 */
#define H8_3069F_TMR_TCSR_OS_NOACT    (0<<0) /*! コンペアマッチAで変化禁止*/
#define H8_3069F_TMR_TCSR_OIS_NOACT   (0<<2) /*! タイマ出力禁止*/
/*! ADTE(A/Dトリガーイネーブル)ビット4番操作．コンペアマッチAまたは外部トリガによるA/D変換開始要求の許可または禁止 */
#define H8_3069F_TMR_TCSR_ADTE        (1<<4) /*! A/D変換開始要求の禁止 */
/*! ICE(インプットキャプチャイネーブル)ビット4番操作．*/
#define H8_3069F_TMR_TCSR_ICE         (1<<4) /*! TCORB1(タイムコンスタントレジスタB1), TCORB3をインプットキャプチャレジスタにする*/
/*! OVFビット5番操作．OV(オーバーフロー)の発生を示す */
#define H8_3069F_TMR_TCSR_OVF         (1<<5) /*! セット */
/*! CMFAビット6番操作．CMFAの発生を示す */
#define H8_3069F_TMR_TCSR_CMFA        (1<<6) /*! セット */
/*! CMFBビット7番操作．CMFBの発生を示す */
#define H8_3069F_TMR_TCSR_CMFB        (1<<7) /*! セット */


/*! カスケード接続したタイマ資源を配列化 */
static struct {
  volatile struct h8_3069f_tmr *tmr;
} regs[TIMER_NUM] = {
  { H8_3069F_TMR01 }, /*! ハードタイマ */
  { H8_3069F_TMR23 }, /*! 差分のキュー管理によるソフトタイマ */
};


/*ターゲット依存部 */
/*! タイマ満了したか検査する関数 */
static BOOL is_check_timer_expired(int index);

/*! タイマ満了処理をする関数 */
static void expire_timer(int index);

/*! タイマ動作中か検査する関数 */
static BOOL is_running_timer(int index);

/* ターゲット非依存部 */
/*! 差分のキューへタイマコントロールブロック挿入 */
static void insert_tmrcb_diffque(TMRCB* newtbf);

/*! 差分のキューからノードを次に進める */
static void next_tmrcb_diffque(void);


/********************************************************************************
*てい倍したカウント値をtcora0(タイマコンスタントレジスタ)にセットする．tcora0の値		*
*が減産していき，0になるとTCNT(タイマカウンタ)	と一致する事でタイマ割込み発生となる．	*
*CPUキックされるとTCSR(タイマコントロール/ステータスレジスタ)のCMFA									*
*(コンペアマッチフラグ)に1が立つ．CMFAを見ることでタイマの満了を知る事ができる．			*
*********************************************************************************/


/*!
*タイマ開始処理
*index : タイマ資源番号
*msec : 要求タイマ値
*flags : 操作フラグ
*/
void start_timer(int index, int msec, int flags)
{
  volatile struct h8_3069f_tmr *tmr = regs[index].tmr;
  int count;
  UINT8 tcr;

  tcr = H8_3069F_TMR_TCR_OVF; /* オーバーフロー信号でタイマカウント */
	/* ハードタイマ処理 */
  if (flags & HARD_TIMER_CYCLE_FLAG) {
    tcr |= H8_3069F_TMR_TCR_CCLR_CLRCMFA; /* カウンタをコンペアマッチAよりクリア */
	}
	/* ソフトタイマ処理 */
  else {
    tcr |= H8_3069F_TMR_TCR_CCLR_DISCLR; /* クロック入力を禁止 */
	}
  tmr->tcr0 = tcr;
  tmr->tcr1 = H8_3069F_TMR_TCR_CLK8192 | H8_3069F_TMR_TCR_CCLR_DISCLR; /* 内部クロックを8192分割し，クロック入力を禁止 */

  tmr->tcsr0 = 0;
  tmr->tcsr1 = 0;

  count = msec / 105; /* 20MHz: (msec * 20000000 / 8192 / 256 / 1000) */

  tmr->tcnt = 0;
  tmr->tcora0 = count;
	/* イマ割込み無効化．TCR(タイマコントロールレジスタ)のCMIEA(コンペアマッチインタラプトイネーブル)ビットを立てる */
  tmr->tcr0 |= H8_3069F_TMR_TCR_CMIEA;
}


/*!
*タイマ満了したか検査する関数
*index : 検査するタイマ資源番号
*(返却値)FALSE : タイマは満了していない
*(返却値)TRUE : タイマ満了
*/
static BOOL is_check_timer_expired(int index)
{
  volatile struct h8_3069f_tmr *tmr = regs[index].tmr;

	/* TCSR(タイマコントロール/ステータスレジスタ)のCMFAビットを見る */
	if (tmr->tcsr0 & H8_3069F_TMR_TCSR_CMFA) {
		return TRUE; /* タイマ満了 */
	}
	else {
		return FALSE; /* タイマは満了してない */
	}
}

/*!
*タイマ満了処理をする関数
*index : タイマ資源番号
*/
static void expire_timer(int index)
{
  volatile struct h8_3069f_tmr *tmr = regs[index].tmr;

	/* TCSR(タイマコントロール/ステータスレジスタ)のCMFAビットを落とす */
  tmr->tcsr0 &= ~H8_3069F_TMR_TCSR_CMFA;
}

/*!
*タイマキャンセルする関数
*index : タイマ資源番号
*/
void cancel_timer(int index)
{
  volatile struct h8_3069f_tmr *tmr = regs[index].tmr;

  expire_timer(index); /* タイマ満了処理 */

  tmr->tcr0 = 0;
  tmr->tcr1 = 0;

	/* タイマ割込み無効化．TCR(タイマコントロールレジスタ)のCMIEA(コンペアマッチインタラプトイネーブル)ビットを落とす */
  tmr->tcr0 &= ~H8_3069F_TMR_TCR_CMIEA;
}


/*!
*タイマ動作中か検査する関数
*index : タイマ資源番号
*(返却値)TRUE : タイマ動作中
*(返却値)FALSE : タイマは動作してない
*/
static BOOL is_running_timer(int index)
{
  volatile struct h8_3069f_tmr *tmr = regs[index].tmr;

	/* tcr(タイマコントロールレジスタ)のCMIEB(コンペアマッチインタラプトイネーブル)ビットを見る */
	if (tmr->tcr0 & H8_3069F_TMR_TCR_CMIEA) {
		return TRUE; /* タイマ動作中 */
	}
	else {
		return FALSE; /* タイマは動作していない */
	}
}


/*!
*タイマの現在値を取得する関数
*index : タイマ資源番号
*(返却値)msec : タイマの現在値
*(返却値)-1 : タイマは動作してない
*/
ER_VLE get_timervalue(int index)
{
  volatile struct h8_3069f_tmr *tmr = regs[index].tmr;
  volatile int count;
  int msec;

  /*
   * 周期タイマの場合は動作中かどうかのチェックの直後にタイマ満了すると
   * カウンタがゼロに初期化されてしまうので，前もって値を取得しておく．
   */
  count = tmr->tcnt;
  msec = count * 2 / 5; /* 20MHz: (count * 8192 * 1000 / 20000000) */

	if (is_running_timer(index)) {
		return (ER_VLE)msec; /* 現在のタイマ値を返却 */
	}
	else {
		return E_NG; /* タイマは動作していない */
	}
}


/**************************************************************************
 * 以下は割込みハンドラであり，非同期で呼ばれるので，ライブラリ関数などを 			*
 * 呼び出す場合には注意が必要．																							*
 * 基本として，以下のいずれかに当てはまる関数しか呼び出してはいけない．				*
 * ・再入可能である(そんなのはない)．																				*
 * ・スレッドから呼ばれることは無い関数である．																*
 * ・スレッドから呼ばれることがあるが，割込み禁止で呼び出している．						*
 * また非コンテキスト状態で呼ばれるため，システム・コールは利用してはいけない．	*
 * (サービス・コールを利用すること)																					*
 **************************************************************************/


/*!
*タイマ割り込みハンドラ
*/
void tmrdriver_intr(void)
{
	static int outp = 0;
	CYCCB *cycb = (CYCCB *)mg_timerque.tmrhead->rqobjp; /* 周期タイマ以外の場合は0となる */
	TMRCB *tbf;
	
	/* 差分のキューによるソフトタイマ */
  if (is_check_timer_expired(mg_timerque.index)) { /* タイマは満了したか */
		DEBUG_OUTMSG("softtimer is expire 1.\n");
		DEBUG_OUTVLE(outp, 0);
		DEBUG_OUTMSG("\n");
		outp++;
    cancel_timer(mg_timerque.index); /* タイマキャンセル処理 */
		next_tmrcb_diffque(); /* 差分のキューからタイマコントロールブロックの排除 */
		
		/* 周期タイマの時はタイマブロックを作成(createを呼ぶと先頭のタイマがかけられてしまうので排除してから呼ぶ) */
    if (cycb) {
    	tbf = (TMRCB *)create_tmrcb_diffque(OTHER_MAKE_TIMER, cycb->cyctim, (TMRRQ_OBJP)cycb, cycb->func, cycb->exinf); /* 差分のキューのノードを作成 */
    	cycb->tobjp = (TMR_OBJP)tbf;
    }
  }
	/* ハードタイマ */
	if (is_check_timer_expired(HARD_TIMER_DEFAULT_DEVICE)) { /* タイマは満了したか */
		DEBUG_OUTMSG("hardtimer id expire 0.\n");
		expire_timer(HARD_TIMER_DEFAULT_DEVICE); /* タイマ満了処理 */
	}
}


/*!
タイマドライバの初期化
*/
void tmrdriver_init(void)
{
	mg_timerque.tmrhead = NULL;
	mg_timerque.index = SOFT_TIMER_DEFAULT_DEVICE;
}


/*!
* 差分のキューのノードを作成
* 多少のすれ違い(タイマ割込みによるノード解放とシステムコールによるノード作成)の考慮
* flag : タイマの要求種類
* request_sec : 要求タイマ値
* rqobjp : ソフトタイマを要求したオブジェクトのポインタ(ソフトタイマで周期機能を使用したいケースのみ指定する.)
* func : タイマ満了時のコールバックルーチン
* *argv : コールバックルーチンに渡すパラメータ
* (返却値)E_NG : hard timerを差分のキュー管理しようとした場合
* (返却値)newtbf : 新規作成したタイマコントロールブロックへポインタ
*/
OBJP create_tmrcb_diffque(short flag, int request_sec, TMRRQ_OBJP rqobjp, TMR_CALLRTE func, void *argv)
{
	TMRCB *newtbf;

	newtbf = (TMRCB *)get_mpf_isr(sizeof(*newtbf)); /* 動的メモリ取得要求 */

	/* メモリが取得できない */
  if(newtbf == NULL) {
    down_system();
  }
  
	newtbf->next = newtbf->prev = NULL;
	newtbf->flag = flag;
	newtbf->msec = request_sec;
	newtbf->rqobjp = rqobjp; /* 周期機能を使用しない時は0が入る */
	newtbf->func = func;
	newtbf->argv = argv;
	
	insert_tmrcb_diffque(newtbf); /* タイマコントロールブロックの挿入 */
	
	return (OBJP)newtbf;
}


/*!
* 差分のキューへタイマコントロールブロック挿入
* *newtbf : 挿入するタイマコントロールブロック
* (返却値)newtbf : 挿入したタイマコントロールブロックへポインタ
*/
static void insert_tmrcb_diffque(TMRCB* newtbf)
{
	TMRCB *worktbf, *tmptbf;
	int time_now; /* 現在までカウントしたタイマ値 */
	int diff_msec; /* 差分タイマ値 */
	int i;

	time_now = 0;
	/* 現在までカウントタイマ値を取得 */
  if (mg_timerque.tmrhead) {
    time_now = (int)get_timervalue(mg_timerque.index);
  }

	/* ここから差分のキューに挿入 */
	tmptbf = worktbf = mg_timerque.tmrhead;
	/* 差分のキューにノードがない場合 */
	if (worktbf == NULL) {
			mg_timerque.tmrhead = newtbf;
			/* すれ違いの考慮 */
			if (newtbf == mg_timerque.tmrhead) {
				start_timer(mg_timerque.index, mg_timerque.tmrhead->msec, 0); /*新規作成したnewtbfのタイマをスタートさせる*/
			}
	}

	/* 差分のキューにノードがある場合(forの継続条件でtmptbf != NULLは指定できない) */
	for (i = 0; worktbf != NULL; i++) {
		diff_msec = worktbf->msec - time_now; /* 現在ノードのタイマ値 - 現在までカウントしたタイマ値 */
		/* すれ違いの考慮 */
    if (diff_msec < 0) { /* すでに終了した場合 */
			diff_msec = 0;
		}
		/* ここから挿入操作 */
		if (newtbf->msec < diff_msec) {
			/* ここで差分をする(最後に挿入される以外は現在ノードの値も差分) */
			/* 差分のキュー現在ノードのタイマ値 - 現在までカウントしたタイマ値 - 新規作成したノードのタイマ値 */
			worktbf->msec = diff_msec - newtbf->msec;
			break;
    }
		time_now = 0;
    newtbf->msec -= diff_msec; /* 新規作成したノードのタイマ値 - 差分のキューの現在ノードのタイマ値 - 現在までカウントしたタイマ値  */
		tmptbf = worktbf;
		worktbf = worktbf->next;
	}
	
	/* ポインタの付け替え */
	newtbf->next = worktbf;
	/* 先頭に挿入 */
	if (i == 0) {
		DEBUG_OUTMSG("many1.\n");
		worktbf->prev = newtbf;
		mg_timerque.tmrhead = newtbf;
	}
	/* 最後に代入 */
	else if (worktbf == NULL) {
		DEBUG_OUTMSG("many2.\n");
		newtbf->prev = tmptbf;
		tmptbf->next = newtbf;
	}
	/* 上記以外に挿入 */
	else {
		DEBUG_OUTMSG("many3.\n");
		newtbf->prev = worktbf->prev;
		worktbf->prev = worktbf->prev->next = newtbf;
	}
	start_timer(mg_timerque.index, mg_timerque.tmrhead->msec, 0); /* 差分のキューの先頭のノードのタイマをスタートさせる */
}


/*!
* 差分のキューからノードを進める(古いノードは排除)
*/
static void next_tmrcb_diffque(void)
{
	TMRCB *worktbf;

	worktbf = mg_timerque.tmrhead;
	
	/* ここでNULLチェックしないとシステムコール以外でのタイマが不使用となる(おかしくなる )*/
	if (worktbf->func != NULL) {
		(*worktbf->func)(worktbf->argv); /* コールバックルーチンの呼び出し */
	}
	
	/* 差分のキューにノードがあるならば */
	if (worktbf) {
		mg_timerque.tmrhead = worktbf->next; /* 次のタイマコントロールブロックへ */
		mg_timerque.tmrhead->prev = NULL;
		/* タイマコントロールブロックの動的メモリ解放(モノリシックカーネルなのでシステムコールは使用できないため，内部関数を呼ぶ) */
		rel_mpf_isr(worktbf);
		if (mg_timerque.tmrhead) {
			start_timer(mg_timerque.index, mg_timerque.tmrhead->msec, 0); /* タイマをスタートさせる */
		}
		else {
			DEBUG_OUTMSG("all release timercb1.\n");
		}
	}
	else {
		DEBUG_OUTMSG("all release timercb2.\n");
	}
}


/*!
* 引数で指定されたタイマコントロールブロックを排除する
* index : タイマ番号
* *deltbf : 対象排除タイマコントロールブロック
* (返却値)E_NOSAPT : ハードタイマを選択された場合
* (返却値)E_NOEXS : (オブジェクト未生成)タイマコントロールブロックが指定されていない
* (返却値)E_OK : 正常終了
*/
ER delete_tmrcb_diffque(TMRCB *deltbf)
{

	/* タイマコントロールブロックは指定されていない */
	if (deltbf == NULL) {
		return E_NOEXS;
	}
	/* ここから排除処理 */
	else {
		/*
		* レディーキューの先頭を抜き取る
		* 一度タイマをキャンセルする(タイマコントロールブロックが排除されたからといって，
		* タイマ割込みが発生しなくなることはない.つまり，コールバックルーチンは呼ばれないが，
		* タイマ割込みが発生する.)
		* また，このタイマキャンセル処理は下のif文内のタイマ再起動処理と近づけないと，ソフトタイマの
		* 誤差が蓄積されていっていしまう.
		*/
		if (deltbf == mg_timerque.tmrhead) {
			mg_timerque.tmrhead = deltbf->next;
			mg_timerque.tmrhead->prev = NULL;
			cancel_timer(SOFT_TIMER_DEFAULT_DEVICE);
			/* まだタイマ要求があれば次の要求にうつる */
			if (mg_timerque.tmrhead) {
				start_timer(mg_timerque.index, mg_timerque.tmrhead->msec, 0); /* タイマをスタートさせる */
			}
		}
		/* レディーキューの最後から抜き取る */
		else if (deltbf->next == NULL) {
			deltbf->prev->next = NULL;
		}
		/*
		* レディーキューの中間から抜き取る
		* 差分のキューとしてソフトタイマを実装しているので，
		* 中間から抜き取る時は後続のノードに抜き取るノードの差分値を加算する
		*/
		else {
			deltbf->next->msec = deltbf->next->msec + deltbf->msec;
			deltbf->prev->next = deltbf->next;
			deltbf->next->prev = deltbf->prev;
		}
		/*
		* タイマコントロールブロックの動的メモリ解放
		* メモリ解放を行うので，deltbfのポインタはNULLしておかなくてよい
		*/
		rel_mpf_isr(deltbf);
	
		return E_OK;
	}
}
