/** @file
 * @brief Encodes JPEG2000 code stream.
 * @author Stephen Gilbert @date 2002-12-02
 */

#include <stdio.h>
#include <stdlib.h>
#include "grib2_int.h"
#include "jasper/jasper.h"

#define MAXOPTSSIZE 1024 /**< Maximum size of options. */

/**
 * This Function encodes a grayscale image into a JPEG2000 code stream
 * specified in the JPEG2000 Part-1 standard (i.e., ISO/IEC 15444-1)
 * using JasPer Software version 1.500.4 (or 1.700.2) written by the
 * University of British Columbia, Image Power Inc, and others.
 * JasPer is available at http://www.ece.uvic.ca/~mdadams/jasper/.
 *
 * ### Program History Log
 * Date | Programmer | Comments
 * -----|------------|---------
 * 2002-12-02 | Gilbert | Initial
 * 2004-12-16 | Gilbert | Added retry argument allowing increased guard bits.
 *
 * @param cin Packed matrix of Grayscale image values to encode.
 * @param width width of image.
 * @param height height of image.
 * @param nbits depth (in bits) of image.  i.e number of bits used to
 * hold each data value.
 * @param ltype indicator of lossless or lossy compression.
 * - 1, for lossy compression
 * - != 1, for lossless compression
 * @param ratio target compression ratio. (ratio:1) Used only when
 * ltype == 1.
 * @param retry If 1 try increasing number of guard bits otherwise, no
 * additional options.
 * @param outjpc Output encoded JPEG2000 code stream.
 * @param jpclen Number of bytes allocated for the output JPEG2000
 * code stream in outjpc.
 *
 * @return
 * - > 0 = Length in bytes of encoded JPEG2000 code stream
 * - -3 = Error encode jpeg2000 code stream.
 *
 * @note Requires JasPer Software version 1.500.4 or 1.700.2 or later.
 *
 * @author Stephen Gilbert @date 2002-12-02
 */
int
enc_jpeg2000(unsigned char *cin, g2int width, g2int height, g2int nbits,
             g2int ltype, g2int ratio, g2int retry, char *outjpc,
             g2int jpclen)
{
    int ier, rwcnt;
    jas_image_t image;
    jas_stream_t *jpcstream, *istream;
    jas_image_cmpt_t cmpt, *pcmpt;
    char opts[MAXOPTSSIZE];

    /* Set lossy compression options, if requested. */
    if (ltype != 1)
        opts[0] = (char)0;
    else
        snprintf(opts,MAXOPTSSIZE,"mode=real\nrate=%f",1.0/(float)ratio);

    if (retry == 1)  /* option to increase number of guard bits */
        strcat(opts,"\nnumgbits=4");

    /* Initialize the JasPer image structure describing the grayscale
     * image to encode into the JPEG2000 code stream. */
    image.tlx_ = 0;
    image.tly_ = 0;
    image.brx_ = (jas_image_coord_t)width;
    image.bry_ = (jas_image_coord_t)height;
    image.numcmpts_ = 1;
    image.maxcmpts_ = 1;
    image.clrspc_ = JAS_CLRSPC_SGRAY;         /* grayscale Image */
    image.cmprof_ = 0;

    cmpt.tlx_ = 0;
    cmpt.tly_ = 0;
    cmpt.hstep_ = 1;
    cmpt.vstep_ = 1;
    cmpt.width_ = (jas_image_coord_t)width;
    cmpt.height_ = (jas_image_coord_t)height;
    cmpt.type_ = JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y);
    cmpt.prec_ = nbits;
    cmpt.sgnd_ = 0;
    cmpt.cps_ = (nbits + 7) / 8;

    pcmpt = &cmpt;
    image.cmpts_ = &pcmpt;

    /* Open a JasPer stream containing the input grayscale values. */
    istream = jas_stream_memopen((char *)cin, height * width * cmpt.cps_);
    cmpt.stream_ = istream;

    /* Open an output stream that will contain the encoded jpeg2000
     * code stream. */
    jpcstream = jas_stream_memopen(outjpc, (int)jpclen);

    /* Encode image. */
    if ((ier = jpc_encode(&image, jpcstream, opts)))
    {
        printf(" jpc_encode return = %d \n",ier);
        return -3;
    }

    /* Clean up JasPer work structures. */
    rwcnt = jpcstream->rwcnt_;
    ier = jas_stream_close(istream);
    ier = jas_stream_close(jpcstream);

    /* Return size of jpeg2000 code stream. */
    return (rwcnt);
}
