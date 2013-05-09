; haribote-os boot asm
; TAB=4

BOTPAK	EQU		0x00280000		; bootpack偺儘乕僪愭
DSKCAC	EQU		0x00100000		; 僨傿僗僋僉儍僢僔儏偺応強
DSKCAC0	EQU		0x00008000		; 僨傿僗僋僉儍僢僔儏偺応強乮儕傾儖儌乕僪乯

; BOOT_INFO娭學
CYLS	EQU		0x0ff0			; 僽乕僩僙僋僞偑愝掕偡傞
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 怓悢偵娭偡傞忣曬丅壗價僢僩僇儔乕偐丠
SCRNX	EQU		0x0ff4			; 夝憸搙偺X
SCRNY	EQU		0x0ff6			; 夝憸搙偺Y
VRAM	EQU		0x0ff8			; 僌儔僼傿僢僋僶僢僼傽偺奐巒斣抧

		ORG		0xc200			; 偙偺僾儘僌儔儉偑偳偙偵撉傒崬傑傟傞偺偐

; 夋柺儌乕僪傪愝掕

		MOV		AL,0x13			; VGA僌儔僼傿僢僋僗丄320x200x8bit僇儔乕
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 夋柺儌乕僪傪儊儌偡傞乮C尵岅偑嶲徠偡傞乯
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; 僉乕儃乕僪偺LED忬懺傪BIOS偵嫵偊偰傕傜偆

		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC关闭一切中断
;	根据AT兼容机的规格，如果要初始化PIC
;	必须在CLI之前进行，否则有时会挂起
;	随后进行PIC初始化

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 如果要连续执行OUT指令，有些机种会无法正常运行
		OUT		0xa1,AL

		CLI						; 禁止CPU级别的中断

; 为了让CPU访问1M以上的内存空间

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20 该信号线能使内存的1MB以上的部分编程可使用状态
		OUT		0x60,AL
		CALL	waitkbdout

; 切换到保护模式

[INSTRSET "i486p"]				; 想要使用486指令的叙述

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0			; CR0:control register 0 只有操作系统能操作的寄存器，很重要
		AND		EAX,0x7fffffff	; 设bit31为0（为了进制颁）
		OR		EAX,0x00000001	; 设bit0为1（为了切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush   ; pipeline管道
pipelineflush:
		MOV		AX,1*8			;  可读写的段32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的传送

		MOV		ESI,bootpack	; 传送源
		MOV		EDI,BOTPAK		; 传送目的地
		MOV		ECX,512*1024/4
		CALL	memcpy

; 磁盘数据最终传送到它本来的位置去

; 首先从启动扇区开始

		MOV		ESI,0x7c00		; 传送源
		MOV		EDI,DSKCAC		; 传送目的地
		MOV		ECX,512/4
		CALL	memcpy

; 所有剩下的

		MOV		ESI,DSKCAC0+512	; 传送源
		MOV		EDI,DSKCAC+512	; 传送目的地
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数变换为字节数/4
		SUB		ECX,512/4		; 减去IPL
		CALL	memcpy

; 必须由asmhead完成的工作，至此全部完毕
;	以后就交由bootpack来完成

; bootpack的启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要传送的东西时，JZ: jump if zero
		MOV		ESI,[EBX+20]	; 转送源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 转送目的地
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02		
		JNZ		waitkbdout		; AND的结果如果不是0，就跳到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 减法运算的结果如果不是0，就跳转到memcpy
		RET


		ALIGNB	16
GDT0:
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; 可以读写的段(segment)32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 可以执行的段(segment)32bit (bootpack用)
		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
