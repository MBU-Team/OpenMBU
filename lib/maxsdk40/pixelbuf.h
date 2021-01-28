//-----------------------------------------------------------------------------
// --------------------
// File ....: pixelbuf.h
// --------------------
// Author...: Tom Hudson
// Date ....: Dec. 09, 1995
// Descr....: Pixel Buffer Classes
// Usage....: These templated classes let you set up a buffer for pixels that
//            will automatically clean itself up when it goes out of scope.
//
// History .: Dec. 09 1995 - Started file
//            
//-----------------------------------------------------------------------------

#ifndef __PIXBUF_H__

#define __PIXBUF_H__

// Handy-dandy pixel buffer classes:

template <class T> class PixelBufT {
private:
     T *buf;
     int width;
public:
     inline               PixelBufT(int width) { buf = (T *)calloc(width,sizeof(T)); this->width=width; };
     inline               ~PixelBufT() { if(buf) free(buf); };
     inline   T*          Ptr() { return buf; };
	 inline   T&          operator[](int i) { return buf[i]; }
           int            Fill(int start, int count, T color) {
                          int ix,jx=start+count;
                          if(jx >= width)
                             return 0;
                          for(ix=start; ix<jx; buf[ix++]=color);
                          return 1;
                          };
     };

typedef PixelBufT<UBYTE> PixelBuf8;
typedef PixelBufT<USHORT> PixelBuf16;
typedef PixelBufT<BMM_Color_24> PixelBuf24;
typedef PixelBufT<BMM_Color_32> PixelBuf32;
typedef PixelBufT<BMM_Color_48> PixelBuf48;
typedef PixelBufT<BMM_Color_64> PixelBuf64;

#endif // __PIXBUF_H__
