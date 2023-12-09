-P_START =   $f9                 ; Start address, 2 bytes
-P_END   =   $fb                 ; End address, 2 bytes
-P_RESULT=   $fd                 ; Result address, 2 bytes
-P_SIZE  =   $ff                 ; Size of packet
-P_SRC   =   $60                 ; Source packet, 8 bytes
-P_DEST  =   $68                 ; Destination packet, 8 bytes

; Pack a single packet
; of 7 bytes into 8 bytes
; and continue if there's more to do
Pack:       jsr Prepare         ; Move data from START to SRC
            ldy #0              ; Initialize index and
            sty P_DEST          ;   pack byte
-loop:      lda P_SRC,y         ; Get unpacked source byte
            asl                 ; Set carry if high bit set
            ror P_DEST          ; Move carry into pack byte
            lsr                 ; Return original, but without bit 7
            iny                 ; Do this for 7 bytes   
            sta P_DEST,y        ; Save to packed destination
            cpy #7              ; ,,
            bne loop            ; ,,
            lsr P_DEST          ; Last shift for bit 7 (always 0)
            ldy #8              ; Transcribe DEST to RESULT
            jsr Xscribe         ; ,,
            bcc Pack            ; Keep going if haven't reached END
            rts

; Unpack a single packet    
; of 8 bytes into 7 bytes 
; and continue if there's more to do  
Unpack:     jsr Prepare         ; Move data from START to SRC
            ldy #0              ; Initialize index
-loop:      lda P_SRC+1,y       ; Get packed source byte
            ror P_SRC           ; If its pack bit is set,
            bcc nx              ;   then append it to
            ora #$80            ;   bit 7
nx:         sta P_DEST,y        ; Save to unpacked destination
            iny                 ; Do this for 7 bytes
            cpy #7              ; ,,
            bne loop            ; ,,
            ldy #7              ; Transcribe DEST packet to RESULT
            jsr Xscribe         ; ,,
            jsr IncStart        ; Increment START once more for 8
            bcc Unpack          ; Keep going if haven't reached END
            rts

; Prepare
; data for packing or unpacking
; by copying bytes from START to the SRC packet
Prepare:    ldy #7              ; Copying 8 bytes from start region to the
-loop:      lda (P_START),y     ;   source packet. For a packing operation, the
            sta P_SRC,y         ;   8th byte is just not seen, so it's okay to
            dey                 ;   always copy 8.
            bpl loop            ;   ,,
            rts

; Transcribe
; destination packet DEST to RESULT region
; Y is number of bytes to transcribe, meaning
;   Y=7 for unpack operation.
;   Y=8 for pack operation
; If carry is set on return, then the entire
; pack/unpack operation is complete.
Xscribe:    sty P_SIZE          ; Keep track of packet size for later
            ldy #0              ; Initialize transcribe index
-loop:      lda P_DEST,y        ; Get the packet destination byte
            sta (P_RESULT),y    ; Transcribe it to the result region
            cpy #7              ; Skip the START increment for the last byte of
            beq skip_inc        ;   packing operation
            jsr IncStart        ; Increment start and check for end
            bcs xs_r            ; ,, Return with carry set if at end
            iny                 ; Continue by incrementing transcribe index
            cpy P_SIZE          ; Have we reached the packet size?
            bcc loop            ; ,,
skip_inc:   lda P_SIZE          ; If end of packet, advance the result region
            clc                 ;   pointer by the packet size
            adc P_RESULT        ;   ,,
            sta P_RESULT        ;   ,,
            bcc xs_r            ;   ,, (return with carry clear, more to do)
            inc P_RESULT+1      ;   ,,
            clc                 ;   ,, Force clear carry for return
xs_r:       rts

; Increment Start
; Returns with carry set if at (or past) end
IncStart:   inc P_START         ; Increment start region pointer
            bne ch_end          ;   in order to compare it to the end on a
            inc P_START+1       ;   byte-by-byte basis
ch_end:     lda P_END+1
            cmp P_START+1
            bcc op_done
            bne op_cont
            lda P_START         ; Has the start page reached the end page?
            cmp P_END           ; If not, continue
            rts
op_cont:    clc
            rts
op_done:    sec 
            rts 
            