/* based on some code by Steve Denson (jazzed) */

def printStr(dev, str() as byte)
    dim n = 0
    dim ch = str(n)
    do while ch
        uartTX(ch)
        n = n + 1
        ch = str(n)
    loop
end def

def printInt(dev, value)
    dim n = value
    dim length = 10
    dim result = 0
    if value < 0 then
        value = -value
        uartTX('-')
    end if
    n = 1000000000
    do while length > 0
        length = length - 1
        if value >= n then
            uartTX(value / n + '0')
            value = value mod n
            result = result + 1
        else if result or n = 1 then
            uartTX('0')
        end if
        n = n / 10
    loop
end def

def printTab(dev)
    uartTX(0x09)
end def

def printNL(dev)
    uartTX(0x0d)
    uartTX(0x0a)
end def

def uartTX(ch)
    asm
        lref 0
        trap 1
    end asm
end def
