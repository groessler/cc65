;
; set left margin (LMARGN) to 0
;

                .include        "atari.inc"
                .constructor    lmarginprep, 24
                .forceimport    lmargindone
                .export         __LMARGN_save                   ; original LMARGN setting
                

                .code

lmarginprep:    lda     LMARGN
                sta     __LMARGN_save
                lda     #0
                sta     LMARGN
                rts

                .bss

__LMARGN_save:  .res    1

                .end
