; $Id: interp.s,v 1.8 2013/11/11 23:22:53 jsmcortina Exp $

; * Copyright 2007, 2008, 2009, 2010, 2011, 2012 James Murray and Kenneth Culver
; *
; * This file is a part of Megasquirt-3.
; *
; interp.s from Robert Swan
; minor ports to MS3 James Murray / Kenneth Culver
; intrp_1dctableu by James Murray derived from intrp_1dctable
; *
; * You should have received a copy of the code LICENSE along with this source, please
; * ask on the www.msextra.com forum if you did not.
; *
.sect	.text
	XDEF	intrp_1ditable
	XDEF	intrp_1dctable
	XDEF	intrp_1dctableu
	XDEF	intrp_2ditable
	XDEF	intrp_2dctable
	XDEF	intrp_2dcstable

        nolist               ;turn off listing
        include "ms3h.inc"
        list

;
; MSII interpolation functions based on Al Grippo's C functions of
; the same names.  (C) Robert Swan
;
; Standard parsing of range arguments for intrp_1d[ci]table
; Inputs:
;   D		-- x (int) value to look up
;   5,SP	-- nx (uchar) number of values in range
;   6,SP	-- xtbl (int *) table of nx ranges
; Outputs:
;   A		-- xidx (uchar) offset to base of range
;   X		-- xprop (uint) proportion of xtbl[xidx:xidx+1] for x
;
RANGE1D:
	TFR	D,Y	; value to look up
	LDX	6,SP	; base of table
	LDAA	5,SP	; number of entries
	; fall through to POLYSEG
;
; POLYSEG, POLYSEGU -- identify polyline segment and proportion
;                      (signed/unsigned)
;
; Determine which of several ranges a 2-byte integer falls within
; given:
;    Y  --  the number in question
;    X  --  the base of the list of numbers
;    A  --  the number of values in the list
; Return
;    A  --  the offset of the range base
;    X  --  the 16-bit proportion of the way through the range
;
POLYSEG:
	DECA		; correct to 0-based array
	LSLA		; offset to last element in list
	CPY	A,X
	BGE	OUTSIDE	; above the maximum.
	BRA	LENTRY
CPLOOP:
	CPY	A,X
	BGE	FOUND
LENTRY:
	SUBA	#2
	BGE	CPLOOP
UNDER:
	CLRA		; less than the first element, so point to it.
OUTSIDE:
	LDX	#0	; outside range -- fraction must always be 0
	RTS

RANGE1DU:
	TFR	D,Y	; value to look up
	LDX	6,SP	; base of table
	LDAA	5,SP	; number of entries
	; fall through to POLYSEGU
;
; Unsigned variant -- much the same
;
POLYSEGU:
	DECA		; correct to 0-based array
	LSLA		; offset to last element in list
	CPY	A,X
	BHS	OUTSIDE	; above the maximum.
	BRA	LENTRYU
CPLOOPU:
	CPY	A,X
	BHS	FOUND
LENTRYU:
	SUBA	#2
	BGE	CPLOOPU
	BRA	UNDER	; lower than the first category
;
; Range found.  B contains its index, X points to its base.
; Use FDIV to determine the proportion through the range.
;
FOUND:
	LEAX	A,X	; point right at relevant slot
	PSHA		; keep offset to return to caller
	TFR	Y,D	; x ==> D
	SUBD	0,X	; now x - X0
	PSHD
	LDD	2,X	; X1 ==> D
	SUBD	0,X	; now X1 - X0
	TFR	D,X	; denominator
	PULD		; numerator
	FDIV		; (x - X0)/(X1 - X0) ==> X
	PULA		; A is the offset to the slot
	RTS
;
; Parse 2D range arguments as given to intrp_2d[ic]table
; Inputs:
;   D		-- xval (uint)
;   4,SP	-- yval
;   7,SP        -- nx (uchar)
;   9,SP        -- ny (uchar)
;   10,SP       -- xtbl (uint *)
;   12,SP       -- ytbl (int *)
; Outputs
;   4,SP	-- xidx 8-bit index of base of range in xtbl containing x
;   10,SP	-- xprop 16-bit proportion of xtbl[xidx:xidx+1] for x
;   A		-- yidx 8-bit index of base of range in ytbl containing y
;   X		-- yprop 16-bit proportion of ytbl[yidx:yidx+1] for y
;
RANGE2D:
	TFR	D,Y	; x value to look up
	LDX	10,SP	; base of x table
	LDAA	7,SP	; number of entries in xtbl
	BSR	POLYSEGU	; x range offset ==> A, proportion ==> X
	LSRA		; change offset to index
	LDY	4,SP	; y value to look up
	STX	10,SP	; reuse xtbl slot for xprop
	STAA	4,SP	; reuse yidx slot for x offset
	LDX	12,SP	; base of y table
	LDAA	9,SP	; number of entries in ytbl
	BSR	POLYSEG	; y table values calculated
	LSRA
	RTS
