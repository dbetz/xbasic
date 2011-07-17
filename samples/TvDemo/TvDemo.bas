
include "print.bas"
include "propeller.bas"
include "TvText.bas"

REM startup program
main

REM main function
def main
    TvText_start(12)
    do
        test
    loop
end def

def test
    dim length = 128
    doChars(TvText_screensize+TvText_cols)
    doOutXYprint(length)
    doRandomChars(length)
    doPrintTest(length)
end def

def doChars(length)
    dim n = 0
    TvText_setpalette_color(0,0x2E,0x02)
    do while n < length
        TvText_putchar('0'+n)
        n = n + 1
    loop
    waitcnt(clkfreq+cnt)
end def

def doOutXYprint(length)
    dim bg = 0xAA
    dim fm = 0xF7
    TvText_out(0)
    REM TvText_out row col test
    for n = 0 to length
        TvText_setpalette_color(0,0x08 | (n & fm),bg)
        TvText_out(TvText_OUTX)
        TvText_out(n MOD TvText_x2cols)
        TvText_out(TvText_OUTY)
        TvText_out(n MOD TvText_rows)
        TvText_out('0'+n)
        waitcnt(clkfreq/100+cnt)
    next n
    TvText_setpalette_color(0,0x5E,bg)
    waitcnt(clkfreq/2+cnt)
end def

def doRandomChars(length)
    dim mask = 0x7f
    dim value = 0
    dim n = 0

    TvText_out(0)
    REM random character positions
    for n = 0 to length
        TvText_setTextXY(random, random)
        TvText_out('0' + (n & mask))
    next n
    waitcnt(clkfreq+cnt)

    REM set background color
    TvText_setpalette_color(0,0x9E,0x2A)
    waitcnt(clkfreq+cnt)
    TvText_out(0)
    waitcnt(clkfreq/2+cnt)
end def

def doPrintTest(length)
    dim mask = 0x7f
    dim modmask = 0x7ff
    dim value = 0
    dim n = 0

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
    waitcnt(clkfreq+cnt)
    TvText_out(0)
    TvText_setpalette_color(0,0x07,0x02)
    waitcnt(clkfreq/4+cnt)
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

REM===================================================
REM return a positive random number
REM
dim lfsr = 1
def random
    REM taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1
    lfsr = (lfsr >> 1) ^ (-(lfsr & 1) & 0xD0000001) 
    REM lfsr = (lfsr >> 1) ^ (-(lfsr & 1) & 0xD0000081) 
    return lfsr & 0x7fffffff
end def

def seed(num)
    lfsr = num
/*
    dim n = 10
    lfsr = num
    do while n
        random
        n = n - 1
    loop
*/
end def
