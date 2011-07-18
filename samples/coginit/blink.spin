PUB start | count
  count := 10
  cognew(@entry, @count)
  repeat

DAT

entry                   rdlong  t1, par
                        rdlong  rate, #0        ' get clkfreq
                        shr     rate, #1
                        mov     next_wait, rate
                        add     next_wait, cnt
                        andn    outa, ledpin
                        or      dira, ledpin

:loop                   or      outa, ledpin
                        waitcnt next_wait, rate
                        andn    outa, ledpin
                        waitcnt next_wait, rate
                        djnz    t1, #:loop

                        cogid   t1
                        cogstop t1

ledpin                  long    1<<15
t1                      res     1
rate                    res     1
next_wait               res     1