;
; Interpolate x within a 1D table of signed characters
; Inputs:
;   D		-- x (int) value to interpolate
;   3,SP	-- nx (uchar) # values in xtbl
;   4,SP	-- xtbl (int *) base of table
;   7,SP	-- sgn (uchar) if != 0, ztbl is signed values
;   8,SP	-- ztbl (uint *) table to interpolate within
;   11,SP        -- page (uchar) page to switch to for interpolation
; Outputs:
;   D		-- interpolated value
;
intrp_1dctable:
        XGDY
        CLRA
        LDAB    11,SP
        ASLD
        ASLD
        ASLD
        XGDX
        LEAX    tables+6,X
        MOVB    0x0,X, RPAGE
        XGDY

	BSR	RANGE1D	; offset --> A, prop --> X
	TST	7,SP	; fancy handling if signed
	BEQ	SIGNOK
SIGNNOK:
	LDAB	#128	; 0x80
	STAB	7,SP	; XOR with this to convert to unsigned
SIGNOK:
	LSRA		; 1 byte per slot
	LDY	8,SP	; ztbl
	LEAY	A,Y	; base slot
	LDAA	0,Y
	EORA	7,SP
	CLRB		; extend to 16 bit with large fraction portion
	STD	4,-SP
	LDAA	1,Y
	EORA	11,SP
	STD	2,SP
	TFR	X,D
	TFR	SP,X
	JSR	ETBLW	; interpolate the two characters as integers
	LEAS	4,SP	; free temporary space
	EORA	7,SP	; restore sign
	LSLB
	ADCA	#0	; round up if MSB of B is set
	SEX	A,D	; sign extend by default
	TST	7,SP
	BNE	SEXOK
	CLRA
SEXOK:
	RTS

;
; Interpolate unsigned x within a 1D table of unsigned characters
; Inputs:
;   D		-- x (unsigned int) value to interpolate
;   3,SP	-- nx (uchar) # values in xtbl
;   4,SP	-- xtbl (int *) base of table
;   7,SP	-- sgn (uchar) if != 0, ztbl is signed values
;   8,SP	-- ztbl (uint *) table to interpolate within
;   11,SP        -- page (uchar) page to switch to for interpolation
; Outputs:
;   D		-- interpolated value
;
intrp_1dctableu:
        XGDY
        CLRA
        LDAB    11,SP
        ASLD
        ASLD
        ASLD
        XGDX
        LEAX    tables+6,X
        MOVB    0x0,X, RPAGE
        XGDY

	BSR	RANGE1DU	; offset --> A, prop --> X
	TST	7,SP	; fancy handling if signed output
	BEQ	SIGNOK
    BRA SIGNNOK

;
; Interpolate x,y within a 2D table of unsigned characters
; Inputs:
;   D		-- x (uint) x value to look up
;   2,SP	-- y (int) y value to look up
;   5,SP	-- nx (uchar) # values in xtbl
;   7,SP	-- ny (uchar) # values in ytbl
;   8,SP	-- xtbl (uint *) table of x control points
;   10,SP	-- ytbl (int *) table of y control points
;   12,SP	-- ztbl (int *) table of nx*ny values to interpolate
;   15,SP	-- hires (uchar) if != 0, return result * 10
;   17,SP       -- page (uchar) page to set RPAGE to.
; Outputs:
;   D		-- interpolated value
;
intrp_2dctable:
        XGDY
        CLRA
        LDAB    17,SP
        ASLD
        ASLD
        ASLD
        XGDX
        LEAX    tables+6,X
        MOVB    0x0,X,  RPAGE
        XGDY

	BSR	RANGE2D ; yidx --> A, yprop --> X; x values in y & xtbl
	STX	10,SP
	LDY	12,SP	; base of table to look up
	LDAB	5,SP	; row width
	MUL
	LEAY	D,Y	; start of relevant row --> Y
	LDAB	2,SP
	LEAY	B,Y	; bottom left corner of cell --> Y
; expand values for accuracy.  Multiply hires by 160, otherwise by 16.
	LDAB	15,SP	; hires flag
	BEQ	NOHIRES
	LDAB	#144	; will become #160
