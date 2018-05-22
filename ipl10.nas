; haribote-ipl
; TAB=4
		org	0X7c00
		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; 启动区名称可以是任意字符串（8字节）
		DW		512				; 每个扇区的大小（必须为512字节）
		DB		1				; 簇的大小（必须为1个扇区）
		DW		1				; FAT的其实位置（一般从第一个扇区开始）
		DB		2				; FAT的个数（必须为2）
		DW		224				; 根目录大小（一般设为224项）
		DW		2880			; 该硬盘大小（必须设为2880扇区）
		DB		0xf0			; 硬盘的种类（必须设为0xf0）
		DW		9				; FAT的长度（必须为9个扇区）
		DW		18				; 1个磁道有几个扇区（必须为18）
		DW		2				; 磁头数（必须是2）
		DD		0				; 不使用分区（必须是0）
		DD		2880			; 重写一次硬盘大小
		DB		0,0,0x29		; 意义不明，固定
		DD		0xffffffff		; （可能是）卷标号码
		DB		"HARIBOTEOS "	; 磁盘的名称（11字节）
		DB		"FAT12   "		; 硬盘格式名称（8字节）
		RESB	18				; 先空出18字节
		
entry：
		mov		ax,0 			;寄存器初始化
		mov 	ss,ax 
		mov 	sp,0X7c00 
		mov 	ds,ax 
		
;磁盘读取
		mov 	ax,0x0820 
		mov 	es,ax 
		mov 	ch,0 			;选取柱面0
		mov 	dh,0 			;选取磁头0
		mov 	cl,2 			;选取扇区2
		
readloop:
		mov 	si,0 			;失败次数
retry：		
		mov 	ah,0x02 		;ah=0x02:读入磁盘
		mov 	al,1 			;1个扇区
		mov 	bx,0 
		mov 	dl,0x00 		;A驱动器
		int 	0x13 			;调用bios13号中断
		JNC		next			; エラーがおきなければnextへ
		ADD		SI,1			; SIに1を足す
		CMP		SI,5			; SIと5を比較
		JAE		error			; SI >= 5 だったらerrorへ
		MOV		AH,0x00
		MOV		DL,0x00			; Aドライブ
		INT		0x13			; ドライブのリセット
		JMP		retry
next:
		MOV		AX,ES			; アドレスを0x200進める
		ADD		AX,0x0020
		MOV		ES,AX			; ADD ES,0x020 という命令がないのでこうしている
		ADD		CL,1			; CLに1を足す
		CMP		CL,18			; CLと18を比較
		JBE		readloop		
		
;出现情况

fin:
		HLT 					;cpu停止运行
		jmp 	fin 			;循环
		
error:
		mov 	si,msg
putloop:
		mov 	al,[si]
		add 	si,1 			;si自加
		cmp 	al,0 
		je 		fin 
		mov 	ah,0x0e 		;显示一个文字 
		mov 	bx,15 			;指定字符颜色
		int 	0x10 			;调用bios10号中断
		jmp 	putloop 
		
msg:
		db 		0x0a, 0x0a 		;换行
		db 		"load error"
		db 		0x0a 			;换行
		db 		0 
		
		resb 	0x7dfe-$		;此处到0x7dfe全填0x00
		
		db 		0x55, 0xaa 		;启动程序标志