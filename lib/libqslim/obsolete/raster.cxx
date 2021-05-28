/************************************************************************

  Raster image support.
  
  $Id: raster.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "gfx.h"
#include "raster.h"

#include <string>
#include <cctype>

ByteRaster::ByteRaster(const ByteRaster &img)
    : Raster<unsigned char>(img.width(), img.height(), img.channels())
{
    memcpy(head(), img.head(), img.length()*sizeof(unsigned char));
}

ByteRaster::ByteRaster(const FloatRaster &img)
    : Raster<unsigned char>(img.width(), img.height(), img.channels())
{
    for(int i=0; i<length(); i++ )
        (*this)[i] = (unsigned char) (255.0f * img[i]);
}

FloatRaster::FloatRaster(const ByteRaster &img)
    : Raster<float>(img.width(), img.height(), img.channels())
{
    for(int i=0; i<length(); i++)
	(*this)[i] = (float)img[i] / 255.0f;
}

FloatRaster::FloatRaster(const FloatRaster &img)
    : Raster<float>(img.width(), img.height(), img.channels())
{
    memcpy(head(), img.head(), img.length()*sizeof(float));
}

////////////////////////////////////////////////////////////////////////
//
// Table of supported formats
//



static char *img_names[] = {"PPM", "PNG", "TIFF", "JPEG"};
static char *img_ext[] = {"ppm", "png", "tif", "jpg"};

const char *image_type_name(int type)
	{ return type>=IMG_LIMIT ? NULL : img_names[type]; }

const char *image_type_ext(int type)
	{ return type>=IMG_LIMIT ? NULL : img_ext[type]; }


int infer_image_type(const char *filename)
{
    const char *ext = strrchr(filename, '.');
    if( !ext ) return -1;

    // Make sure extension is lower case
    std::string lo(ext+1);
    for(int i=0; i<lo.length(); i++)  lo[i] = tolower(lo[i]);

    // Search for extension in the table
    for(int typ=0; typ<IMG_LIMIT; typ++)
	if(lo == img_ext[typ])  return typ;

    // Test for alternatives
    if(lo=="tiff")  return IMG_TIFF;

    // Unknown type
    return -1;
}


bool write_image(const char *filename, const ByteRaster& img, int type)
{
    if( type<0 )
	type = infer_image_type(filename);

    switch( type )
    {
    case IMG_PNM:  return write_pnm_image(filename, img);
    case IMG_PNG:  return write_png_image(filename, img);
    case IMG_TIFF: return write_tiff_image(filename, img);
    case IMG_JPEG: return write_jpeg_image(filename, img);
    default:       return false;
    }
}

ByteRaster *read_image(const char *filename, int type)
{
    if( type<0 )
	type = infer_image_type(filename);

    switch( type )
    {
    case IMG_PNM:  return read_pnm_image(filename);
    case IMG_PNG:  return read_png_image(filename);
    case IMG_TIFF: return read_tiff_image(filename);
    case IMG_JPEG: return read_jpeg_image(filename);
    default:       return NULL;
    }
}