NOHIRES:
	ADDB	#16	; scale bytes by 16 or 160
	LEAS	-8,SP	; space for two working cells plus two for storage
	STAB	6,SP	; use the fourth storage cell for the scale value
	LDAA	0,Y
	MUL		; scale lower left cell
	STD	0,SP	; X00 --> 0,SP 
	LDAA	1,Y
	LDAB	6,SP
	MUL		; scale lower right cell
	STD	2,SP	; X10 --> 2,SP
	TFR	SP,X	; base of values
	LDD	16,SP	; propx [z00, z10] --> D
	BSR	ETBLW
	STD	4,SP
	LDAA	13,SP	; row width
	LEAY	A,Y	; next row up
	LDAA	0,Y
	LDAB	6,SP
	MUL		; scale upper left cell
	STD	0,SP
	LDAA	1,Y
	LDAB	6,SP
	MUL		; scale upper right cell
	STD	2,SP
	TFR	SP,X
	LDD	16,SP	; propx
	BSR	ETBLW	; propx[z01, z11] --> D
	STD	6,SP	; overwrite scale.  Not needed anymore.
	LEAX	4,SP
	LDD	18,SP	; propy
	BSR	ETBLW	; propy[propx[z00, z10], propx[z01, z11]] --> D
	LEAS	8,SP	; free temporary space
	LSRD		; divide by 16
	LSRD
	LSRD
	LSRD
	ADCB	#0	; round up if last shift carried
	ADCA	#0
	RTS
;
; Signed branch of intrp_1ditable (below).  Transfer the values to
; temp storage, turn them into unsigned, interpolate and adjust.
;
DOSIGN:
	STD	6,-SP	; push prop and make working space
	LDD	0,X	; z0
	EORA	#128
	STD	2,SP
	LDD	2,X	; z1
	EORA	#128
	STD	4,SP
	PULD		; restore prop
	TFR	SP,X
	BSR	ETBLW
	EORA	#128	; correct sign
	LEAS	4,SP
	RTS
;
; Interpolate x within a 1D table of integers
; Inputs:
;   D		-- x (int) value to interpolate
;   3,SP	-- nx (uchar) # values in xtbl
;   4,SP	-- xtbl (int *) base of table
;   7,SP	-- sgn (char) true if ztbl is signed
;   8,SP	-- ztbl (uint *) table to interpolate within
;   11,SP       -- page (uchar) page to set RPAGE to
; Outputs:
;   D		-- interpolated value
;
intrp_1ditable:
        XGDY
        CLRA
        LDAB    11,SP
        ASLD
        ASLD
        ASLD
        XGDX
        LEAX    tables+6,X
        MOVB    0x0,X,  RPAGE
        XGDY

	JSR	RANGE1D	; offset --> A, prop --> X
	SEX	A,D
	ADDD	8,SP	; base of pertinent range --> D
	EXG	D,X	; prop in D, base in X
	TST	7,SP	; is it signed?
	BNE	DOSIGN
	; *** unsigned... fall through to ETBLW
;
; Extended precision TBL function.  The CPU's internal ones don't
; quite get the resolution needed.
; Inputs:
;   X		-- xptr (int *) pointer to first of two signed integers
;   D		-- prop (uint) proportion [0:FFFF] between xptr[0:1]
; Outputs:
;   D		-- intrp (int) interpolated value
;
ETBLW:
	PSHY		; restore at end
	TFR	D,Y	; keep hold of prop
	LDD	2,X	; x2
	SUBD	0,X	; x2 - x1
	BHS	ARGRDY
	; need to reverse arguments
	NEGA
	NEGB
	SBCA	#0	; negate difference
	EXG	D,Y	; complement prop
	COMB
	COMA
	EXG	D,Y
	LEAX	2,X	; origin is now second arg
ARGRDY:
	EMUL		; multiply +ve difference by prop
	LSLA		; put MS bit of D in carry for rounding
	TFR	Y,D
	ADCB	1,X	; round up if needed
	ADCA	0,X
	PULY
	RTS
;
; Interpolate x,y within a 2D table of integers
; Inputs:
;   D		-- x (uint) x value to look up
;   2,SP	-- y (int) y value to look up
;   5,SP	-- nx (uchar) # values in xtbl
;   7,SP	-- ny (uchar) # values in ytbl
;   8,SP	-- xtbl (uint *) table of x control points
;   10,SP	-- ytbl (int *) table of y control points
;   12,SP	-- ztbl (int *) table of nx*ny values to interpolate
;   15,SP       -- page (uchar) page to set RPAGE to
; Outputs:
;   D		-- interpolated value
;
intrp_2ditable:
        XGDY
        CLRA
        LDAB    15,SP
        ASLD
        ASLD
        ASLD
        XGDX
        LEAX    tables+6,X
        MOVB    0x0,X,   RPAGE
        XGDY

	JSR	RANGE2D	; parse x and y ranges A contains yidx, X yprop
	STX	10,SP	; keep yprop
	LDY	12,SP	; base of table to look up
	LDAB	5,SP	; get row width
	LSLB		; integers are 2 bytes wide
	MUL
	LEAY	D,Y	; Y points to start of relevant row
	LDAB	2,SP	; xidx
	LSLB
	LEAY	B,Y	; Y points to bottom left point of grid cell
	TFR	Y,X	; base
