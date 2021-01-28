/************************************************************************

  PNG image file format support.

  The I/O code in this file was originally based on the example.c
  skeleton code distributed with the PNG library.

  $Id: raster-png.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include <vector>
#include <gfx/gfx.h>
#include <gfx/raster.h>

#ifdef HAVE_LIBPNG

#include <png.h>

ByteRaster *read_png_image(const char *file_name)
{
   FILE *fp = fopen(file_name, "rb");
   if( !fp ) return NULL;

   // The last three arguments can be used to set up error handling callbacks.
   png_structp png_ptr =
       png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if( !png_ptr ) { fclose(fp); return NULL; }

   // Allocate required structure to hold memory information.
   png_infop info_ptr = png_create_info_struct(png_ptr);
   if( !info_ptr )
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      return NULL;
   }

   // Because we didn't set up any error handlers, we need to be
   // prepared to handle longjmps out of the library on error
   // conditions.
   if( setjmp(png_ptr->jmpbuf) )
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
      return NULL;
   }

   png_init_io(png_ptr, fp);

   // Read in all the image information
   png_read_info(png_ptr, info_ptr);

   // Get the header for the first image chunk
   png_uint_32 width, height;
   int bit_depth, color_type, interlace_type;

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);


   ////////////////////////////////////////////////////////////////
   // the following tell the PNG library how to transform the image
   // during input

   if( bit_depth == 16 )
       // truncate 16 bits/pixel to 8 bits/pixel
       png_set_strip_16(png_ptr);

   if( color_type == PNG_COLOR_TYPE_PALETTE )
       // expand paletted colors into RGB color values
       png_set_expand(png_ptr);
   else if( color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 )
       // expand grayscale images to full 8 bits/pixel
       png_set_expand(png_ptr);

   // Expand paletted or RGB images with transparency to full alpha
   // channels so the data will be available as RGBA quartets.
   if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
      png_set_expand(png_ptr);

   // update the palette and info structure
   png_read_update_info(png_ptr, info_ptr);


   // read the image data
   std::vector<png_bytep> row_pointers(height);
   int row;
   int nchan = png_get_channels(png_ptr, info_ptr);
   int nbytes = png_get_rowbytes(png_ptr, info_ptr);

   for (row = 0; row < height; row++)
      row_pointers[row] = (png_bytep)malloc(nbytes);

   png_read_image(png_ptr, &row_pointers.front());
   png_read_end(png_ptr, info_ptr);

   // Read it into a ByteRaster structure
   ByteRaster *img = new ByteRaster(width, height, nchan);

   unsigned char *pixel = img->pixel(0,0);
   for(row=0; row<height; row++)
   {
       memcpy(pixel, row_pointers[row], nbytes);
       pixel += nbytes;
   }

   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
   for(row=0; row<height; row++) free(row_pointers[row]);

   fclose(fp);
   return img;
}

bool write_png_image(const char *file_name, const ByteRaster& img)
{
   FILE *fp = fopen(file_name, "wb");
   if( !fp ) return false;

   png_structp png_ptr =
       png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if( !png_ptr ) { fclose(fp); return false; }

   png_infop info_ptr = png_create_info_struct(png_ptr);
   if( !info_ptr )
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return false;
   }

   if( setjmp(png_ptr->jmpbuf) )
   {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
      return false;
   }

   png_init_io(png_ptr, fp);

   int img_type = PNG_COLOR_TYPE_RGB;
   switch( img.channels() )
   {
   case 1:  img_type=PNG_COLOR_TYPE_GRAY; break;
   case 2:  img_type=PNG_COLOR_TYPE_GRAY_ALPHA; break;
   case 4:  img_type=PNG_COLOR_TYPE_RGB_ALPHA; break;
   }

   png_set_IHDR(png_ptr, info_ptr,
		img.width(), img.height(), 8, img_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   png_write_info(png_ptr, info_ptr);

   std::vector<png_bytep> row_pointers(img.height());
   for(int k=0; k<img.height(); k++)
     row_pointers[k] = (png_bytep)img.head() +k*img.width()*img.channels();

   png_write_image(png_ptr, &row_pointers.front());
   png_write_end(png_ptr, info_ptr);

   png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
   fclose(fp);
   return true;
}

#else

bool write_png_image(const char *, const ByteRaster&) { return false; }
ByteRaster *read_png_image(const char *) { return NULL; }

#endif
