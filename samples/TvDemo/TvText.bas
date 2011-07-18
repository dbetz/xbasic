REM ==================================================
REM TvText.bas
REM This module is the TvText driver.
REM It includes the TvText PASM and access methods.
REM ==================================================

include "TV.bas"

REM ==================================================
REM TvText driver constants
REM ==================================================

def TvText_OUTCLS       = 0x0
def TvText_OUTHOME      = 0x1
def TvText_OUTBKSP      = 0x8
def TvText_OUTTAB       = 0x9
def TvText_OUTX         = 0xA
def TvText_OUTY         = 0xB
def TvText_OUTCOLOR     = 0xC

def TvText_rows         = 13
def TvText_cols         = 42                 // text columns
def TvText_x2cols       = TvText_cols<<1     // 2x bytes for display
def TvText_screensize   = TvText_x2cols*TvText_rows
def TvText_lastrow      = TvText_screensize - TvText_x2cols

def TvText_WHITE_BLACK  = 0
def TvText_WHITE_RED    = 1
def TvText_YELLOW_BROWN = 2
def TvText_GREY_WHITE   = 3
def TvText_CYAN_TEAL    = 4
def TvText_GREEN_LIME   = 5
def TvText_RED_PINK     = 6
def TvText_CYAN_NAVY    = 7

REM
REM Modified TvText color palette
REM
REM Color index 0 sets the un-used background color
REM
dim TvText_palette() as byte = {
    0x07,   0x03        // 0    white / black
    0x07,   0xBB        // 1    white / red
    0x9E,   0x9B        // 2   yellow / brown
    0x04,   0x07        // 3     grey / white
    0x3D,   0x4C        // 4     cyan / teal
    0x6B,   0x6E        // 5    green / lime
    0xBB,   0xCE        // 6      red / pink
    0x3D,   0x39        // 7     cyan / navy
}

/*
REM ORIGINAL TvText color palette
dim TvText_palette() as byte = {
    0x07,   0x0A        // 0    white / dark blue
    0x07,   0xBB        // 1    white / red
    0x9E,   0x9B        // 2   yellow / brown
    0x04,   0x07        // 3     grey / white
    0x3D,   0x3B        // 4     cyan / dark cyan
    0x6B,   0x6E        // 5    green / gray-green
    0xBB,   0xCE        // 6      red / pink
    0x3C,   0x0A        // 7     cyan / blue
}
*/

