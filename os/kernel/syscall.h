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


#ifndef _SYSCALL_H_INCLUDE_
#define _SYSCALL_H_INCLUDE_

#include "defines.h"
#include "arch/cpu/interrupt.h"

#define WAIT_ERCD_NOCHANGE 0 /*! 他のシステムコールにより返却値が書き換えられない場合のマクロ(TCBのwait_info_wait_erに設定) */

/*! システム・コール番号の定義 */
typedef enum {
  ISR_TYPE_ACRE_TSK = 0,
  ISR_TYPE_DEL_TSK,
  ISR_TYPE_STA_TSK,
	ISR_TYPE_RUN_TSK,
  ISR_TYPE_EXT_TSK,
  ISR_TYPE_EXD_TSK,
  ISR_TYPE_TER_TSK,
  ISR_TYPE_GET_PRI,
  ISR_TYPE_CHG_PRI,
	ISR_TYPE_CHG_SLT,
	ISR_TYPE_GET_SLT,
  ISR_TYPE_SLP_TSK,
  ISR_TYPE_TSLP_TSK,
  ISR_TYPE_WUP_TSK,
  ISR_TYPE_REL_WAI,
  ISR_TYPE_DLY_TSK,
  ISR_TYPE_WAI_TSK,
  ISR_TYPE_ACRE_SEM,
  ISR_TYPE_DEL_SEM,
  ISR_TYPE_SIG_SEM,
  ISR_TYPE_WAI_SEM,
  ISR_TYPE_POL_SEM,
  ISR_TYPE_TWAI_SEM,
	ISR_TYPE_ACRE_MBX,
	ISR_TYPE_DEL_MBX,
	ISR_TYPE_SND_MBX,
	ISR_TYPE_RCV_MBX,
	ISR_TYPE_PRCV_MBX,
	ISR_TYPE_TRCV_MBX,
  ISR_TYPE_ACRE_MTX,
	ISR_TYPE_DEL_MTX,
  ISR_TYPE_LOC_MTX,
  ISR_TYPE_PLOC_MTX,
  ISR_TYPE_TLOC_MTX,
  ISR_TYPE_UNL_MTX,
	ISR_TYPE_GET_MPF,
	ISR_TYPE_REL_MPF,
	ISR_TYPE_ACRE_CYC,
	ISR_TYPE_DEL_CYC,
	ISR_TYPE_STA_CYC,
	ISR_TYPE_STP_CYC,
  ISR_TYPE_ACRE_ALM,
  ISR_TYPE_DEL_ALM,
  ISR_TYPE_STA_ALM,
  ISR_TYPE_STP_ALM,
	ISR_TYPE_DEF_INH,
	ISR_TYPE_ROT_RDQ,
  ISR_TYPE_GET_TID,
  ISR_TYPE_DIS_DSP,
  ISR_TYPE_ENA_DSP,
  ISR_TYPE_SNS_DSP,
  ISR_TYPE_SET_POW,
	ISR_TYPE_SEL_SCHDUL, /* サービスコールのみとなるので，実際はいらないが，他と一貫性と保つため */
	ISR_TYPE_ROL_SYS,
 } ISR_TYPE;


