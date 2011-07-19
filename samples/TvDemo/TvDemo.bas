REM ==================================================================
REM xbasic TvDemo.bas
REM ==================================================================

include "math.bas"
include "print.bas"
include "propeller.bas"
include "TvText.bas"

REM ------------------------------------------------------------------
REM startup program
main

REM ------------------------------------------------------------------
REM main function
REM
def main
    TvText_start(12)
    do
        test
    loop
end def

REM ------------------------------------------------------------------
REM test function
REM
def test
    dim length = 128
    doChars(TvText_screensize+TvText_cols)
    doOutXYprint(length)
    doRandomChars(length)
    doPrintTest(length)
end def

REM ------------------------------------------------------------------
REM print scrolling chars
REM
def doChars(length)
    dim n = 0
    TvText_setpalette_color(0,0x07,0x02)
    do while n < length
        TvText_putchar('0'+n)
        n = n + 1
    loop
    waitcnt(clkfreq*4+cnt)
end def

REM ------------------------------------------------------------------
REM print XY chars 
REM
def doOutXYprint(length)
    dim bg = 0x02
    TvText_out(0)
    REM TvText_out row col test
    TvText_setpalette_color(0,0x9E,bg)
    for n = 0 to length
        TvText_out(TvText_OUTX)
        TvText_out(n MOD TvText_x2cols)
        TvText_out(TvText_OUTY)
        TvText_out(n MOD TvText_rows)
        TvText_out('0'+n)
        waitcnt(clkfreq/50+cnt)
    next n
    waitcnt(clkfreq/2+cnt)
end def

REM ------------------------------------------------------------------
REM print random chars at random XY positions
REM
def doRandomChars(length)
    dim mask = 0x7f
    dim value = 0
    dim n = 0

    TvText_out(0)
    REM random character positions
    for n = 0 to length
        TvText_setTextXY(RND, RND)
        TvText_out('0' + (n & mask))
    next n
    waitcnt(clkfreq+cnt)

    REM set background color
    TvText_setpalette_color(0,0x9E,0x2A)
    waitcnt(clkfreq+cnt)
    TvText_out(0)
    waitcnt(clkfreq/2+cnt)
end def

REM ------------------------------------------------------------------
REM print text, bin, hex, dec, and chars using palette
REM
def doPrintTest(length)
    dim mask = 0x7f
    dim modmask = 0x7ff
    dim value = 0
    dim n = 0

    TvText_setpalette_color(0,0x9E,0x1A)

    n = 0
    REM test printing
    for n = 0 to length
        value = '0' + (n & mask)
        TvText_str("   Hello World! ")
        TvText_bin(value,10)
        TvText_out(' ')
        TvText_hex(value,4)
        TvText_out(' ')
        TvText_putchar(value)
        TvText_out(' ')
        TvText_dec(value)
        TvText_str("    \r")
        if n then
            TvText_out(0xC)
            TvText_out(n & 7)
        end if
    next n
    waitcnt(clkfreq*2+cnt)
    TvText_out(0)
    TvText_setpalette_color(0,0x07,0x02)
    waitcnt(clkfreq+cnt)
end def

dim hex16array(16) = {
    '0', '1', '2', '3', '4', '5', '6', '7'
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
}

REM===================================================
REM Print a hexidecimal number to current text position
REM @param value - decimal number
REM @param digits - number of digits to print
REM
def printhex(value, digits)
    dim n
    do while digits > 0
        digits = digits - 1
        n = (value >> (digits<<2)) & 0xf
        uartTx(hex16array(n))
    loop
end def

