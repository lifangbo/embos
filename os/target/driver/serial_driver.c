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


/*
* このデバイスドライバはキャラクタ型デバイスドライバとなる
*/

#include "kernel/defines.h"
#include "serial_driver.h"


#define SERIAL_SCI_NUM 							3 /*! sciポート数 */

#define H8_3069F_SCI0								((volatile struct h8_3069f_sci *)0xffffb0) /*! sciポート1 */
#define H8_3069F_SCI1								((volatile struct h8_3069f_sci *)0xffffb8) /*! sciポート2 */
#define H8_3069F_SCI2								((volatile struct h8_3069f_sci *)0xffffc0) /*! sciポート3 */


/*! sci定義 */
struct h8_3069f_sci {
  volatile UINT8 smr; /*! シリアルモードレジスタ */
  volatile UINT8 brr; /*! ビットレートレジスタ */
  volatile UINT8 scr; /*! シリアルコントロールレジスタ */
  volatile UINT8 tdr; /*! トランスレッションデータレジスタ */
  volatile UINT8 ssr; /*! シリアルステータスレジスタ */
  volatile UINT8 rdr; 
  volatile UINT8 scmr;
};


/* シリアルモードレジスタ操作マクロ */
#define H8_3069F_SCI_SMR_CKS_PER1		(0<<0) /*! クロックセレクト1 */
#define H8_3069F_SCI_SMR_CKS_PER4		(1<<0) /*! クロックセレクト4 */
#define H8_3069F_SCI_SMR_CKS_PER16  (2<<0) /*! クロックセレクト16 */
#define H8_3069F_SCI_SMR_CKS_PER64	(3<<0) /*! クロックセレクト64 */
#define H8_3069F_SCI_SMR_MP     		(1<<2) /*! パリティ有効無効 */
#define H8_3069F_SCI_SMR_STOP   		(1<<3) /*! ストップビット長 */
#define H8_3069F_SCI_SMR_OE     		(1<<4) /*! パリティの種類 */
#define H8_3069F_SCI_SMR_PE     		(1<<5) /*! パリティ有効無効 */
#define H8_3069F_SCI_SMR_CHR    		(1<<6) /*! データ長 */
#define H8_3069F_SCI_SMR_CA     		(1<<7) /*! 同期モード */

/* シリアルコントロールレジスタ操作マクロ */
#define H8_3069F_SCI_SCR_CKE0   		(1<<0) /*! クロックイネーブル */
#define H8_3069F_SCI_SCR_CKE1   		(1<<1) /*! クロックイネーブル */
#define H8_3069F_SCI_SCR_TEIE   		(1<<2) /*! クロックイネーブル */
#define H8_3069F_SCI_SCR_MPIE   		(1<<3) /*! クロックイネーブル */
#define H8_3069F_SCI_SCR_RE     		(1<<4) /*! 受信有効 */
#define H8_3069F_SCI_SCR_TE     		(1<<5) /*! 送信有効 */
#define H8_3069F_SCI_SCR_RIE    		(1<<6) /*! 受信割込み有効 */
#define H8_3069F_SCI_SCR_TIE    		(1<<7) /*! 送信割込み有効 */

/* シリアルステータスレジスタ操作マクロ */
#define H8_3069F_SCI_SSR_MPBT				(1<<0)
#define H8_3069F_SCI_SSR_MPB				(1<<1)
#define H8_3069F_SCI_SSR_TEND				(1<<2)
#define H8_3069F_SCI_SSR_PER				(1<<3)
#define H8_3069F_SCI_SSR_FERERS			(1<<4)
#define H8_3069F_SCI_SSR_ORER				(1<<5)
#define H8_3069F_SCI_SSR_RDRF				(1<<6) /*! 受信完了 */
#define H8_3069F_SCI_SSR_TDRE				(1<<7) /*! 送信完了 */


/*! sciをポート番号ごとに配列化 */
static struct {
  volatile struct h8_3069f_sci *sci;
} regs[SERIAL_SCI_NUM] = {
  { H8_3069F_SCI0 }, 			/* ポート1 */
  { H8_3069F_SCI1 }, 			/* ポート2 */
  { H8_3069F_SCI2 }, 			/* ポート3 */
};


/* serial_driver.cには使用していない関数もある(それはstaticにできないのでhファイルにプロトタイプがある) */
/*! 送信可能か？ */
static int serial_is_send_enable(int index);

/*! 受信可能か？ */
static int serial_is_recv_enable(int index);


/*!
* デバイス初期化
* index : sciポート番号
*/
void serial_init(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;

  sci->scr = 0;
  sci->smr = 0;
  sci->brr = 64; /* 20MHzのクロックから9600bpsを生成(25MHzの場合は80にする) */
  sci->scr = H8_3069F_SCI_SCR_RE | H8_3069F_SCI_SCR_TE; /* 送受信可能 */
  sci->ssr = 0;
}


/*!
* 送信可能か？
* index : sciポート番号
*/
int serial_is_send_enable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  return (sci->ssr & H8_3069F_SCI_SSR_TDRE);
}


/*!
* １文字送信
* 0を返すだけなのにvoidとしないのはlib.cのreturn のところでこの関数を
* 使用しているから
* index : sciポート番号
* c : 送信する文字
*/
int serial_send_byte(int index, unsigned char c)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;

  /* 送信可能になるまで待つ */
  while (!serial_is_send_enable(index)) {
    ;
  }
  sci->tdr = c;
  sci->ssr &= ~H8_3069F_SCI_SSR_TDRE; /* 送信開始 */

	return 0;
}


/*!
* 受信可能か？
* index : sciポート番号
*/
static int serial_is_recv_enable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  
  return (sci->ssr & H8_3069F_SCI_SSR_RDRF);
}


/*!
* １文字受信
* index : sciポート番号
*/
unsigned char serial_recv_byte(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  unsigned char c;

  /* 受信文字が来るまで待つ */
  while (!serial_is_recv_enable(index)) {
    ;
  }
  c = sci->rdr;
  sci->ssr &= ~H8_3069F_SCI_SSR_RDRF; /* 受信完了 */

  return c;
}


/*! 
* 送信割込み有効か？
* index : sciポート番号
* (返却値)TRUE : 送信割込み有効
* (返却値)FALSE : 送信割込み無効
*/
BOOL serial_intr_is_send_enable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  if (sci->scr & H8_3069F_SCI_SCR_TIE) {
  	return TRUE;
  }
  else {
  	return FALSE;
  }
}


/*!
* 送信割込み有効化
* index : sciポート番号
*/
void serial_intr_send_enable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr |= H8_3069F_SCI_SCR_TIE;
}


/*!
* 送信割込み無効化
* index : sciポート番号
*/
void serial_intr_send_disable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr &= ~H8_3069F_SCI_SCR_TIE;
}


/*!
* 受信割込み有効か？
* index : sciポート番号
* (返却値)TRUE : 受信割込み有効
* (返却値)FALSE : 受信割込み無効
*/
BOOL serial_intr_is_recv_enable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  
  if (sci->scr & H8_3069F_SCI_SCR_RIE) {
  	return TRUE;
  }
  else {
  	return FALSE;
  }
}


/*!
* 受信割込み有効化
* index : sciポート番号
*/
void serial_intr_recv_enable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr |= H8_3069F_SCI_SCR_RIE;
}


/*!
* 受信割込み無効化
* index : sciポート番号
*/
void serial_intr_recv_disable(int index)
{
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr &= ~H8_3069F_SCI_SCR_RIE;
}
