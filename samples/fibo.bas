REM ===============================================
REM basic fibo test performance
REM
REM    6482 ms @ 80MHz from HUB
REM    8642 ms @ 96MHz from C3 Flash
REM    5401 ms @ 96MHz from HUB
REM    7202 ms @ 96MHz from SpinSocket-Flash
REM
REM ===============================================

REM ------------------------------
REM Need a deeper stack for fibo
REM
option stacksize=256

include "print.bas"
include "propeller.bas"

def fibo(n)
    if n < 2 then
        return n
    else
        return fibo(n-1) + fibo(n-2)
    end if
end def

def test(count, endcount)
    dim result
    dim ms, ticks
    dim start
    dim passed

    do while (count <= endcount)
        print "FIBO( ";count;" )",
        start = CNT
        result = fibo(count)
        passed = CNT
        ticks = (passed-start) 
        ms = ticks/(clkfreq/1000)
        print " = ", result, " ("; ticks; "  ", "ticks)", " ("; ms; " ", "ms)"
        count = count + 1
    loop
end def


REM ===============================================
REM main program loop
REM

print
do
    dim ticks
    ticks = CNT
    test(0,25)
    ticks = CNT-ticks
    print "Total run time: "; ticks/(clkfreq/1000); " ms"
loop

REM ===============================================

