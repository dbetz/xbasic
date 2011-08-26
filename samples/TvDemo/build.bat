@set OS=cygwin
@set CYGWIN=nodosfilewarning
@set XB_BIN=..\..\bin\%OS%
@set XB_INC=..\..\include
@REM --------------------
@REM This shows how to build TV.bas from scratch
@REM bstc -c -Ograux -L c:\bstc\spin TV.spin
@REM %XB_BIN%\bin2xbasic TV.dat TV.bas
@REM --------------------
%XB_BIN%\xbcom -b %1 -p %2 TvDemo.bas -r %3 %4 %5 %6 %7
