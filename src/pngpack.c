/** @file
 * @brief Function to pack data with PNG compression.
 * @author Stephen Gilbert @date 2003-08-27
 */
#include <stdlib.h>
#include <math.h>
#include "grib2_int.h"

/**
 * This subroutine packs up a data field into PNG image format. After
 * the data field is scaled, and the reference value is subtracted
 * out, it is treated as a grayscale image and passed to a PNG
 * encoder. It also fills in GRIB2 Data Representation Template 5.41
 * or 5.40010 with the appropriate values.
 *
 * @param fld Contains the data values to pack.
 * @param width number of points in the x direction.
 * @param height number of points in the y direction.
 * @param idrstmpl Contains the array of values for Data
 * Representation
 * [Template 5.41](https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_temp5-41.shtml)
 * or 5.40010.
 * - 0 Reference value - ignored on input, set by pngpack routine.
 * - 1 Binary Scale Factor - used on input.
 * - 2 Decimal Scale Factor - used on input.
 * - 3 number of bits for each grayscale pixel value - ignored on
 input.
 * - 4 Original field type - currently ignored on input, set = 0 on
 output. Data values assumed to be reals.
 * @param cpack The packed data field.
 * @param lcpack length of packed field cpack.
 *
 * @author Stephen Gilbert @date 2003-08-27
 */
void
pngpack(g2float *fld, g2int width, g2int height, g2int *idrstmpl, 
        unsigned char *cpack, g2int *lcpack)
{
    g2int *ifld = NULL;
    static g2float alog2 = ALOG2;       /*  ln(2.0) */
    g2int j, nbits, imin, imax, maxdif;
    g2int ndpts, nbytes;
    g2float bscale, dscale, rmax, rmin, temp;
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
    maxdif = (g2int)rint((rmax-rmin) * dscale * bscale);

    /* If max and min values are not equal, pack up field. If they are
     * equal, we have a constant field, and the reference value (rmin)
     * is the value for each point in the field and set nbits to 0. */
    if (rmin != rmax  &&  maxdif != 0)
    {
        ifld = malloc(ndpts * sizeof(g2int));

        /* Determine which algorithm to use based on user-supplied
         * binary scale factor and number of bits. */
        if (idrstmpl[1] == 0)
        {
            /* No binary scaling and calculate minumum number of bits
             * in which the data will fit. */
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

        /* Pack data into full octets, then do PNG encode and
         * calculate the length of the packed data in bytes. */
        if (nbits <= 8) 
            nbits = 8;
        else if (nbits <= 16) 
            nbits = 16;
        else if (nbits <= 24) 
            nbits = 24;
        else 
            nbits = 32;

        nbytes = (nbits / 8) * ndpts;
        ctemp = calloc(nbytes, 1);
        sbits(ctemp, ifld, 0, nbits, 0, ndpts);

        /* Encode data into PNG Format. */
        if ((*lcpack = (g2int)enc_png(ctemp, width, height, nbits,
                                      cpack)) <= 0)
            printf("pngpack: ERROR Packing PNG = %d\n", (int)*lcpack);
        
        free(ctemp);
    }
    else
    {
        nbits = 0;
        *lcpack = 0;
    }

    /* Fill in ref value and number of bits in Template 5.0. */
    mkieee(&rmin, idrstmpl, 1);   /* ensure reference value is IEEE format */
    idrstmpl[3] = nbits;
    idrstmpl[4] = 0;         /* original data were reals */
    
    if (ifld)
        free(ifld);
}
