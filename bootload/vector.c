#include "defines.h"

extern void start(void);        /* �������ȡ����å� */
extern void intr_softerr(void); /* ���եȥ����������顼 */
extern void intr_syscall(void); /* �����ƥࡦ������ */
extern void intr_serintr(void); /* ���ꥢ������ */
extern void intr_timintr(void); /* �����޳���� */
extern void intr_nmiintr(void); /* NMI����� */

/*
 * ����ߥ٥��������ꡥ
 * ��󥫡�������ץȤ�����ˤ�ꡤ��Ƭ���Ϥ����֤���롥
 */
void (*vectors[])(void) = {
  start, NULL, NULL, NULL, NULL, NULL, NULL, intr_nmiintr, 
  intr_syscall, intr_softerr, intr_softerr, intr_softerr,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
  intr_timintr, intr_timintr, intr_timintr, intr_timintr, 
  intr_timintr, intr_timintr, intr_timintr, intr_timintr,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr,
};