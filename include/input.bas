include "print.bas"
include "string.bas"

def lineMax = 128

dim lineBuffer(lineMax + 1) as byte
dim linePtr = 0

def inputGetLine(dev)
    inputLine(dev, lineBuffer)
    linePtr = 0
end def

def inputInt(dev)
    dim ch = skipSpaces
    dim value = 0
    dim sign = 1
    
    // find the start of the token by skipping spaces
    // read a new line if necessary
    do while ch = NUL
        inputGetLine(dev)
        ch = skipSpaces
    loop
    
    // check for a missing input
    if ch = ',' then
        return 0
    end if
    
    // check for a minus sign
    if ch = '-' then
        sign = -1
        ch = lineBuffer(linePtr)
        linePtr = linePtr + 1
    end if
    
    // build up the integer
    do while isdigit(ch)
        value = value * 10 + ch - '0'
        linePtr = linePtr + 1
        ch = lineBuffer(linePtr)
    loop
    
    // look for the closing delimiter
    do while ch <> NUL
        linePtr = linePtr + 1
        if ch = ',' then
            return value
        end if
        ch = lineBuffer(linePtr)
    loop
    
    // return the integer
    return value
end def

def inputStr(dev, buf() as byte)
    dim ch = skipSpaces
    dim start, i
    
    // find the start of the token by skipping spaces
    // read a new line if necessary
    do while ch = NUL
        inputGetLine(dev)
        ch = skipSpaces
    loop
    
    // remember the start of the token
    start = linePtr
    i = 0
    
    // copy the token to the buffer
    do while ch <> NUL and ch <> ','
        buf(i) = ch
        linePtr = linePtr + 1
        i = i + 1
        ch = lineBuffer(linePtr)
    loop
    
    // terminate the string
    buf(i) = NUL

    // look for the closing delimiter
    do while ch <> NUL
        linePtr = linePtr + 1
        if ch = ',' then
            return i
        end if
        ch = lineBuffer(linePtr)
    loop
    
    // return the string length
    return i
end def

def skipSpaces
    dim ch = lineBuffer(linePtr)
    do while ch <> NUL and isspace(ch)
        linePtr = linePtr + 1
        ch = lineBuffer(linePtr)
    loop
    return ch
end def

def inputLine(dev, buf() as byte)
    dim i = 0
    dim ch
    do while i < lineMax
        ch = uartRX
        select ch
            case LF
                uartTX(CR)
                uartTX(LF)
                buf(i) = NUL
                return 1
            case BS, DEL
                if i > 0 then
                    uartTX(BS)
                    uartTX(' ')
                    uartTX(BS)
                    i = i - 1
                end if
            case else
                uartTX(ch)
                buf(i) = ch
                i = i + 1
        end select
    loop
    buf(i) = NUL
    return 0
end def

def uartRX
    asm
        dup
        trap 0
        returnx
    end asm
end def
