include "print.bas"

for i = 0 to 20
  print i; " is ";
  select i
    case 0 to 5
      print "between 0 and 5"
    case 6, 8
      print "6 or 8"
    case 9 to 12, 15
      print "between 9 and 12 or 15"
    case else
      print "something else"
  end select
next i
