/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: stdio-based convenience library for opening/seeking/decoding

 ********************************************************************/

// modified vorbisfile to use Torque Streams
// Kurtis Seebaldt


#ifndef _OV_FILE_H_
#define _OV_FILE_H_

#include <stdio.h>
#include "vorbis/codec.h"

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

/* The function prototypes for the callbacks are basically the same as for
 * the stdio functions fread, fseek, fclose, ftell.
 * The one difference is that the FILE * arguments have been replaced with
 * a void * - this is to be used as a pointer to whatever internal data these
 * functions might need. In the stdio case, it's just a FILE * cast to a void *
 *
 * If you use other functions, check the docs for these functions and return
 * the right values. For seek_func(), you *MUST* return -1 if the stream is
 * unseekable
 */
//typedef struct {
//  size_t (*read_func)  (Stream *ptr, size_t size, size_t nmemb, void *datasource);
//  int    (*seek_func)  (Stream *datasource, ogg_int64_t offset, int whence);
//  int    (*close_func) (void *datasource);
//  long   (*tell_func)  (void *datasource);
//} ov_callbacks;

#define  NOTOPEN   0
#define  PARTOPEN  1
#define  OPENED    2
#define  STREAMSET 3
#define  INITSET   4

typedef struct OggVorbis_File {
  Stream           *datasource; /* Pointer to a FILE *, etc. */
  int              seekable;
  ogg_int64_t      offset;
  ogg_int64_t      end;
  ogg_sync_state   oy;

  /* If the FILE handle isn't seekable (eg, a pipe), only the current
     stream appears */
  int              links;
  ogg_int64_t     *offsets;
  ogg_int64_t     *dataoffsets;
  long            *serialnos;
  ogg_int64_t     *pcmlengths;
  vorbis_info     *vi;
  vorbis_comment  *vc;

  /* Decoding working state local storage */
  ogg_int64_t      pcm_offset;
  int              ready_state;
  long             current_serialno;
  int              current_link;

  double           bittrack;
  double           samptrack;

  ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

//  ov_callbacks callbacks;

} OggVorbis_File;


class OggVorbisFile {

public:
	OggVorbisFile();
	~OggVorbisFile();
	int ov_clear();
	int ov_open(Stream *stream,char *initial,long ibytes);
	int ov_open_callbacks(Stream *datasource,
		char *initial, long ibytes);

	int ov_test(Stream *stream,char *initial,long ibytes);
	int ov_test_callbacks(Stream *datasource,
		char *initial, long ibytes);
	int ov_test_open();

	long ov_bitrate(int i);
	long ov_bitrate_instant();
	long ov_streams();
	long ov_seekable();
	long ov_serialnumber(int i);

	ogg_int64_t ov_raw_total(int i);
	ogg_int64_t ov_pcm_total(int i);
	double ov_time_total(int i);

	int ov_raw_seek(long pos);
	int ov_pcm_seek(ogg_int64_t pos);
	int ov_pcm_seek_page(ogg_int64_t pos);
	int ov_time_seek(double pos);
	int ov_time_seek_page(double pos);

	ogg_int64_t ov_raw_tell();
	ogg_int64_t ov_pcm_tell();
	double ov_time_tell();

	vorbis_info *ov_info(int link);
	vorbis_comment *ov_comment(int link);

	long ov_read_float(float ***pcm_channels,
			  int *bitstream);
	long ov_read(char *buffer,int length,
		    int bigendianp,int *bitstream);

private:
	OggVorbis_File *vf;

	long _get_data();
	void _seek_helper(long offset);
	long _get_next_page(ogg_page *og,int boundary);
	long _get_prev_page(ogg_page *og);
	int _bisect_forward_serialno(long begin,long searched,long end,long currentno,long m);
	int _fetch_headers(vorbis_info *vi,vorbis_comment *vc,long *serialno,ogg_page *og_ptr);
	void _prefetch_all_headers(long dataoffset);
	void _make_decode_ready();
	int _open_seekable2();
	void _decode_clear();
	int _process_packet(int readp);
	int _fseek64_wrap(Stream *stream,ogg_int64_t off,int whence);
	int _ov_open1(Stream *stream,char *initial,long ibytes);
	int _ov_open2();

};

#endif
