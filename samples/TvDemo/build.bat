set OS=cygwin
set XBASIC=..\include
bstc -c -Ograux -L c:\bstc\spin TV.spin
..\bin\%OS%\bin2xbasic TV.dat TV.bas
..\bin\%OS%\xbcom -b %1 -p %2 TvDemo.bas -r %3 %4 %5 %6 %7
