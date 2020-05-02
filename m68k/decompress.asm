
; ===============================================================
; NOTICE
; ===============================================================
; The following snippet is the original decompression code by
; Konami, diassembled from the "Contra Hard Corps" ROM.
; 
; The code syntax targets the ASM68K assembler.
; Modify for the assembler of your choice if neccessary.
; ===============================================================


; ---------------------------------------------------------------
; Konami's LZSS variant 1 (LZKN1) decompressor
; ---------------------------------------------------------------
; INPUT:
;       a5      Input buffer (compressed data location)
;       a6      Output buffer (decompressed data location)
;
; USES:
;       d0-d2, a0, a5-a6
; ---------------------------------------------------------------

KonDec:
        move.w  (a5)+,d0        ; read uncompressed data size   // NOTICE: not used
        bra.s   @InitDecomp

; ---------------------------------------------------------------
; Alternative (unused) decompressor entry point
; Skips uncompressed data size at the start of the buffer
;
; NOTICE: This size is not used during decompression anyways.
; ---------------------------------------------------------------

KonDec2:
        addq.l  #2,a5           ; skip data size in compressed stream
; ---------------------------------------------------------------

@InitDecomp:
        lea     (@MainLoop).l,a0
        moveq   #0,d7           ; ignore the following DBF

@MainLoop:
        dbf     d7,@RunDecoding ; if bits in decription field remain, branch
        moveq   #7,d7           ; set repeat count to 8
        move.b  (a5)+,d1        ; fetch a new decription field from compressed stream

@RunDecoding:
        lsr.w   #1,d1           ; shift a bit from the description bitfield
        bcs.w   @DecodeFlag     ; if bit=1, treat current byte as decompression flag
        move.b  (a5)+,(a6)+     ; if bit=0, treat current byte as raw data
        jmp     (a0)            ; back to @MainLoop
; ---------------------------------------------------------------

@DecodeFlag:
        moveq   #0,d0
        move.b  (a5)+,d0        ; read flag from a compressed stream
        bmi.w   @Mode10or11     ; if bit 7 is set, branch
        cmpi.b  #$1F,d0
        beq.w   @QuitDecomp     ; if flag is $1F, branch
        move.l  d0,d2           ; d2 = %00000000 0ddnnnnn
        lsl.w   #3,d0           ; d0 = %000000dd nnnnn000
        move.b  (a5)+,d0        ; d0 = Displacement (0..1023)
        andi.w  #$1F,d2         ; d2 = %00000000 000nnnnn
        addq.w  #2,d2           ; d2 = Repeat count (2..33)
        jmp     (@UncCopyMode).l
; ---------------------------------------------------------------

@Mode10or11:
        btst    #6,d0
        bne.w   @CompCopyMode   ; if bits 7 and 6 are set, branch
        move.l  d0,d2           ; d2 = %00000000 10nndddd
        lsr.w   #4,d2           ; d2 = %00000000 000010nn
        subq.w  #7,d2           ; d2 = Repeat count (1..4)
        andi.w  #$F,d0          ; d0 = Displacement (0..15)

@UncCopyMode:
        neg.w   d0              ; negate displacement

@UncCopyLoop:
        move.b  (a6,d0.w),(a6)+ ; self-copy block of uncompressed stream
        dbf     d2,@UncCopyLoop ; repeat
        jmp     (a0)            ; back to @MainLoop
; ---------------------------------------------------------------

@CompCopyMode:
        subi.b  #$B9,d0         ; d0 = Repeat count (7..70)

@CompCopyLoop:
        move.b  (a5)+,(a6)+     ; copy uncompressed byte
        dbf     d0,@CompCopyLoop; repeat
        jmp     (a0)            ; back to @MainLoop
; ---------------------------------------------------------------

@QuitDecomp:
        rts
        
