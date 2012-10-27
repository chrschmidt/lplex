/*
	Copyright (C) 2012 Christian Schmidt

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2 of the License, or (at your
	option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
	Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation,
	Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef __audioio_h_included__
#define __audioio_h_included__

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}
#include <FLAC++/all.h>
#include <processor.hpp>

int audiofile_audit (const char * filename, FLAC__StreamMetadata *meta);

class lavReader: public lpcmReader {
  AVFormatContext * format;
  int stream;
  int sampleformat;
  AVPacket packets[2];
  AVFrame * audioframe;
  AVAudioResampleContext * inputconv;

  void close();
  
public:
  lavReader (const char *filename, unsigned char *buf, uint32_t size, int alignUnit=0);
  ~lavReader ();

  virtual uint64_t fillBuf( uint64_t limit=0, counter<uint64_t> *midCount=NULL );
  virtual uint16_t reset( const char *filename, int alignUnit=0 );
};

#endif
