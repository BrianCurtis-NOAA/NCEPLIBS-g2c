/** @file
 * @brief Unpack Section 7 (Data Section) as defined in GRIB Edition 2.
 * @author Stephen Gilbert @date 2002-10-31
 */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "grib2_int.h"

/**
 *
 * This subroutine unpacks Section 7 (Data Section) as defined in GRIB
 * Edition 2.
 *
 * ### Program History Log
 * Date | Programmer | Comments
 * -----|------------|---------
 * 2002-10-31 | Gilbert | Initial
 * 2002-12-20 | Gilbert | Added GDT info to arguments and added 5.51 processing.
 * 2003-08-29 | Gilbert | New templates using PNG and JPEG2000 algorithms/templates.
 * 2004-11-29 | Gilbert | JPEG2000 now allowed to use WMO Template 5.40 PNG allowed to use 5.41
 * 2004-12-16 | Taylor | Added check on comunpack return code.
 * 2008-12-23 | Wesley | Initialize Number of data points unpacked
 *
 * @param cgrib char array containing Section 7 of the GRIB2 message
 * @param iofst Bit offset of the beginning of Section 7 in cgrib.
 * @param igdsnum Grid Definition Template Number (see Code Table 3.0)
 * (Only used for DRS Template 5.51)
 * @param igdstmpl Pointer to an integer array containing the data
 * values for the specified Grid Definition Template (N=igdsnum). Each
 * element of this integer array contains an entry (in the order
 * specified) of Grid Definition Template 3.N. (Only used for DRS
 * Template 5.51).
 * @param idrsnum Data Representation Template Number (see Code Table 5.0)
 * @param idrstmpl Pointer to an integer array containing the data
 * values for the specified Data Representation Template
 * (N=idrsnum). Each element of this integer array contains an entry
 * (in the order specified) of Data Representation Template 5.N
 * @param ndpts Number of data points unpacked and returned.
 * @param fld Pointer to a float array containing the unpacked data
 * field.
 *
 * @return
 * - ::G2_NO_ERROR No error.
 * - ::G2_UNPACK_BAD_SEC Array passed had incorrect section number.
 * - ::G2_UNPACK7_BAD_DRT Unrecognized Data Representation Template.
 * - ::G2_UNPACK7_WRONG_GDT need one of GDT 3.50 through 3.53 to decode DRT 5.51
 * - ::G2_UNPACK_NO_MEM Memory allocation error.
 * - ::G2_UNPACK7_CORRUPT_SEC Corrupt section 7.
 *
 * @author Stephen Gilbert @date 2002-10-31
 */
g2int
g2_unpack7(unsigned char *cgrib, g2int *iofst, g2int igdsnum, g2int *igdstmpl,
           g2int idrsnum, g2int *idrstmpl, g2int ndpts, g2float **fld)
{
    g2int isecnum;
    g2int ipos, lensec;
    g2float *lfld;

    *fld = NULL;

    gbit(cgrib, &lensec, *iofst, 32);        /* Get Length of Section */
    *iofst = *iofst + 32;
    gbit(cgrib, &isecnum, *iofst, 8);         /* Get Section Number */
    *iofst = *iofst + 8;

    if (isecnum != 7)
        return G2_UNPACK_BAD_SEC;

    ipos = *iofst / 8;
    if (!(lfld = calloc(ndpts ? ndpts : 1, sizeof(g2float))))
        return G2_UNPACK_NO_MEM;

    *fld = lfld;

    if (idrsnum == 0)
        simunpack(cgrib + ipos, idrstmpl, ndpts, lfld);
    else if (idrsnum == 2 || idrsnum == 3)
    {
        if (comunpack(cgrib+ipos, lensec, idrsnum, idrstmpl, ndpts, lfld))
            return G2_UNPACK7_CORRUPT_SEC;
    }
    else if (idrsnum == 50)
    {            /* Spectral Simple */
        simunpack(cgrib + ipos, idrstmpl, ndpts - 1, lfld + 1);
        rdieee(idrstmpl + 4, lfld, 1);
    }
    else if (idrsnum == 51)              /* Spectral complex */
    {
        if (igdsnum >= 50 && igdsnum <= 53)
            specunpack(cgrib + ipos, idrstmpl, ndpts, igdstmpl[0], igdstmpl[2],
                       igdstmpl[2], lfld);
        else
        {
            fprintf(stderr, "g2_unpack7: Cannot use GDT 3.%d to unpack Data Section 5.51.\n",
                    (int)igdsnum);
            if (lfld)
                free(lfld);
            *fld = NULL;
            return G2_UNPACK7_WRONG_GDT;
        }
    }
#if defined USE_JPEG2000 || defined USE_OPENJPEG
    else if (idrsnum == 40 || idrsnum == 40000)
    {
        jpcunpack(cgrib + ipos, lensec - 5, idrstmpl, ndpts, lfld);
    }
#endif  /* USE_JPEG2000 */
#ifdef USE_PNG
    else if (idrsnum == 41 || idrsnum == 40010)
    {
        pngunpack(cgrib + ipos, lensec - 5, idrstmpl, ndpts, lfld);
    }
#endif  /* USE_PNG */
    else
    {
        fprintf(stderr, "g2_unpack7: Data Representation Template 5.%d not yet "
                "implemented.\n", (int)idrsnum);
        if (lfld)
            free(lfld);
        *fld = NULL;
        return G2_UNPACK7_BAD_DRT;
    }

    *iofst = *iofst + (8 * lensec);

    return G2_NO_ERROR;
}
