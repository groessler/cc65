;
; restore LMARGN setting
;

                .include        "atari.inc"
                .destructor     lmargindone, 24
                .import         __LMARGN_save

lmargindone:    lda     __LMARGN_save
                sta     LMARGN
                rts

                .end
