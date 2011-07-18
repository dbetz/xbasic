include "propeller.bas"
include "print.bas"

def test_for(count)
  dim i, start = cnt

  for i = 1 to count
  next i

  return cnt - start
end def

def test_while(count)
  dim i, start = cnt

  i = 1
  do while i <= count
    i = i + 1
  loop

  return cnt - start
end def

print "for: "; test_for(1000) / (clkfreq / 1000)
print "while: "; test_while(1000) / (clkfreq / 1000)
