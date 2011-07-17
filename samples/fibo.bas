REM ===============================================
REM basic fibo test
REM ===============================================

REM ------------------------------
REM Need a deeper stack for fibo
REM
option stacksize=256

include "print.bas"
include "propeller.bas"

dim freq

def fibo(n)
    if n > 2 then
        return fibo(n-1) + fibo(n-2)
    else
        return 1
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
        if count = 0 then
            result = 0
        else
            result = fibo(count)
        end if
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

