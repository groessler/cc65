;
; Ullrich von Bassewitz, 06.08.1998
;
; void __fastcall__ gotoxy (unsigned char x, unsigned char y);
; void __fastcall__ gotox (unsigned char x);
;

        .export         gotoxy, _gotoxy, _gotox
        .import         popa, VTABZ

        .include        "apple2.inc"

gotoxy:
        jsr     popa            ; Get Y

_gotoxy:
        clc
        adc     WNDTOP
        sta     CV              ; Store Y
        jsr     VTABZ
        jsr     popa            ; Get X

_gotox:
        sta     CH              ; Store X
        .ifdef  __APPLE2ENH__
        bit     RD80VID         ; In 80 column mode?
        bpl     :+
        sta     OURCH           ; Store X
:       .endif
        rts
