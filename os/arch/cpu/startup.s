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


	.h8300h
	.section .text
	.global	_start
#	.type	_start,@function
_start:
	mov.l	#_bootstack,sp
	jsr	@_main

1:
	bra	1b

	.global	_dispatch
#	.type	_dispatch,@function
_dispatch:
	mov.l	@er0,er7
	mov.l	@er7+,er0
	mov.l	@er7+,er1
	mov.l	@er7+,er2
	mov.l	@er7+,er3
	mov.l	@er7+,er4
	mov.l	@er7+,er5
	mov.l	@er7+,er6
	rte
