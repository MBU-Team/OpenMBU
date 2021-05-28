#ifndef GFXRASTER_INCLUDED // -*- C++ -*-
#define GFXRASTER_INCLUDED
#if !defined(__GNUC__)
#  pragma once
#endif

/************************************************************************

  Raster image support.
    
  $Id: raster.h,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include "gfx.h"
#include "vec2.h"

typedef TVec2<short> PixelAddress;

template<class T>
class Raster
{
private:
    T *data;
    int W, H, nchan;

public:
    Raster(int w, int h, int c)
    {
	W=w; H=h; nchan=c;
	data = new T[length()];
    }
    virtual ~Raster() { delete[] data; }

    int width() const { return W; }
    int height() const { return H; }
    int channels() const { return nchan; }
    int length() const { return W*H*nchan; }

    T& operator[](int i)       { return data[i]; }
    T  operator[](int i) const { return data[i]; }

    virtual T       *pixel(int i, int j)       { return data + (j*W+i)*nchan; }
    virtual const T *pixel(int i, int j) const { return data + (j*W+i)*nchan; }
    T       *pixel(const PixelAddress &a)       { return pixel(a[0], a[1]); }
    const T *pixel(const PixelAddress &a) const { return pixel(a[0], a[1]); }

    T       *head()       { return data; }
    const T *head() const { return data; }

    void reverse(int start=0, int end=-1);
    void hflip();
    void vflip();
};


class FloatRaster;
class ByteRaster : public Raster<unsigned char>
{
public:
    ByteRaster(int w, int h, int c) : Raster<unsigned char>(w,h,c) {}
    ByteRaster(const ByteRaster& img);
    ByteRaster(const FloatRaster& img);
};


class FloatRaster : public Raster<float>
{
public:
    FloatRaster(int w, int h, int c) : Raster<float>(w,h,c) {}
    FloatRaster(const FloatRaster &img);
    FloatRaster(const ByteRaster &img);
};


////////////////////////////////////////////////////////////////////////
//
// Templated raster methods
//

template<class T>
inline void Raster<T>::reverse(int start, int end)
{
    if(end<0 || end>=length()) end = length() - channels();

    int i=start, j=end;
    
    while(i<j)
    {
	for(int k=0; k<channels(); k++)
	{
	    T tmp = (*this)[i+k];
	    (*this)[i+k] = (*this)[j+k];
	    (*this)[j+k] = tmp;
	}
	i += channels();
	j -= channels();
    }
}

template<class T>
inline void Raster<T>::hflip()
{
     int i = 0;
     int j = channels()*(width()-1);

     while( i<length() )
     {
	 reverse(i, i+j);
	 i += j + channels();
     }
}

template<class T>
inline void Raster<T>::vflip()
{
    reverse();
    hflip();
}


////////////////////////////////////////////////////////////////////////
//
// Supported external image file formats.
//

enum { IMG_PNM=0, IMG_PNG=1, IMG_TIFF=2, IMG_JPEG=3, IMG_LIMIT=4 };

extern const char *image_type_name(int type);
extern const char *image_type_ext(int type);
extern int infer_image_type(const char *filename);

// Image I/O based on filename extensions
extern bool write_image(const char *filename, const ByteRaster&, int type=-1);
extern ByteRaster *read_image(const char *filename, int type=-1);

// PNM support provided by libgfx (always available)
extern bool will_write_raw_pnm;
extern bool write_pnm_image(const char *filename, const ByteRaster&);
extern ByteRaster *read_pnm_image(const char *filename);

// TIFF support provided through libtiff (if available).
extern bool write_tiff_image(const char *filename, const ByteRaster&);
extern ByteRaster *read_tiff_image(const char *filename);

// PNG support provided through libpng (if available).
extern bool write_png_image(const char *filename, const ByteRaster&);
extern ByteRaster *read_png_image(const char *filename);

// JPEG support provided through libjpeg (if available)
extern int jpeg_output_quality;
extern bool write_jpeg_image(const char *filename, const ByteRaster&);
extern ByteRaster *read_jpeg_image(const char *filename);

// GFXRASTER_INCLUDED
#endif