/*! システム・コール呼び出し時のパラメータ類格納域の定義 */
typedef struct {
  union {
    struct {
			TSK_TYPE type;
      TSK_FUNC func;
      char *name;
      int priority;
      int stacksize;
      int rate;
      int rel_exetim;
			int deadtim;
			int floatim;
      int argc;
      char **argv;
      ER_ID ret;
    } acre_tsk;
    struct {
    	ER_ID tskid;
    	ER ret;
    } del_tsk;
    struct {
    	ER_ID tskid;
    	ER ret;
    } sta_tsk;
		struct {
			TSK_TYPE type;
      TSK_FUNC func;
      char *name;
      int priority;
      int stacksize;
      int rate;
      int rel_exetim;
			int deadtim;
			int floatim;
      int argc;
      char **argv;
      ER_ID ret;
    } run_tsk;
    struct {
    	int dmummy;
    } ext_tsk;
    struct {
      int dummy;
    } exd_tsk;
    struct {
    	ER_ID tskid;
    	ER ret;
    } ter_tsk;
    struct {
    	ER_ID tskid;
    	int *p_tskpri;
    	ER ret;
    } get_pri;
    struct {
    	ER_ID tskid;
    	int tskpri;
    	ER ret;
    } chg_pri;
		struct {
			SCHDUL_TYPE type;
			ER_ID tskid;
			int slice;
			ER ret;
		} chg_slt;
		struct {
			SCHDUL_TYPE type;
			ER_ID tskid;
			int *p_slice;
			ER ret;
		} get_slt;
    struct {
    	ER ret;
    } slp_tsk;
    struct {
    	int msec;
    	ER ret;
    } tslp_tsk;
    struct {
    	ER_ID tskid;
    	ER ret;
    } wup_tsk;
    struct {
    	ER_ID tskid;
    	ER ret;
    } rel_wai;
    struct {
    	int msec;
    	ER ret;
    } dly_tsk;
    struct {
      int size;
      void *ret;
    } kmalloc;
    struct {
      char *p;
      int ret;
    } kmfree;
    struct {
    	SEM_TYPE type;
    	SEM_ATR atr;
    	int semvalue;
    	int maxvalue;
    	ER_ID ret;
    } acre_sem;
    struct {
    	ER_ID semid;
    	ER ret;
    } del_sem;
    struct {
    	ER_ID semid;
    	ER ret;
    } sig_sem;
    struct {
    	ER_ID semid;
    	ER ret;
    } wai_sem;
		struct {
			ER_ID semid;
			ER ret;
		} pol_sem;
		struct {
			ER_ID semid;
			int msec;
			ER ret;
		} twai_sem;
		struct {
			MBX_TYPE type;
			MBX_MATR msg_atr;
			MBX_WATR wai_atr;
			int max_msgpri;
			ER_ID ret;
		} acre_mbx;
		struct {
			ER_ID mbxid;
			ER ret;
		} del_mbx;
		struct {
			ER_ID mbxid;
			T_MSG *pk_msg;
			ER ret;
		} snd_mbx;
		struct {
			ER_ID mbxid;
			T_MSG **pk_msg;
			ER ret;
		} rcv_mbx;
		struct {
			ER_ID mbxid;
			T_MSG **pk_msg;
			ER ret;
		} prcv_mbx;
		struct {
			ER_ID mbxid;
			T_MSG **pk_msg;
			int tmout;
			ER ret;
		} trcv_mbx;
		struct {
			MTX_TYPE type;
			MTX_ATR atr;
			PIVER_TYPE piver_type;
			int maxlocks;
			int pcl_param;
			ER_ID ret;
		} acre_mtx;
		struct {
			ER_ID mtxid;
			ER ret;
		} del_mtx;
		struct {
			ER_ID mtxid;
			ER ret;
		} loc_mtx;
		struct {
			ER_ID mtxid;
			ER ret;
		} ploc_mtx;
		struct {
			ER_ID mtxid;
			int msec;
			ER ret;
		} tloc_mtx;
		struct {
			ER_ID mtxid;
			ER ret;
		} unl_mtx;
		struct {
      int size;
      void *ret;
    } get_mpf;
    struct {
      char *p;
      int ret;
    } rel_mpf;
    struct {
    	CYC_TYPE type;
    	void *exinf;
    	int cyctim;
    	TMR_CALLRTE func;
    	ER_ID ret;
		} acre_cyc;
		struct {
			ER_ID cycid;
			ER ret;
		} del_cyc;
		struct {
			ER_ID cycid;
			ER ret;
		} sta_cyc;
		struct {
			ER_ID cycid;
			ER ret;
		} stp_cyc;
    struct {
    	ALM_TYPE type;
    	void *exinf;
    	TMR_CALLRTE func;
    	ER_ID ret;
		} acre_alm;
		struct {
			ER_ID almid;
			ER ret;
		} del_alm;
		struct {
			ER_ID almid;
			int msec;
			ER ret;
		} sta_alm;
		struct {
			ER_ID almid;
			ER ret;
		} stp_alm;
		struct {
      SOFTVEC type;
      IR_HANDL handler;
      ER ret;
    } def_inh;
		struct {
			int tskpri;
			ER ret;
		} rot_rdq;
		struct {
			ER_ID ret;
		} get_tid;
		struct {
			int dummy;
		} dis_dsp;
		struct {
			int dummy;
		} ena_dsp;
		struct {
			BOOL ret;
		} sns_dsp;
		struct {
			int dummy;
		} set_pow;
		/* サービスコールのみとなるので，実際はいらないが，他と一貫性と保つため */
		struct {
			SCHDUL_TYPE type;
			long schdul_param;
			ER ret;
		} sel_schdul;
		struct {
			ROL_ATR atr;
			ER ret;
		} rol_sys;
  } un;
} SYSCALL_PARAMCB;


#endif
