REM =================================================================
REM FFT without PASM_BUTTERFLIES
REM =================================================================

REM -----------------------------------------------------------------
REM send butterfly command
REM
def FFT_butterflies (cmd, bxp, byp)
    if cmd & FFT_CMD_DECIMATE then      // Data bit-reversal reordering required?
        FFT_decimate(bxp, byp)
    end if
    if cmd & FFT_CMD_BUTTERFLY then     // FFT butterfly required?
        FFT_bfly (bxp, byp)
    end if
    if cmd & FFT_CMD_MAGNITUDE then     // Convert to magnitude required?
        FFT_magnitude (bxp, byp)
    end if
end def

REM -----------------------------------------------------------------
REM reverse support
REM @value = value to reverse
REM @count = 32 - (number of bits to reverse)
def reverse(value, count)
    asm
        LREF 0              // get value
        NATIVE 0xA0bc0805   // MOV t4, tos
        drop
        LREF 1              // get count
        NATIVE 0x3cbc0805   // REV t4, tos
        NATIVE 0xA0bc0a04   // MOV tos, t4
        returnx             // return tos value
    end asm
end def

REM -----------------------------------------------------------------
REM Radix-2 decimation in time.
REM Moves every sample of bx and by to a postion given by
REM reversing the bits of its original array index.
REM
def FFT_decimate (bxp, byp)

    dim revi, tx1, ty1
    dim one = 1     // local variables are faster than constants
    dim two = 2
    dim revsize = 32-FFT_LOG2_FFT_SIZE  // to use REV instruction
    dim i = FFT_FFT_SIZE

    do while i
        REM reverse FFT_LOG2_FFT_SIZE bits of i and zero the rest
        REM revi = i >< FFT_LOG2_FFT_SIZE
        revi = reverse(i, revsize)
        if i < revi then
            tx1 = PEEK(bxp + (i<<two))
            ty1 = PEEK(byp + (i<<two))

            POKE(bxp + (i<<two), PEEK(bxp + (revi<<two)))
            POKE(byp + (i<<two), PEEK(byp + (revi<<two)))

            POKE(bxp + (revi<<two), tx1)
            POKE(byp + (revi<<two), ty1)
        end if
        i = i - one
    loop

end def

REM -----------------------------------------------------------------
REM
def FFT_magnitude (bxp, byp)
    dim real, imag
    dim one = 1
    dim two = 2
    dim fftsize = FFT_FFT_SIZE >> 1
    dim i = 0

    do while i <= fftsize
        REM Scale down by half FFT size, back to original signal input range
        real = PEEK(bxp + (i<<two)) / fftsize
        imag = PEEK(byp + (i<<two)) / fftsize

        REM Frequency magnitude is square root of cos part sqaured plus sin part squared
        POKE(bxp + (i<<two), Math_sqrt(((real * real) + (imag * imag))))
        i = i + one
    loop
end def

