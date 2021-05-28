/************************************************************************

  JPEG image file format support.

  The I/O code in this file was originally based on the example.c
  skeleton code distributed with the JPEG library (release 6b).

  $Id: raster-jpeg.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "gfx.h"
#include "raster.h"

// Quality factors are expressed on a 0--100 scale
int jpeg_output_quality = 100;

#ifdef HAVE_LIBJPEG

#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}

bool write_jpeg_image(const char *filename, const ByteRaster& img)
{
    FILE *outfile = fopen(filename, "wb");
    if( !outfile ) return false;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo);

    cinfo.image_width = img.width();
    cinfo.image_height = img.height();
    cinfo.input_components = img.channels();

    if(img.channels()==1)	cinfo.in_color_space = JCS_GRAYSCALE;
    else if(img.channels()==3)	cinfo.in_color_space = JCS_RGB;
    else			cinfo.in_color_space = JCS_UNKNOWN;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, jpeg_output_quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    int row_stride = img.width() * img.channels();
    const unsigned char *scanline = img.head();
    while( cinfo.next_scanline < cinfo.image_height )
    {
	(void) jpeg_write_scanlines(&cinfo, (JSAMPLE **)&scanline, 1);
	scanline += row_stride;
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);

    return true;
}

ByteRaster *read_jpeg_image(const char *filename)
{
    FILE *infile = fopen(filename, "rb");
    if( !infile )  return NULL;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo);

    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo);

    ByteRaster *img = new ByteRaster(cinfo.output_width,
				     cinfo.output_height,
				     cinfo.output_components);

    int row_stride = cinfo.output_width * cinfo.output_components;
    unsigned char *scanline = img->head();
    while( cinfo.output_scanline < cinfo.output_height)
    {
	(void) jpeg_read_scanlines(&cinfo, (JSAMPLE **)&scanline, 1);
	scanline += row_stride;
    }

    (void) jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return img;
}

#else

bool write_jpeg_image(const char *, const ByteRaster&) { return false; }
ByteRaster *read_jpeg_image(const char *) { return NULL; }

#endif
