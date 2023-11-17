START   =   $f9                 ; Start address, 2 bytes
END     =   $fb                 ; End address, 2 bytes
DATA    =   $fd                 ; Data address, 2 bytes
SIZE    =   $ff                 ; Size of packet
SRC     =   $60                 ; Source packet, 8 bytes
DEST    =   $68                 ; Destination packet, 8 bytes
*       =   $1800

; Pack a single packet
; of 7 bytes into 8 bytes
; and continue if there's more to do
Pack:       jsr Prepare         ; Move data from START to SRC
            ldy #0              ; Initialize index and
            sta DEST            ;   pack byte
-loop:      lda SRC,y           ; Get unpacked source byte
            asl                 ; Set carry if high bit set
            ror DEST            ; Move carry into pack byte
            lsr                 ; Return original, but without bit 7
            iny                 ; Do this for 7 bytes   
            sta DEST,y          ; Save to packed destination
            cpy #7              ; ,,
            bne loop            ; ,,
            lsr DEST            ; Last shift for bit 7 (always 0)
            ldy #8              ; Transcribe DEST to DATA
            jsr Xscribe         ; ,,
            bcc Pack            ; Keep going if haven't reached END
            rts

; Unpack a single packet    
; of 8 bytes into 7 bytes 
; and continue if there's more to do  
Unpack:     jsr Prepare         ; Move data from START to SRC
            ldy #0              ; Initialize index
-loop:      lda SRC+1,y         ; Get packed source byte
            ror SRC             ; If it's pack bit is set,
            bcc nx              ;   then append it to
            ora #$80            ;   bit 7
nx:         sta DEST,y          ; Save to unpacked destination
            iny                 ; Do this for 7 bytes
            cpy #7              ; ,,
            bne loop            ; ,,
            ldy #7              ; Transcribe DEST to DATA
            jsr Xscribe         ; ,,
            bcc Unpack          ; Keep going if haven't reached END
            rts

; Prepare
; data for packing or unpacking
; by copying bytes from START to the SRC packet
Prepare:    ldy #7              ; Copying 8 bytes from start region to the
-loop:      lda (START),y       ;   source packet. For a packing operation, the
            sta SRC,y           ;   8th byte is just not seen, so it's okay to
            dey                 ;   always copy 8.
            bpl loop            ;   ,,
            rts

; Transcribe
; destination packet DEST to data region DATA
; Y is number of bytes to transcribe, meaning
;   Y=7 for unpack operation.
;   Y=8 for pack operation
; If carry is set on return, then the entire
; pack/unpack operation is complete.
Xscribe:    sty SIZE            ; Keep track of packet size for later
            ldy #0              ; Initialize transcribe index
-loop:      lda DEST,y          ; Get the packet destination byte
            sta (DATA),y        ; Transcribe it to the data region
            inc START           ; Increment start region pointer
            bne ch_end          ;   in order to compare it to the end on a
            inc START+1         ;   byte-by-byte basis
ch_end:     lda START+1         ; Has the start page reached the end page?
            cmp END+1           ; If not, continue
            bcc cont            ; ,,
            lda START           ; If so, has the start byte reached the end?
            cmp END             ; ,,
            bcs xs_r            ; If so, return with carry set (meaning, done)
cont:       iny                 ; Continue by incrementing transcribe index
            cpy SIZE            ; Have we reached the packet size?
            bne loop            ; ,,
            lda SIZE            ; If end of packet, advance the data region
            clc                 ;   pointer by the packet size
            adc DATA            ;   ,,
            sta DATA            ;   ,,
            bcc xs_r            ;   ,, (return with carry clear, more to do)
            inc DATA+1          ;   ,,
            clc                 ;   ,, Force clear carry for return
xs_r:       rts
