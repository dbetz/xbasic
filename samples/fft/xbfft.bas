REM =================================================================
REM FFT benchmark program for xbasic
REM This version is one COG and does not use PASM butterflies
REM =================================================================

include "fft.bas"
include "math.bas"
include "print.bas"
include "propeller.bas"

REM =================================================================
REM constants and data
REM
def FFT_FFT_SIZE        = 1024
def FFT_LOG2_FFT_SIZE   = 10

def FFT_CMD_DECIMATE    = 1
def FFT_CMD_BUTTERFLY   = 2
def FFT_CMD_MAGNITUDE   = 4

REM default size is integer (long)
REM
dim bx(FFT_FFT_SIZE)
dim by(FFT_FFT_SIZE)

REM For testing define 16 samples  of an input wave form here.
dim indat() = {
    4096, 3784, 2896, 1567, 0, -1567, -2896, -3784, -4096, -3784, -2896, -1567, 0, 1567, 2896, 3784
}

REM =================================================================
REM start program
REM
main

REM finish program in a loop
do
    waitcnt(clkfreq+cnt)
loop

REM -----------------------------------------------------------------
REM main fft test
REM Test harness for the FFT
REM
def main

    print
    print "heater_fft v2.0";
    print " - ported to xbasic by ";
    print " jazzed July 2011."
    print
    print " Test time about 3 seconds";
    print " on 80MHz Propeller."
    print " Starting test ...."
    print

    REM Input some data
    fillInput

    REM The Fourier transform, including bit-reversal reordering and magnitude converstion
    startTime = cnt
    FFT_butterflies(FFT_CMD_DECIMATE | FFT_CMD_BUTTERFLY | FFT_CMD_MAGNITUDE, @bx, @by)
    endTime = cnt

    REM Print resulting spectrum
    printSpectrum

    print "1024 point FFT plus magnitude ";
    print "calculation run time = ";
    print (endTime - startTime) / (clkfreq / 1000); " ms"
    print

end def

REM -----------------------------------------------------------------
REM dim real, imag, magnitude
REM Spectrum is available in first half of the buffers after FFT.
REM
def printSpectrum
    dim i
    print "Freq. Magnitude"
    for i = 0 to (FFT_FFT_SIZE / 2)
        print i, bx(i)
    next i
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

REM -----------------------------------------------------------------
REM Fill buffer bx with samples of of an imput signal and clear by.
REM WARNING: Keep the peek amplitude < 2048.
REM
def fillInput
REM dim r, sx, sy
    dim length = FFT_FFT_SIZE
    dim one   = 1
    dim k = 0
    do while k < length

        REM Two frequencies of the waveform defined in indat
        bx(k) = (indat((3*k) mod 16) / 4) - 1
        bx(k) = bx(k) + (indat((5*k) mod 16) / 4) - 1

        REM The highest frequency
        if k & 1 then
            bx(k) = bx(k) + 200
        else
            bx(k) = bx(k) + -200
        end if

        REM A DC level
        bx(k) = bx(k) + 200

        REM Clear Y array.
        by(k) = 0
        
        k = k + one
    loop

end def

REM end of file
REM =================================================================





