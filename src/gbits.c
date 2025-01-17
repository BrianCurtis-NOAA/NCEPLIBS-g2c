/** @file
 * @brief Functions to pack and unpack bits to/from a packed bit
 * string.
 * @author NOAA Programmer
 */
#include "grib2_int.h"

/**
 * Get bits - unpack bits: Extract arbitrary size values from a packed
 * bit string, right justifying each value in the unpacked iout array.
 *
 * @param in pointer to character array input.
 * @param iout pointer to unpacked array output.
 * @param iskip initial number of bits to skip.
 * @param nbits number of bits to take.
 *
 * @author NOAA Programmer
 */
void
gbit(unsigned char *in, g2int *iout, g2int iskip, g2int nbits)
{
    gbits(in, iout, iskip, nbits, (g2int)0, (g2int)1);
}

/**
 * Store bits - put arbitrary size values into a packed bit string,
 * taking the low order bits from each value in the unpacked array.
 *
 * @param out Pointer to packed array output. Must be allocated large
 * enough to hold output.
 * @param in Pointer to unpacked array input.
 * @param iskip Initial number of bits to skip.
 * @param nbits Number of bits to pack.
 *
 * @author NOAA Programmer
 */
void
sbit(unsigned char *out, g2int *in, g2int iskip, g2int nbits)
{
    sbits(out, in, iskip, nbits, (g2int)0, (g2int)1);
}

/**
 * Get bits - unpack bits: Extract arbitrary size values from a packed
 * bit string, right justifying each value in the unpacked iout array.
 *
 * @param in Pointer to character array input.
 * @param iout Pointer to unpacked array output.
 * @param iskip Initial number of bits to skip.
 * @param nbits Number of bits to take.
 * @param nskip Additional number of bits to skip on each iteration.
 * @param n Number of iterations.
 *
 * @author NOAA Programmer
 */
void
gbits(unsigned char *in, g2int *iout, g2int iskip, g2int nbits,
      g2int nskip, g2int n)
{
    g2int i, tbit, bitcnt, ibit, itmp;
    g2int nbit, index;
    static g2int ones[]={1, 3, 7, 15, 31, 63, 127, 255};

    /*     nbit is the start position of the field in bits */
    nbit = iskip;
    for (i = 0; i < n; i++)
    {
        bitcnt = nbits;
        index = nbit / 8;
        ibit = nbit % 8;
        nbit = nbit + nbits + nskip;

        /*        first byte */
        tbit = (bitcnt < (8 - ibit)) ? bitcnt : 8 - ibit;  // find min
        itmp = (int)*(in + index) & ones[7 - ibit];
        if (tbit != 8 - ibit) itmp >>= (8 - ibit - tbit);
        index++;
        bitcnt = bitcnt - tbit;

        /*        now transfer whole bytes */
        while (bitcnt >= 8)
        {
            itmp = itmp << 8 | (int)*(in + index);
            bitcnt = bitcnt - 8;
            index++;
        }

        /* get data from last byte */
        if (bitcnt > 0)
        {
            itmp = ( itmp << bitcnt ) |
                (((int)*(in + index) >> (8 - bitcnt)) & ones[bitcnt - 1]);
        }

        *(iout + i) = itmp;
    }
}

/**
 * Store bits - put arbitrary size values into a packed bit string,
 * taking the low order bits from each value in the unpacked array.
 *
 * @param out Pointer to packed array output. Must be allocated large
 * enough to hold output.
 * @param in Pointer to unpacked array input.
 * @param iskip Initial number of bits to skip.
 * @param nbits Number of bits to pack.
 * @param nskip Additional number of bits to skip on each iteration.
 * @param n Number of iterations.
 *
 * @author NOAA Programmer
 */
void
sbits(unsigned char *out, g2int *in, g2int iskip, g2int nbits,
      g2int nskip, g2int n)
{
    g2int i, bitcnt, tbit, ibit, itmp, imask, itmp2, itmp3;
    g2int nbit, index;
    static g2int ones[]={1, 3, 7, 15, 31, 63, 127, 255};

    /* number bits from zero to ... nbit is the last bit of the field
     * to be filled. */
    nbit = iskip + nbits - 1;
    for (i = 0; i < n; i++)
    {
        itmp = *(in + i);
        bitcnt = nbits;
        index = nbit / 8;
        ibit = nbit % 8;
        nbit = nbit + nbits + nskip;

        /*        make byte aligned  */
        if (ibit != 7)
        {
            tbit = (bitcnt < (ibit+1)) ? bitcnt : ibit + 1;  /* find min */
            imask = ones[tbit - 1] << (7 - ibit);
            itmp2 = (itmp << (7 - ibit)) & imask;
            itmp3 = (int)*(out + index) & (255 - imask);
            out[index] = (unsigned char)(itmp2 | itmp3);
            bitcnt = bitcnt - tbit;
            itmp = itmp >> tbit;
            index--;
        }
        /*        now byte aligned */

        /*        do by bytes */
        while (bitcnt >= 8)
        {
            out[index] = (unsigned char)(itmp & 255);
            itmp = itmp >> 8;
            bitcnt = bitcnt - 8;
            index--;
        }

        /*        do last byte */
        if (bitcnt > 0)
        {
            itmp2 = itmp & ones[bitcnt - 1];
            itmp3 = (int)*(out + index) & (255 - ones[bitcnt - 1]);
            out[index] = (unsigned char)(itmp2 | itmp3);
        }
    }

}
