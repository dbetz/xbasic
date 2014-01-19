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
        NATIVE MOV t4, tos
        drop
        LREF 1              // get shift
        NATIVE SAR t4, tos
        NATIVE MOV tos, t4
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

REM===================================================
REM return a positive random number
REM
def RND
    rnd_lfsr = (rnd_lfsr >> 1) ^ (-(rnd_lfsr & 1) & rnd_poly)
    return rnd_lfsr & 0x7fffffff
end def

dim rnd_lfsr        = 0x55555555
dim rnd_poly        = 0x20000051 // fair alternative
REM dim rnd_poly    = 0xD0000001

REM===================================================
REM seed the PRNG generator
REM
def SEED(num)
    rnd_lfsr = num
end def
