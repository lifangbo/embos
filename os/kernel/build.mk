 #
 # <著作権及び免責事項>
 #
 # 　本ソフトウェアはフリーソフトです．自由にご使用下さい．
 #
 # このソフトウェアを使用したことによって生じたすべての障害,損害,不具合等に関しては,
 # 私と私の関係者及び,私の所属する団体とも,一切の責任を負いません．
 # 各自の責任においてご使用下さい
 # 
 # この<著作権及び免責事項>があるソースに関しましては,すべて以下の者が作成しました．
 # 作成者 : mtksum
 # 連絡先 : t-moteki@hykwlab.org
 #


#kernel/build.mk

# target非依存部
# kernel source
C_SOURCES += command.c kernel.c mailbox.c memory.c mutex.c prinvermutex.c ready.c scheduler.c semaphore.c srvcall.c syscall.c system_manage.c task_manage.c task_sync.c intr_manage.c time_manage.c timer_callrte.c
