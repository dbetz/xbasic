option stacksize=64

include "print.bas"
include "string.bas"
include "propeller.bas"

dim my_array(10) in "text" = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }

dim my_bytearray(10) as byte = { 2, 4, 6, 8, 10, 1, 3, 5, 7, 9 }

dim greeting(32) as byte

dim buf(20) as byte

def show(array(), bytearray() as byte, size)
  dim n
  for n = 0 to size - 1
    print n, array(n), bytearray(n)
  next n
end def

def fill(array(), bytearray() as byte, size)
  dim n
  for n = 0 to size - 1
    array(n) = n
    bytearray(n) = size - n - 1
  next n
end def

def dumpstr(str() as byte)
  dim i = 0
  do while str(i) <> 0
    print str(i)
    i = i + 1
  loop
end def

strcpy(buf, "Goodbye!")
buf(4) = 0

strcpy(greeting, "Hello, world!")
print greeting
print greeting(1)
dumpstr(greeting)
greeting(5) = 0
print greeting
print "Hello, world!"
print "Goodbye!"
print buf

show(my_array, my_bytearray, 10)
fill(my_array, my_bytearray, 10)
show(my_array, my_bytearray, 10)
