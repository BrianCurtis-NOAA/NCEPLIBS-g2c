/** @file
 * @brief Pack up a data field into a JPEG2000 code stream.
 * @author Stephen Gilbert @date 2003-08-17
 */
#include <stdlib.h>
#include <math.h>
#include "grib2_int.h"

/**
 * This subroutine packs up a data field into a JPEG2000 code
 * stream. After the data field is scaled, and the reference value is
 * subtracted out, it is treated as a grayscale image and passed to a
 * JPEG2000 encoder. It also fills in GRIB2 Data Representation
 * Template 5.40 or 5.40000 with the appropriate values.
 *
 * ### Program History Log
 * Date | Programmer | Comments
 * -----|------------|---------
 * 2003-08-17 | Gilbert | Initial.
 * 2004-11-92 | Gilbert | Fixed bug packing a near constant field.
 * 2004-07-19 | Gilbert | If jpeg2000 encoding fails, try again with different encoder options.
 * 2005-05-10 | Gilbert | Imposed minimum size on cpack.
 *
 * @param fld Contains the data values to pack.
 * @param width number of points in the x direction.
 * @param height number of points in the y direction.
 * @param idrstmpl Contains the array of values for Data
 * Representation Template [Table
 * 5.40](https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_temp5-40.shtml)
 * or 5.40000.
 * - 0 Reference value - ignored on input, set by jpcpack routine.
 * - 1 Binary Scale Factor - used on input, unchanged by jpcpack
 routine.
 * - 2 Decimal Scale Factor - used on input, unchanged by jpcpack
 routine.
 * - 3 number of bits for each data value - ignored on input
 * - 4 Original field type - currently ignored on input Data values
 assumed to be reals. Set to 0 on output.
 * - 5 if 0 use lossless compression, if 1 use lossy compression.
 * - 6 Desired compression ratio, if idrstmpl[5]=1. Set to 255, if
 idrstmpl[5]=0.
 * @param cpack The packed data field.
 * @param lcpack Pointer that points to a g2int which contains the
 * length of packed field in cpack. This must be set by the calling
 * function.
 *
 * @author Stephen Gilbert @date 2003-08-17
 */
void
jpcpack(g2float *fld, g2int width, g2int height, g2int *idrstmpl,
        unsigned char *cpack, g2int *lcpack)
{
    g2int  *ifld = NULL;
    static g2float alog2 = ALOG2;       /*  ln(2.0) */
    g2int  j, nbits, imin, imax, maxdif;
    g2int  ndpts, nbytes, nsize, retry;
    g2float  bscale, dscale, rmax, rmin, temp;
    unsigned char *ctemp;

    ndpts = width * height;
    bscale = int_power(2.0, -idrstmpl[1]);
    dscale = int_power(10.0, idrstmpl[2]);

    /* Find max and min values in the data. */
    rmax = fld[0];
    rmin = fld[0];
    for (j = 1; j < ndpts; j++)
    {
        if (fld[j] > rmax)
            rmax = fld[j];
        if (fld[j] < rmin)
            rmin = fld[j];
    }
    if (idrstmpl[1] == 0)
        maxdif = (g2int)(rint(rmax * dscale) - rint(rmin * dscale));
    else
        maxdif = (g2int)rint((rmax - rmin) * dscale * bscale);

    /* If max and min values are not equal, pack up field. If they are
     * equal, we have a constant field, and the reference value (rmin)
     * is the value for each point in the field and set nbits to 0. */
    if (rmin != rmax && maxdif != 0)
    {
        ifld = malloc(ndpts * sizeof(g2int));

        /* Determine which algorithm to use based on user-supplied
         * binary scale factor and number of bits. */
        if (idrstmpl[1] == 0)
        {
            /*  No binary scaling and calculate minumum number of bits
             *  in which the data will fit. */
            imin = (g2int)rint(rmin * dscale);
            imax = (g2int)rint(rmax * dscale);
            maxdif = imax - imin;
            temp = log((double)(maxdif + 1)) / alog2;
            nbits = (g2int)ceil(temp);
            rmin = (g2float)imin;
            /*   scale data */
            for(j = 0; j < ndpts; j++)
                ifld[j] = (g2int)rint(fld[j] * dscale) - imin;
        }
        else
        {
            /* Use binary scaling factor and calculate minumum number
             * of bits in which the data will fit. */
            rmin = rmin * dscale;
            rmax = rmax * dscale;
            maxdif = (g2int)rint((rmax - rmin) * bscale);
            temp = log((double)(maxdif + 1)) / alog2;
            nbits = (g2int)ceil(temp);
            /*   scale data */
            for (j = 0; j < ndpts; j++)
                ifld[j] = (g2int)rint(((fld[j] * dscale) - rmin) * bscale);
        }

        /* Pack data into full octets, then do JPEG 2000 encode and
         * calculate the length of the packed data in bytes. */
        retry = 0;
        nbytes = (nbits + 7) / 8;
        nsize = *lcpack;          /* needed for input to enc_jpeg2000 */
        ctemp = calloc(ndpts, nbytes);
        sbits(ctemp, ifld, 0, nbytes * 8, 0, ndpts);
        if ((*lcpack = (g2int)enc_jpeg2000(ctemp, width, height, nbits, idrstmpl[5],
                                           idrstmpl[6], retry, (char *)cpack, nsize)) <= 0)
        {
            printf("jpcpack: ERROR Packing JPC = %d\n", (int)*lcpack);
            if (*lcpack == -3)
            {
                retry = 1;
                if ((*lcpack = (g2int)enc_jpeg2000(ctemp, width, height, nbits, idrstmpl[5],
                                                   idrstmpl[6], retry, (char *)cpack, nsize)) <= 0)
                    printf("jpcpack: Retry Failed.\n");
                else
                    printf("jpcpack: Retry Successful.\n");
            }
        }
        free(ctemp);
    }
    else
    {
        nbits = 0;
        *lcpack = 0;
    }

    /* Fill in ref value and number of bits in Template 5.0. */
    mkieee(&rmin, idrstmpl, 1);   /* ensure reference value is IEEE format. */
    idrstmpl[3] = nbits;
    idrstmpl[4] = 0;         /* original data were reals */
    if (idrstmpl[5] == 0)
        idrstmpl[6] = 255;       /* lossy not used */
    if (ifld)
        free(ifld);
}
