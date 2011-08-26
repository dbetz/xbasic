include "input.bas"
include "print.bas"

dim a(32) as byte = "hello there"

dim c(4)

print #1, "a="; a

input #2, "hello? "; a, b, c(2), c(0)

print "a="; a
print "b="; b

for i = 0 to 3
  print "c("; i; ")="; c(i)
next i

print "Say something? ";
inputLine(0, a)

print "a='"; a; "'"
