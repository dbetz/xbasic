REM ===============================================
REM xbasic loop1k.bas test performance:
REM
REM    14ms @ 80MHz from HUB
REM    23ms @ 80MHz from C3 Flash (1.64*HUB)
REM    19ms @ 80MHz from SSF Flash (1.36*HUB)
REM    12ms @ 96MHz from HUB
REM    16ms @ 96MHz from SSF Flash (1.33*HUB)
REM
REM ===============================================

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
waitcnt(clkfreq+cnt) // wait for terminal startup
test(1000)
do
loop