REM -----------------------------------------------------------------
REM Apply FFT butterflies to N complex samples in x, in time decimated order !
REM Resulting FFT is in x in the correct order.
REM
def FFT_bfly (bxp, byp)

    dim one = 1
    dim four = 4
    dim twelve = 12

    dim level

    dim b0x_ptr
    dim b0y_ptr
    dim b1x_ptr
    dim b1y_ptr
    dim wx_ptr
    dim wy_ptr

    dim a, b, c, d
    dim k1, k2, k3
    dim tx, ty

    dim log2fftsize = FFT_LOG2_FFT_SIZE

    dim flight_max = FFT_FFT_SIZE >> 1      // Initial number of flights in a level
    dim wSkip = FFT_FFT_SIZE << 1           // But we advance w pointer by 4 for long w's
    dim butterflySpan = 4                   // Span measured in bytes
    dim butterfly_max = 1                   // 1 butterfly per flight initially
    dim flightSkip = 4                      // But we advance pointer by 4 bytes per butterfly

    dim n = 0
    dim k = 0
    dim m = log2fftsize

    REM Loop through all the decimation levels

    do while m
        m = m - one

        b0x_ptr = bxp
        b0y_ptr = byp

        b1x_ptr = b0x_ptr + butterflySpan
        b1y_ptr = b0y_ptr + butterflySpan

        REM Loop though all the flights in a level

        n = flight_max
        do while n
            n = n - one

            wx_ptr = @wx
            wy_ptr = @wy

            REM Loop through all the butterflies in a flight
            REM
            k = butterfly_max
            do while k 
                k = k - one

                REM print "flight ", log2fftsize;":";m, flight_max;":";n, butterfly_max;":";k,

                REM At last...the butterfly.
                REM ----------------------
                a = PEEK(b1x_ptr)                 //Get X[b1]
                b = PEEK(b1y_ptr)

                REM c = Math_SignExW(rdword(wx_ptr))//Get W[wIndex]
                REM d = Math_SignExW(rdword(wy_ptr))
                c = PEEK(wx_ptr)                  //Get W[wIndex]
                d = PEEK(wy_ptr)                  // all data entered is long

                REM print a; " "; b; " "; c; " "; d,

                k1 = Math_SAR(a * (c + d),twelve)   //Somewhat optimized complex multiply
                k2 = Math_SAR(d * (a + b),twelve)   //   T = X[b1] * W[wIndex]
                k3 = Math_SAR(c * (b - a),twelve) 
                tx = k1 - k2
                ty = k1 + k3

                k1 = PEEK(b0x_ptr)        //bx[b0]
                k2 = PEEK(b0y_ptr)        //by[b0]

                REM print k1; "."; k2; "."; tx; "."; ty, 
                REM print k1 - tx; " "; k2 - ty; " "; k1 + tx; " "; k2 + ty

                POKE(b1x_ptr, k1 - tx)    //X[b1] = X[b0] - T
                POKE(b1y_ptr, k2 - ty)

                POKE(b0x_ptr, k1 + tx)    //X[b0] = X[b0] + T
                POKE(b0y_ptr, k2 + ty)
                REM ---------------------

                b0x_ptr = b0x_ptr + four    //Advance to next butterfly in flight,
                b0y_ptr = b0y_ptr + four    //skiping 4 bytes for each.

                b1x_ptr = b1x_ptr + four
                b1y_ptr = b1y_ptr + four

                wx_ptr = wx_ptr + wSkip     //Advance to next w
                wy_ptr = wy_ptr + wSkip

            REM end do while k
            loop

            b0x_ptr = b0x_ptr + flightSkip  //Advance to first butterfly of next flight
            b0y_ptr = b0y_ptr + flightSkip
            b1x_ptr = b1x_ptr + flightSkip
            b1y_ptr = b1y_ptr + flightSkip

        REM end do while n
        loop

        butterflySpan = butterflySpan << one    // On the next level butterflies are twice as wide
        flightSkip = flightSkip << one          // and so is the flight skip
        flight_max = flight_max >> one          // On the next level there are half as many flights
        wSkip = wSkip >> one                    // And w's are half as far apart
        butterfly_max = butterfly_max << one    // On the next level there are twice the butterflies per flight

    REM end do while m
    loop

end def

REM The W "twiddle factor" tables.
REM These are one half cycle of cos and minus sine respectively scaled up to +/- 4096
REM
dim wx() as integer = {
    4096,4095,4095,4095,4094,4094,4093,4092,4091,4089,4088,4086,4084,4082,4080,4078
    4076,4073,4071,4068,4065,4062,4058,4055,4051,4047,4043,4039,4035,4031,4026,4022
    4017,4012,4007,4001,3996,3990,3985,3979,3973,3967,3960,3954,3947,3940,3933,3926
    3919,3912,3904,3897,3889,3881,3873,3864,3856,3848,3839,3830,3821,3812,3803,3793
    3784,3774,3764,3754,3744,3734,3723,3713,3702,3691,3680,3669,3658,3647,3635,3624
    3612,3600,3588,3576,3563,3551,3538,3526,3513,3500,3487,3473,3460,3447,3433,3419
    3405,3391,3377,3363,3348,3334,3319,3304,3289,3274,3259,3244,3229,3213,3197,3182
    3166,3150,3134,3117,3101,3085,3068,3051,3034,3018,3000,2983,2966,2949,2931,2914
    2896,2878,2860,2842,2824,2806,2787,2769,2750,2732,2713,2694,2675,2656,2637,2617
    2598,2578,2559,2539,2519,2500,2480,2460,2439,2419,2399,2379,2358,2337,2317,2296
    2275,2254,2233,2212,2191,2170,2148,2127,2105,2084,2062,2040,2018,1997,1975,1952
    1930,1908,1886,1864,1841,1819,1796,1773,1751,1728,1705,1682,1659,1636,1613,1590
    1567,1544,1520,1497,1474,1450,1427,1403,1379,1356,1332,1308,1284,1260,1237,1213
    1189,1164,1140,1116,1092,1068,1043,1019, 995, 970, 946, 921, 897, 872, 848, 823
    799,  774, 749, 725, 700, 675, 650, 625, 601, 576, 551, 526, 501, 476, 451, 426
    401,  376, 351, 326, 301, 276, 251, 226, 200, 175, 150, 125, 100,  75,  50,  25
}

