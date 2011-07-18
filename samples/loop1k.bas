REM
REM xbasic loop1k.bas test - 12ms on 96MHz hub
REM
include "propeller.bas"
include "print.bas"

def test(length)

    dim start, elapsed
    dim one = 1
    start = cnt
    do while length
        length = length - one
    loop
    elapsed = cnt - start

    print elapsed / (clkfreq / 1000); " milliseconds"

end def

REM Run loop test for 1000 iterations
REM
test(1000)
do
loop

