def clkfreq
  asm
    dup                // make space for the return value
    native 0x08fc0a00  // rdlong tos, #0
    returnx
  end asm
end def

def cogid
  asm
    dup                // make space for the return value
    native 0x0cfc0a01  // cogid tos
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
    native 0x80bc0a06   // add tos, base
    lit 0xfffc          // bitwise and to 14 bits
    band
    lit 2               // shift left 2
    shl
    bor                 // bitwise or with id and par
    lref 2              // get par
    native 0x80bc0a06   // add tos, base
    lit 0xfffc          // bitwise and to 14 bits
    band
    lit 16              // shift left 16
    shl
    bor                 // bitwise or with id
    native 0x0cfc0a02   // coginit tos wr
    returnx
  end asm
end def

def cogstop(id)
  asm
    lref 0
    native 0x0cfc0a03   // cogstop tos wr
    returnx
  end asm
end def

def locknew
  asm
    dup                 // make space for the return value
    native 0x0dfc0a04   // locknew tos wc
    native 0xa0f00a01   // if_c mov tos, #1
    native 0xa4b00a05   // if_c neg tos, tos
    returnx
  end asm
end def

def lockret(id)
  asm
    lref 0
    native 0x0cfc0a04   // locknew tos
    returnx
  end asm
end def

def lockset(id)
  asm
    lref 0
    native 0x0d7c0a06   // lockset tos wc
    native 0xa0fc0a01   // mov tos, #0
    native 0x70fc0a01   // muxc tos, #1
    returnx
  end asm
end def

def lockclr(id)
  asm
    lref 0
    native 0x0d7c0a07   // lockclr tos wc
    native 0xa0fc0a01   // mov tos, #0
    native 0x70fc0a01   // muxc tos, #1
    returnx
  end asm
end def

def waitcnt(n)
  asm
    lref 0
    native 0xf87c0a00   // waitcnt tos, #0
    returnx
  end asm
end def

def waitpeq(state, mask)
  asm
    lref 1              // get mask
    native 0xa0bc0205   // mov t1, tos
    lref 0              // get state
    native 0xf0bc0a01   // waitpeq tos, t1 wr
    returnx
  end asm
end def

def waitpne(state, mask)
  asm
    lref 1              // get mask
    native 0xa0bc0205   // mov t1, tos
    lref 0              // get state
    native 0xf4bc0a01   // waitpne tos, t1 wr
    returnx
  end asm
end def

def hubaddr(addr)
  asm
    lref 0
    native 0x80bc0a06   // add tos, base
    returnx
  end asm
end def

REM call as: value = PEEKB(ptr)
def PEEKB(addr)
    asm
        lref 0              // get value tos
        native 0x80bc0a06   // add tos, base
        native 0x00bc0a05   // rdbyte tos, tos
        returnx
    end asm
end def

REM call as: POKEB(ptr, value)
def POKEB(addr,value)
    asm
        lref 0              // get address tos
        native 0x80bc0a06   // add tos, base
        native 0xa0bc0805   // mov t4, tos
        drop
        lref 1              // get value tos
        native 0x003c0a04   // wrbyte tos, t4
        returnx             // need drop or returnx
    end asm
end def

REM call as: value = PEEKW(ptr)
def PEEKW(addr)
   asm
       lref 0              // get value tos
       native 0x80bc0a06   // add tos, base
       native 0x04bc0a05   // rdword tos, tos
       returnx
   end asm
end def

REM call as: POKEW(ptr, value)
def POKEW(addr,value)
   asm
       lref 0              // get address tos
       native 0x80bc0a06   // add tos, base
       native 0xa0bc0805   // mov t4, tos
       drop
       lref 1              // get value tos
       native 0x043c0a04   // wrword tos, t4
       returnx             // need drop or returnx
   end asm
end def

REM call as: value = PEEK(ptr)
def PEEK(addr)
    asm
        lref 0              // get value tos
        native 0x80bc0a06   // add tos, base
        native 0x08bc0a05   // rdlong tos, tos
        returnx
    end asm
end def

REM call as: POKE(ptr, value)
def POKE(addr,value)
    asm
        lref 0              // get address tos
        native 0x80bc0a06   // add tos, base
        native 0xa0bc0805   // mov t4, tos
        drop
        lref 1              // get value tos
        native 0x083c0a04   // wrlong tos, t4
        returnx             // need drop or returnx
    end asm
end def