REM ==================================================
REM
REM Changing a TV palette color
REM
REM Use TvText_setpalette_color(index, foreground, background).
REM Once the indexed palette is changed, you can use it.
REM TV Colors can be difficult to understand with words.
REM A color graph can be found at www.MicroCSource.com/TvColors.png
REM
REM Generally, the colors are controlled with 2 nibbles:
REM The most significant byte MSB nibble controls the color.
REM The least significant byte LSB nibble controls the shade.
REM
REM A byte having LSB nibble 0x08 is a super-bright color and
REM will be totally different from a byte containing 0x0A or 0x07.
REM Bright colors: GREEN=0x08, BROWN=0x18, ORANGE=0x38, RED=0x48
REM HOTPINK=0x58,VIOLET=0x68, PURPLE=0x78, PURPLEBLUE=0x88
REM BRIGHTBLUE=0x98, DODGERBLUE=0xA8, GUNMETALBLUE=0xB8
REM Various BRIGHTGREENs=0xC8,0xD8,0xE8,0xF8
REM
REM Color shades 0x0A, 0x0B, 0x0C, 0x0D, and 0x0E will have MSB colors
REM from BLUE, to GREEN, to YELLOW/BROWN, to RED, to PURPLE.
REM for example shades of BLUE are 0x1A, 0x1B, 0x1C, 0x1D, 0x1E
REM where 0x1E is the lightest color but will have a tinge of PURPLE.
REM BABY BLUE is 0x2E.
REM
REM Propeller supports 6 shades of GRAY: BLACK=0x02 (don't use 0 or 1)
REM DARKGRAY=0x03, GRAY=0x04, LIGHTGRAY=0x05, WHITEGRAY=0x06, WHITE=0x07
REM
REM ==================================================

REM ==================================================
REM TvText driver global variables
REM ==================================================

dim TvText_cog          = 0

dim TvText_color        = 0
dim TvText_row          = 0
dim TvText_col          = 0
dim TvText_contrast     = 0
dim TvText_flag         = 0

REM need screensize words - xbasic only does byte and 32 bit integer
dim TvText_screen (TvText_screensize<<1) as byte    // need 2x bytes
dim TvText_colors (16)

dim TvText_TV_status    = 0
dim TvText_TV_enable    = 1
dim TvText_TV_pins      = 0
dim TvText_TV_mode      = 0x12
dim TvText_TV_screen    = 0
dim TvText_TV_colors    = 0
dim TvText_TV_ht        = TvText_cols
dim TvText_TV_vt        = TvText_rows
dim TvText_TV_hx        = 4
dim TvText_TV_vx        = 1
dim TvText_TV_ho        = 0
dim TvText_TV_vo        = 0
dim TvText_TV_brdcast   = 0
dim TvText_TV_aural     = 0

REM =================================================
REM start up the TV
REM =================================================

def TvText_start(basepin)
    dim group = (basepin & 0x38) << 1 
    if (basepin & 4) then
        group = group | 5
    end if

    // setup the palette
    TvText_setpalette(TvText_palette)

    // set unassigned pins for driver
    TvText_tv_pins = group
    TvText_tv_screen = hubaddr(@TvText_screen)
    TvText_tv_colors = hubaddr(@TvText_colors)
    TvText_cog = cognew(@TV_array,@TvText_TV_status)+1

    // wait for COG to start
    waitcnt(clkfreq/5+cnt)

    // clear screen and go home
    TvText_out(0)
end def

def TvText_stop
    if TvText_cog > 0 then
        cogstop(TvText_cog)
        TvText_cog = 0
    end if
end def

REM===================================================
REM set colors to palette
REM
def TvText_setpalette(colorptr() as byte)
    dim i, fg, bg
    for i = 0 to 7
        fg = colorptr((i << 1))
        bg = colorptr((i << 1) + 1)
        TvText_colors((i << 1))       = (fg << 24) + (bg << 16) + (fg << 8) + bg
        TvText_colors((i << 1) + 1)   = (fg << 24) + (fg << 16) + (bg << 8) + bg
    next i
end def

REM===================================================
REM set colors to palette
REM @index - palette color index. 
REM if index < 0 or index > 7 then becomes index & 7
REM @fg - palette foreground color
REM @bg - palette background color
REM
def TvText_setpalette_color(index,fg,bg)
    dim i = index & 7
    TvText_colors((i << 1))       = (fg << 24) + (bg << 16) + (fg << 8) + bg
    TvText_colors((i << 1) + 1)   = (fg << 24) + (fg << 16) + (bg << 8) + bg
end def

def TvText_bcopy(dst() as byte, src() as byte, sndx, length)
    dim n = 0
    do while n < length
        dst(n) = src(n+sndx)
        n = n + 1
    loop
end def

REM===================================================
REM print newline - scroll screen if necessary
REM
def TvText_newline
    dim x, y, n, value
    REM need lastrow to be 2xbytes
    dim lastrow = TvText_lastrow<<1
    dim clrsize = TvText_screensize>>1
    TvText_col = 0
    TvText_row = TvText_row + 1

    if TvText_row = TvText_rows then
        TvText_row = TvText_row - 1 
        REM copy size-c bytes from s+c to s
        TvText_bcopy(TvText_screen, TvText_screen, TvText_x2cols, lastrow)

        REM clear the new last line
        n = lastrow>>1
        do while n < clrsize
            value = TvText_char(' ')
            //TvText_screen((n<<1)+0) = value & 0xff
            //TvText_screen((n<<1)+1) = value >> 8
            POKEW(@TvText_screen+(n<<1), value)
            n = n + 1
        loop

        TvText_col = 0
    end if
end def

def TvText_char(c)
    dim val = ((TvText_color << 1) | (c & 1)) << 10
    val = val + 0x200 + (c & 0xFE)
    return val
end def

REM===================================================
REM clear screen to current background color
REM
def TvText_cls
    dim length  = TvText_screensize
    dim value   = TvText_char(' ')
    dim mask    = 0xff
    dim blank   = 0x220
    dim n = 0

    TvText_row = 0
    TvText_col = 0
    do while n < length    // do while is faster than for loop
        //TvText_screen((n<<1)+0) = value & mask
        //TvText_screen((n<<1)+1) = value >> 8
        POKEW(@TvText_screen+(n<<1), blank)
        n = n + 1
    loop
end def

REM===================================================
REM print character to current position
REM
def TvText_printc(c)
    dim cols    = TvText_x2cols
    dim dcols   = TvText_x2cols>>1
    dim mask    = 0xff
    dim value   = TvText_char(c)
    //TvText_screen(TvText_row * cols + (TvText_col<<1))   = value & mask
    //TvText_screen(TvText_row * cols + (TvText_col<<1)+1) = value >> 8
    POKEW(@TvText_screen+(TvText_row * cols + (TvText_col<<1)), value)
    TvText_col = TvText_col + 1
    if TvText_col = dcols then
        TvText_newline
    end if
end def

REM===================================================
REM print character to screen at current position
REM @param c - character to print
REM
def TvText_putchar(c)
    dim x, y
    dim newline = 0xd
    if c = newline then
        TvText_newline
    else
        TvText_printc(c)
    end if
end def

REM===================================================
REM set character x,y position
REM @param mx - x or column position
REM @param my - y or row position
REM
def TvText_setTextXY(mx,my)
    TvText_col = mx mod TvText_x2cols
    TvText_row = my mod TvText_rows
end def

REM===================================================
REM get current x or column text position
REM
def TvText_getx
    return TvText_col
end def

REM===================================================
REM get current y or row text position
REM
def TvText_gety
    return TvText_row
end def

REM===================================================
REM set current x or column text position
REM
def TvText_setx(x)
    TvText_col = x
end def

REM===================================================
REM set current y or row text position
REM
def TvText_sety(y)
    TvText_row = y
end def

REM===================================================
REM set color ... faster than out
REM @param c - color
REM
def TvText_setcolor(c)
    TvText_color = c
end def

REM===================================================
REM get current color
REM
def TvText_getcolor
    return TvText_color
end def
  
REM===================================================
REM print string to current x,y position
REM @param s - string of chars TvText_print("Hello")
REM
def TvText_print(s() as byte)
    dim n = 0
    dim ch = s(n)
    do while ch
        if(ch = 0xd) then
            newline
        else
            TvText_putchar(ch)
        end if
        n = n + 1
        ch = s(n)
    loop
end def

REM===================================================
REM print zero terminated text to x,y position
REM @param mx - x or column position
REM @param my - y or row position
REM @param s  - string of chars TvText_printXY("Hello",5,10)
REM
def TvText_printXY(mx,my, s() as byte)
    dim n, ch
    TvText_col = mx mod TvText_x2cols
    TvText_row = my mod TvText_rows
    n = 0
    ch = s(n)
    do while ch
        TvText_putchar(ch)
        n = n + 1
        ch = s(n)
    loop
end def

REM===================================================
REM Print a zero-terminated string to current text position
REM @param s  - string of chars TvText_str("Hello")
REM
def TvText_str(s() as byte)
    dim n = 0
    dim ch = s(n)
    do while ch
        TvText_putchar(ch)
        n = n + 1
        ch = s(n)
    loop
end def

REM===================================================
REM Print a decimal number to current text position
REM @param value - decimal number
REM
def TvText_dec(value)
    dim n = value
    dim length = 10
    dim result = 0

    if value < 0 then
        value = 0-value
        TvText_putchar('-')
    end if

    n = 1000000000

    do while length > 0
        length = length - 1
        if value >= n then
            TvText_putchar(value / n + '0')
            value = value mod n
            result = result + 1
        else if result then
            TvText_putchar('0')
        else if n = 1 then
            TvText_putchar('0')
        end if
        n = n / 10
    loop

end def

dim TvText_hexarray(16) = {
    '0', '1', '2', '3', '4', '5', '6', '7'
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
}

REM===================================================
REM Print a hexidecimal number to current text position
REM @param value - decimal number
REM @param digits - number of digits to print
REM
def TvText_hex(value, digits)
    dim n = 0
    dim mask    = 0xf   // faster than using constants
    dim one     = 1     // faster than using constants
    do while digits > 0
        digits = digits - one
        n = (value >> (digits<<2)) & mask
        TvText_putchar(TvText_hexarray(n))
    loop
end def

REM ===================================================
REM  Print a binary number to current text position
REM  @param value - number
REM  @param digits - number of digits to print
REM 
def TvText_bin(value, digits)
    dim one = 1         // faster than using constants
    dim minus1 = -1     // faster than using constants
    dim dig = 0
    n = digits-1
    do while n > minus1
        dig = '0' + ( (value & (1<<n)) >> n)
        TvText_out(dig)
        n = n - one
    loop
end def


REM===================================================
REM Output a character
REM
REM @param c - interpreted as follows - see TvText_OUTxxx
REM
REM  0x00 = clear screen
REM  0x01 = home
REM  0x08 = backspace
REM  0x09 = tab (8 spaces per)
REM  0x0A = set X position (X follows)
REM  0x0B = set Y position (Y follows)
REM  0x0C = set color (color follows)
REM  0x0D = return
REM  others = printable characters
REM
def TvText_out(c)
    dim zero = 0
    dim one  = 1
    dim seven = 7

    if TvText_flag = zero then

        REM  0x00 = clear screen
        if c = zero then
            TvText_cls

        REM  0x01 = home
        else if c = one then
            TvText_col = zero
            TvText_row = zero

        REM  0x08 = backspace
        else if c = 8 then
            if TvText_col then
                TvText_col = TvText_col - one
            end if

        REM  0x09 = tab (8 spaces per)
        else if c = 9 then
            do while TvText_col & seven
                  TvText_putchar(' ')
            loop

        REM  0x0A = set X position (X follows)
        else if c = 0xa then
            TvText_flag = c
            return

        REM  0x0B = set Y position (Y follows)
        else if c = 0xb then
            TvText_flag = c
            return

        REM  0x0C = set color (color follows)
        else if c = 0xc then
            TvText_flag = c
            return

        REM  0x0D = return ... putchar handles it
        REM  others = printable characters
        else
            TvText_putchar(c)
        end if

    REM  0x0A = set X position (X follows)
    else if TvText_flag = 0xA then
        TvText_col = c mod TvText_cols

    REM  0x0B = set Y position (Y follows)
    else if TvText_flag = 0xB then
        TvText_row = c mod TvText_rows

    REM  0x0C = set color (color follows)
    else if TvText_flag = 0xC then
        TvText_color = c
    end if
    TvText_flag = zero
end def