dim wy() as integer = {
        0,  -25,  -50,  -75, -100, -125, -150, -175, -200, -226, -251, -276, -301, -326, -351, -376
    -401 , -426, -451, -476, -501, -526, -551, -576, -601, -625, -650, -675, -700, -725, -749, -774
    -799 , -823, -848, -872, -897, -921, -946, -970, -995,-1019,-1043,-1068,-1092,-1116,-1140,-1164
    -1189,-1213,-1237,-1260,-1284,-1308,-1332,-1356,-1379,-1403,-1427,-1450,-1474,-1497,-1520,-1544
    -1567,-1590,-1613,-1636,-1659,-1682,-1705,-1728,-1751,-1773,-1796,-1819,-1841,-1864,-1886,-1908
    -1930,-1952,-1975,-1997,-2018,-2040,-2062,-2084,-2105,-2127,-2148,-2170,-2191,-2212,-2233,-2254
    -2275,-2296,-2317,-2337,-2358,-2379,-2399,-2419,-2439,-2460,-2480,-2500,-2519,-2539,-2559,-2578
    -2598,-2617,-2637,-2656,-2675,-2694,-2713,-2732,-2750,-2769,-2787,-2806,-2824,-2842,-2860,-2878
    -2896,-2914,-2931,-2949,-2966,-2983,-3000,-3018,-3034,-3051,-3068,-3085,-3101,-3117,-3134,-3150
    -3166,-3182,-3197,-3213,-3229,-3244,-3259,-3274,-3289,-3304,-3319,-3334,-3348,-3363,-3377,-3391
    -3405,-3419,-3433,-3447,-3460,-3473,-3487,-3500,-3513,-3526,-3538,-3551,-3563,-3576,-3588,-3600
    -3612,-3624,-3635,-3647,-3658,-3669,-3680,-3691,-3702,-3713,-3723,-3734,-3744,-3754,-3764,-3774
    -3784,-3793,-3803,-3812,-3821,-3830,-3839,-3848,-3856,-3864,-3873,-3881,-3889,-3897,-3904,-3912
    -3919,-3926,-3933,-3940,-3947,-3954,-3960,-3967,-3973,-3979,-3985,-3990,-3996,-4001,-4007,-4012
    -4017,-4022,-4026,-4031,-4035,-4039,-4043,-4047,-4051,-4055,-4058,-4062,-4065,-4068,-4071,-4073
    -4076,-4078,-4080,-4082,-4084,-4086,-4088,-4089,-4091,-4092,-4093,-4094,-4094,-4095,-4095,-4095
    -4096,-4095,-4095,-4095,-4094,-4094,-4093,-4092,-4091,-4089,-4088,-4086,-4084,-4082,-4080,-4078
    -4076,-4073,-4071,-4068,-4065,-4062,-4058,-4055,-4051,-4047,-4043,-4039,-4035,-4031,-4026,-4022
    -4017,-4012,-4007,-4001,-3996,-3990,-3985,-3979,-3973,-3967,-3960,-3954,-3947,-3940,-3933,-3926
    -3919,-3912,-3904,-3897,-3889,-3881,-3873,-3864,-3856,-3848,-3839,-3830,-3821,-3812,-3803,-3793
    -3784,-3774,-3764,-3754,-3744,-3734,-3723,-3713,-3702,-3691,-3680,-3669,-3658,-3647,-3635,-3624
    -3612,-3600,-3588,-3576,-3563,-3551,-3538,-3526,-3513,-3500,-3487,-3473,-3460,-3447,-3433,-3419
    -3405,-3391,-3377,-3363,-3348,-3334,-3319,-3304,-3289,-3274,-3259,-3244,-3229,-3213,-3197,-3182
    -3166,-3150,-3134,-3117,-3101,-3085,-3068,-3051,-3034,-3018,-3000,-2983,-2966,-2949,-2931,-2914
    -2896,-2878,-2860,-2842,-2824,-2806,-2787,-2769,-2750,-2732,-2713,-2694,-2675,-2656,-2637,-2617
    -2598,-2578,-2559,-2539,-2519,-2500,-2480,-2460,-2439,-2419,-2399,-2379,-2358,-2337,-2317,-2296
    -2275,-2254,-2233,-2212,-2191,-2170,-2148,-2127,-2105,-2084,-2062,-2040,-2018,-1997,-1975,-1952
    -1930,-1908,-1886,-1864,-1841,-1819,-1796,-1773,-1751,-1728,-1705,-1682,-1659,-1636,-1613,-1590
    -1567,-1544,-1520,-1497,-1474,-1450,-1427,-1403,-1379,-1356,-1332,-1308,-1284,-1260,-1237,-1213
    -1189,-1164,-1140,-1116,-1092,-1068,-1043,-1019, -995, -970, -946, -921, -897, -872, -848, -823
     -799, -774, -749, -725, -700, -675, -650, -625, -601, -576, -551, -526, -501, -476, -451, -426
     -401, -376, -351, -326, -301, -276, -251, -226, -200, -175, -150, -125, -100,  -75,  -50,  -25
}

REM end of file
REM =================================================================






