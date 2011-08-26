set OS=cygwin
set CYGWIN=nodosfilewarning
set XB_BIN=..\..\bin\%OS%
set XB_INC=..\..\include
%XB_BIN%\xbcom -b %1 -p %2 xbfft.bas -r -t %4 %5 %6 %7
