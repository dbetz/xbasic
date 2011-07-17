rem ==================================================
rem  ASCII character codes
rem ==================================================

def NUL = 0x00
def BS  = 0x08
def TAB = 0x09
def LF  = 0x0a
def CR  = 0x0d
def DEL = 0x7f

rem ==================================================
rem  return length of string
rem  @param str - string to measure
rem ==================================================

def strlen(str() as byte)
    dim n, ch
    n = 0
    ch = str(n)
    do while ch
        n = n + 1
        ch = str(n)
    loop
    return n
end def

rem ==================================================
rem  copy string from str2 to str1
rem  @param str1 - dest
rem  @param str2 - src
rem ==================================================

def strcpy(str1() as byte, str2() as byte)
  dim i = 0
  do
    str1(i) = str2(i)
    i = i + 1
  loop until str2(i) = 0
end def

rem ==================================================
rem  return whether a character is a space or tab
rem  @param ch - character to check
rem ==================================================

def isspace(ch)
    return ch = ' ' or ch = TAB
end def

rem ==================================================
rem  return whether a character is a decimal digit
rem  @param ch - character to check
rem ==================================================

def isdigit(ch)
    return ch >= '0' and ch <= '9'
end def
