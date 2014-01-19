def clkfreq
  asm
    dup                // make space for the return value
    native rdlong tos, #0
    returnx
  end asm
end def

def cogid
  asm
    dup                // make space for the return value
    native cogid tos
    returnx
  end asm
end def

def cognew(code, par)
  return coginit(0x8, code, par)
end def

def coginit(id, code, par)
  asm
    lref 0              // get id
    lit 0xf             // bitwise and low order 4 bits
    band
    lref 1              // get code
    native add tos, base
    lit 0xfffc          // bitwise and to 14 bits
    band
    lit 2               // shift left 2
    shl
    bor                 // bitwise or with id and par
    lref 2              // get par
    native add tos, base
    lit 0xfffc          // bitwise and to 14 bits
    band
    lit 16              // shift left 16
    shl
    bor                 // bitwise or with id
    native coginit tos wr
    returnx
  end asm
end def

def cogstop(id)
  asm
    lref 0
    native cogstop tos wr
    returnx
  end asm
end def

def locknew
  asm
    dup                 // make space for the return value
    native locknew tos wc
    native if_c mov tos, #1
    native if_c neg tos, tos
    returnx
  end asm
end def

def lockret(id)
  asm
    lref 0
    native locknew tos
    returnx
  end asm
end def

def lockset(id)
  asm
    lref 0
    native lockset tos wc
    native mov tos, #0
    native muxc tos, #1
    returnx
  end asm
end def

def lockclr(id)
  asm
    lref 0
    native lockclr tos wc
    native mov tos, #0
    native muxc tos, #1
    returnx
  end asm
end def

def waitcnt(n)
  asm
    lref 0
    native waitcnt tos, #0
    returnx
  end asm
end def

def waitpeq(state, mask)
  asm
    lref 1                      // get mask
    native mov t1, tos
    drop
    lref 0                      // get state
    native waitpeq tos, t1 wr
    returnx
  end asm
end def

def waitpne(state, mask)
  asm
    lref 1              // get mask
    native mov t1, tos
    lref 0              // get state
    native waitpne tos, t1 wr
    returnx
  end asm
end def

def hubaddr(addr)
  asm
    lref 0
    native add tos, base
    returnx
  end asm
end def

REM call as: value = PEEKB(ptr)
def PEEKB(addr)
    asm
        lref 0              // get value tos
        native add tos, base
        native rdbyte tos, tos
        returnx
    end asm
end def

REM call as: POKEB(ptr, value)
def POKEB(addr,value)
    asm
        lref 0              // get address tos
        native add tos, base
        native mov t4, tos
        drop
        lref 1              // get value tos
        native wrbyte tos, t4
        returnx             // need drop or returnx
    end asm
end def

REM call as: value = PEEKW(ptr)
def PEEKW(addr)
   asm
       lref 0              // get value tos
       native add tos, base
       native rdword tos, tos
       returnx
   end asm
end def

REM call as: POKEW(ptr, value)
def POKEW(addr,value)
   asm
       lref 0              // get address tos
       native add tos, base
       native mov t4, tos
       drop
       lref 1              // get value tos
       native wrword tos, t4
       returnx             // need drop or returnx
   end asm
end def

REM call as: value = PEEK(ptr)
def PEEK(addr)
    asm
        lref 0              // get value tos
        native add tos, base
        native rdlong tos, tos
        returnx
    end asm
end def

REM call as: POKE(ptr, value)
def POKE(addr,value)
    asm
        lref 0              // get address tos
        native add tos, base
        native mov t4, tos
        drop
        lref 1              // get value tos
        native wrlong tos, t4
        returnx             // need drop or returnx
    end asm
end def