; Weighted average by three interpolations
; Copy values out.  Need to convert them for unsigned arithmetic.
	LDD	0,X	; lower left
	EORA	#128	; unsigned
	STD	6,-SP	; space for 3 ints on stack
	LDD	2,X	; lower right
	EORA	#128
	STD	2,SP
	TFR	SP,X
	LDD	14,SP	; xprop
; weighted average of lower edge
	BSR	ETBLW	; propx[z00, z10] --> D
	STD	0,SP	; lower interpolation
	LDAA	11,SP	; row width
	LSLA
	LEAY	A,Y	; base of next row up
	LDD	0,Y	; upper left
	EORA	#128
	STD	2,SP
	LDD	2,Y	; upper right
	EORA	#128
	STD	4,SP
	LDD	14,SP	; xprop
	LEAX	2,SP
; weighted average of upper edge
	BSR	ETBLW	; propx[z01, z11] --> D
	STD	2,SP	; propx[z00, z10], propx[z01, z11] on stack
	TFR	SP,X	; base
	LDD	16,SP	; yprop
; weighted average of vertical line between
	BSR	ETBLW	; propy[prox[z00, z10],propx[z01,z11]] -> D
	LEAS	6,SP
	EORA	#128	; correct sign
	RTS
;
; Interpolate x,y within a 2D table of signed characters
; Inputs:
;   D		-- x (uint) x value to look up
;   2,SP	-- y (int) y value to look up
;   5,SP	-- nx (uchar) # values in xtbl
;   7,SP	-- ny (uchar) # values in ytbl
;   8,SP	-- xtbl (uint *) table of x control points
;   10,SP	-- ytbl (int *) table of y control points
;   12,SP	-- ztbl (char *) table of nx*ny values to interpolate
;   15,SP       -- page (uchar) page to set RPAGE to
; Outputs:
;   D		-- interpolated value
;
intrp_2dcstable:
        XGDY
        CLRA
        LDAB    15,SP
        ASLD
        ASLD
        ASLD
        XGDX
        LEAX    tables+6,X
        MOVB    0x0,X,  RPAGE
        XGDY

	JSR	RANGE2D ; yidx --> A, yprop --> X; x values in y & xtbl
	STX	10,SP	; keep yprop in ytbl slot
	LDY	12,SP	; base of table to look up
	LDAB	5,SP	; row width
	MUL
	LEAY	D,Y	; left of row --> y
	LDAB	2,SP	; offset within row
	LEAY	B,Y	; bottom left corner of cell --> Y
	LDAB	0,Y	; lower left cell
	EORB	#128	; cancel out sign
	LDAA	#16	; scale value
	MUL
	STD	8,-SP	; z00 value on top of stack
	LDAB	1,Y
	EORB	#128
	LDAA	#16
	MUL		; scale lower right cell
	STD	2,SP	; z10 --> 2,SP
	TFR	SP,X	; base of values
	LDD	16,SP	; propx [z00, z10] --> D
	JSR	ETBLW
	STD	4,SP
	LDAA	13,SP	; row width
	LEAY	A,Y	; next row up
	LDAB	0,Y
	EORB	#128
	LDAA	#16
	MUL		; scale upper left cell
	STD	0,SP	; z01 on top of stack
	LDAB	1,Y
	EORB	#128
	LDAA	#16
	MUL		; scale upper right cell
	STD	2,SP	; z11 at 2,SP
	TFR	SP,X
	LDD	16,SP	; propx
	JSR	ETBLW	; propx[z01, z11] --> D
	STD	6,SP
	LEAX	4,SP	; point to two interpolated values
	LDD	18,SP	; propy
	JSR	ETBLW	; propy[propx[z00, z10], propx[z01, z11]] --> D
	LEAS	8,SP	; free temporary space
	SUBD	#2048	; 0x5000 == 160 x 0x80 -- sign correct
	ASRA		; divide by 16
	RORB
	ASRA
	RORB
	ASRA
	RORB
	ASRA
	RORB
	ADCB	#0	; round up if last shift carried
	ADCA	#0
	RTS
