; =============================================================================
; Pure64 -- a 64-bit OS loader written in Assembly for x86-64 systems
; Copyright (C) 2008-2012 Return Infinity -- see LICENSE.TXT
;
; System Variables
; =============================================================================


;CONFIG
cfg_smpinit:		db 1	; By default SMP is enabled. Set to 0 to disable.
cfg_default:		db 0	; By default we don't need a config file so set to 0. If a config file is found set to 1.
cfg_e820:		db 1	; By default E820 should be present. Pure64 will set this to 0 if not found/usable.
cfg_mbr:		db 0	; Did we boot off of a disk with a proper MBR
cfg_hdd:		db 0	; Was a bootable drive detected

; Memory locations
E820Map:		equ 0x0000000000004000
InfoMap:		equ 0x0000000000005000
SystemVariables:	equ 0x0000000000005A00
ahci_cmdlist:		equ 0x0000000000070000	; 4096 bytes	0x070000 -> 0x071FFF
ahci_cmdtable:		equ 0x0000000000072000	; 57344 bytes	0x072000 -> 0x07FFFF

; DQ - Starting at offset 0, increments by 0x8
os_ACPITableAddress:	equ SystemVariables + 0x00
screen_cursor_offset:	equ SystemVariables + 0x08
hd1_maxlba:		equ SystemVariables + 0x10
os_Counter_Timer:	equ SystemVariables + 0x18
os_Counter_RTC:		equ SystemVariables + 0x20
os_LocalAPICAddress:	equ SystemVariables + 0x28
os_IOAPICAddress:	equ SystemVariables + 0x30
os_HPETAddress:		equ SystemVariables + 0x38
sata_base:		equ SystemVariables + 0x40

; DD - Starting at offset 128, increments by 4
hd1_size:		equ SystemVariables + 128
os_BSP:			equ SystemVariables + 132
drive_port:		equ SystemVariables + 136
mem_amount:		equ SystemVariables + 140

; DW - Starting at offset 256, increments by 2
cpu_speed:		equ SystemVariables + 256
cpu_activated:		equ SystemVariables + 258
cpu_detected:		equ SystemVariables + 260
ata_base:		equ SystemVariables + 262

; DB - Starting at offset 384, increments by 1
hd1_enable:		equ SystemVariables + 384
hd1_lba48:		equ SystemVariables + 385
screen_cursor_x:	equ SystemVariables + 386
screen_cursor_y:	equ SystemVariables + 387
memtempstring:		equ SystemVariables + 390
speedtempstring:	equ SystemVariables + 400
cpu_amount_string:	equ SystemVariables + 410
hdtempstring:		equ SystemVariables + 420
os_key:			equ SystemVariables + 421
os_IOAPICCount:		equ SystemVariables + 424

;MISC
screen_cols:		db 80
screen_rows:		db 25
hextable: 		db '0123456789ABCDEF'

;STRINGS
pure64:			db 'Pure64 - ', 0
msg_done:		db ' Done', 0
msg_CPU:		db '[CPU: ', 0
msg_mhz:		db 'MHz x', 0
msg_MEM:		db ']  [MEM: ', 0
msg_mb:			db ' MiB]', 0
msg_HDD:		db '  [HDD: ', 0
msg_gb:			db ' GiB]', 0
msg_loadingkernel:	db 'Loading kernel... ', 0
msg_invalidkernel:	db 'checksum failed. Boot aborted!', 0
msg_startingkernel:	db 'Starting kernel...', 0
no64msg:		db 'ERROR: This computer does not support 64-Bit mode. Press any key to reboot.', 0
initStartupMsg:		db 'Pure64 v0.6.0 - http://www.returninfinity.com', 13, 10, 13, 10, 'Initializing system... ', 0
msg_date:		db '2013/03/14', 0
;hdd_setup_no_sata:	db 'No supported SATA Controller detected', 0
hdd_setup_no_disk:	db 'No HDD detected', 0
hdd_setup_read_error:	db 'ERROR: Could not read HDD', 0


; -----------------------------------------------------------------------------
align 16
GDTR64:					; Global Descriptors Table Register
	dw gdt64_end - gdt64 - 1	; limit of GDT (size minus one)
	dq 0x0000000000001000		; linear address of GDT

gdt64:					; This structure is copied to 0x0000000000001000
SYS64_NULL_SEL equ $-gdt64		; Null Segment
	dq 0x0000000000000000
SYS64_CODE_SEL equ $-gdt64		; Code segment, read/execute, nonconforming
	dq 0x0020980000000000		; 0x00209A0000000000
SYS64_DATA_SEL equ $-gdt64		; Data segment, read/write, expand down
	dq 0x0000900000000000		; 0x0020920000000000
gdt64_end:

IDTR64:					; Interrupt Descriptor Table Register
	dw 256*16-1			; limit of IDT (size minus one) (4096 bytes - 1)
	dq 0x0000000000000000		; linear address of IDT
; -----------------------------------------------------------------------------


; =============================================================================
; EOF
