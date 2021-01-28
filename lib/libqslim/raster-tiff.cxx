/************************************************************************

  TIFF image file format support.
  
  $Id: raster-tiff.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include <gfx/gfx.h>
#include <gfx/raster.h>
#include <cstring>

#ifdef HAVE_LIBTIFF
#include <tiffio.h>

////////////////////////////////////////////////////////////////////////
//
// TIFF output
//
static
bool __tiff_write(TIFF *tif, const ByteRaster& img)
{
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, img.width());
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, img.height());

    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, img.channels());
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, 
		 img.channels()==1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);

#ifdef HAVE_LIBTIFF_LZW
    //
    // LZW compression is problematic because it is patented by Unisys.
    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    //
    // Predictors:
    //     1 (default) -- No predictor
    //     2           -- Horizontal differencing
    TIFFSetField(tif, TIFFTAG_PREDICTOR, 2);
#endif

    uint32 scanline_size = img.channels() * img.width();
    if( TIFFScanlineSize(tif) != scanline_size )
	// ??BUG: Can this mismatch of scanline sizes every occur?
	return false;


    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, 0));

    char *scanline_buf = new char[scanline_size];

    const unsigned char *scanline = img.head();
    for(int y=0; y<img.height(); y++)
    {
	memcpy(scanline_buf, scanline, scanline_size);
	// NOTE: TIFFWriteScanline modifies the buffer you pass it.
	//       Thus, we need to copy stuff out of the raster first.
	TIFFWriteScanline(tif, scanline_buf, y, 0);
	scanline += scanline_size;
    }
    delete[] scanline_buf;

    return true;
}

bool write_tiff_image(const char *filename, const ByteRaster& img)
{
    TIFF *tif = TIFFOpen(filename, "w");
    if( !tif ) return false;

    bool result = __tiff_write(tif, img);

    TIFFClose(tif);

    return result;
}

////////////////////////////////////////////////////////////////////////
//
// TIFF input
//
static
void unpack_tiff_raster(uint32 *raster, ByteRaster *img, int npixels)
{
    unsigned char *pix = img->head();
    
    for(int i=0; i<npixels; i++)
    {
	*pix++ = TIFFGetR(raster[i]);
	if( img->channels() >= 3 )
	{
	    *pix++ = TIFFGetG(raster[i]);
	    *pix++ = TIFFGetB(raster[i]);
	    if( img->channels() == 4 )
		*pix++ = TIFFGetA(raster[i]);
	}
    }
}

static
ByteRaster *__tiff_read(TIFF *tif)
{
    uint32 w, h;
    uint16 nchan;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nchan);

    int npixels = w*h;

    uint32 *raster = (uint32 *)_TIFFmalloc(npixels * sizeof(uint32));
    if( !raster ) return NULL;

    TIFFReadRGBAImage(tif, w, h, raster, true);

    ByteRaster *img = new ByteRaster(w, h, nchan);
    unpack_tiff_raster(raster, img, npixels);
    //
    // libtiff returned the pixels with the origin in the lower left
    // rather than the upper left corner.  We fix that by flipping the
    // pixels.
    //
    img->vflip();


    _TIFFfree(raster);

    return img;
}

ByteRaster *read_tiff_image(const char *filename)
{
    TIFF *tif = TIFFOpen(filename, "r");
    if( !tif ) return NULL;

    ByteRaster *img = __tiff_read(tif);

    TIFFClose(tif);

    return img;
}

#else

bool write_tiff_image(const char *, const ByteRaster&) { return false; }
ByteRaster *read_tiff_image(const char *) { return NULL; }

#endif
