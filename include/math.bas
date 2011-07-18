REM =================================================================
REM xBasic Math library
REM =================================================================

REM -----------------------------------------------------------------
REM return square root of an integer
REM
def Math_SQRT(value)
    dim result = 0
    dim zero = 0
    dim one = 1
    dim two = 2
    dim bit = 1 << 30

    REM "bit" starts at the highest power of four <= the argument.
    do while bit > value
        bit = bit >> two
    loop

    do while bit <> zero
        if value >= result + bit then
            value = value - (result + bit)
            result = (result >> one) + bit
        else
            result = result >> one
        end if
        bit = bit >> two
    loop
    return result
end def

REM -----------------------------------------------------------------
REM arithmetic shift right
REM
def Math_SAR(value, shift)
    asm
        LREF 0              // get value
        NATIVE 0xA0bc0805   // MOV t4, tos
        drop
        LREF 1              // get shift
        NATIVE 0x38bc0805   // SAR t4, tos
        NATIVE 0xA0bc0a04   // MOV tos, t4
        returnx             // return tos value
    end asm
end def

REM -----------------------------------------------------------------
REM sign extend a short word
REM
def Math_SignExW(value)
    dim msbw = 0x8000
    dim negw = 0xFFFF8000
    if value & msbw then
        value = value | negw
    end if
    return value
end def

