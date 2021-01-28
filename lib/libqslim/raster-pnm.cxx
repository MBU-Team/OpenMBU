/************************************************************************

  PNM image file format support
  
  $Id: raster-pnm.cxx,v 1.2 2006/09/07 00:05:52 bramage Exp $

 ************************************************************************/

#include <gfx/gfx.h>
#include <gfx/raster.h>

#include <fstream>
using namespace std;

bool will_write_raw_pnm = true;

////////////////////////////////////////////////////////////////////////
//
// PNM output routine
//

bool write_pnm_image(const char *filename, const ByteRaster& img)
{
    ofstream out(filename, ios::out|ios::binary);
    if( !out.good() ) return false;

    bool is_raw = will_write_raw_pnm;

    //
    // First, write the PNM header.
    // 
    char magic;
    if(img.channels() == 1)      magic = is_raw ? '5':'2';   // PGM
    else if(img.channels() < 3)  return false;               // unsupported
    else                         magic = is_raw ? '6':'3';   // truncate to PPM

    out << "P" << magic << " "
	<< img.width() << " " << img.height() << " 255" << endl;

    //
    // Now, write the PNM data.  If there are more than 3 channels,
    // we'll just write out the first 3 as RGB and ignore the rest.
    //
    if( is_raw )
    {
	if( img.channels() > 3 )
	    for(int i=0; i<img.length(); i+=img.channels())
		out.write((const char *)img.pixel(0,0)+i, 3);
	else
	    out.write((const char *)img.pixel(0,0), img.length());
    }
    else
    {
	for(int i=0; i<img.length(); i+=img.channels())
	{
	    out << (int)(img[i]) << " "
		<< (int)(img[i+1]) << " "
		<< (int)(img[i+2]) << endl;
	}
    }

    return true;
}

////////////////////////////////////////////////////////////////////////
//
// PNM input routines
//

static
istream& pnm_skip(istream& in)
{
    for(;;)
    {
	in >> ws;
	if( in.peek() == '#' )
	    in.ignore(65535, '\n');
	else
	    return in;
    }
}

static
void pnm_read_ascii(istream& in, ByteRaster& img)
{
    unsigned char *current = img.head();
    int val;

    for(int j=0; j<img.height(); j++) for(int i=0; i<img.width(); i++)
        for(int k=0; k<img.channels(); k++)
        {
            pnm_skip(in) >> val;
            *current++ = (unsigned char)val;
        }
}

static
void pnm_read_ascii(istream& in, ByteRaster& img, float scale)
{
    unsigned char *current = img.head();
    float val;

    for(int j=0; j<img.height(); j++) for(int i=0; i<img.width(); i++)
        for(int k=0; k<img.channels(); k++)
        {
            pnm_skip(in) >> val;
            *current++ = (unsigned char)(val*scale);
        }
}

static
void pnm_read_raw(istream& in, ByteRaster& img)
{
    char c;  in.get(c);  // extract 1 whitespace character

    //
    // Is this guaranteed to read all the requested bytes?
    //
    in.read((char *)img.head(), img.length());
}

ByteRaster *read_pnm_image(const char *filename)
{
    ifstream in(filename, ios::in|ios::binary);
    if( !in.good() ) return NULL;

    //
    // Read the PNM header and allocate and appropriate raster.
    //

    unsigned char P, N;
    in >> P >> N;

    if( P!='P' )  return NULL;

    int width, height, maxval;
    pnm_skip(in) >> width;
    pnm_skip(in) >> height;
    pnm_skip(in) >> maxval;

    int magic = N - '0';
    bool is_raw = magic > 3;

    int channels = 1;
    if( magic==3 || magic==6 )
	channels = 3;

    ByteRaster *img = new ByteRaster(width, height, channels);

    //
    // Read the image data into the raster
    //

    if( is_raw )
    {
	if( maxval>255 )  return NULL;

	// BUG: We ignore the scaling implied by maxval<255

	pnm_read_raw(in, *img);
    }
    else if( maxval==255 )
	pnm_read_ascii(in, *img);
    else
	pnm_read_ascii(in, *img, 255.0f/(float)maxval);

    return img;
}
