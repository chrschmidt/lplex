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

#include "audioio.hpp"
#include "util.h"

static int open_file (const char * filename, AVFormatContext ** format) {
  int i;
  AVCodec * codec = NULL;

  if (avformat_open_input (format, filename, NULL, NULL)) {
    ERR (_f ("%s is not in a libavformat supported format.\n", filename));
    return -1;
  }
  if (avformat_find_stream_info (*format, NULL) < 0) {
    ERR (_f ("Failed to acquire stream info for %s\n", filename));
    return -1;
  }
  for (i=0; i<(*format)->nb_streams; i++)
    if ((*format)->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
      break;
  if (i == (*format)->nb_streams) {
    ERR (_f ("No audio stream found in %s\n", filename));
    return -1;
  }
  if (!(codec = avcodec_find_decoder ((*format)->streams[i]->codec->codec_id))) {
    ERR (_f ("No suitable decoder found for audio stream in %s\n", filename));
    return -1;
  }
  if (avcodec_open2 ((*format)->streams[i]->codec, codec, NULL)) {
    ERR (_f ("Failed to initialize decoder for audio stream in %s\n", filename));
    return -1;
  }

  return i;
}

static int get_decoded_frame (AVFormatContext * file, const int stream, AVFrame ** frame, AVPacket * packets) {
  int result, done, consumed;
  
  if (!*frame && !(*frame = avcodec_alloc_frame()))
    FATAL ("Unable to allocate frame\n");
  
  do {
    while (!packets[0].size) {
      if ((result = av_read_frame (file, &packets[0]))<0)
	return result;
      if (packets[0].stream_index != stream)
	av_free_packet (&packets[0]);
      else
	packets[1] = packets[0];
    }
    consumed = avcodec_decode_audio4 (file->streams[stream]->codec, *frame, &done, &packets[0]);
    if (consumed<0)
      return consumed;
    else if (consumed==0 && packets[0].size>0)
      FATAL ("get_decoded_frame: decoder consumed no data, avoiding deadlock\n");
    packets[0].size -= consumed;
    if (packets[0].size < 0)
      FATAL ("get_decoded_frame: decoder consumed more data than the packet contained\n");
    if (packets[0].size) {
      packets[0].data += consumed;
    } else {
      av_free_packet (&packets[1]);
      packets[0].data = NULL;
    }
  } while (!done);

  return 0;
}

static void codec_to_meta (const AVFormatContext * format, const int stream, FLAC__StreamMetadata * meta) {
  AVCodecContext * ctx = format->streams[stream]->codec;
  int bitdepth;

  meta->data.stream_info.min_blocksize =
  meta->data.stream_info.max_blocksize =
  meta->data.stream_info.min_framesize =
  meta->data.stream_info.max_framesize = 0;
  meta->data.stream_info.sample_rate = ctx->sample_rate <= 48000 ? 48000 : 96000;
  meta->data.stream_info.channels = ctx->channels;
  if (!format->duration)
    FATAL (_f ("Container %s does not report duration\n",
	       format->iformat->name));
  if (!ctx->bits_per_raw_sample)
    WARN (_f ("Codec %s does not report bits_per_raw_sample\n",
	      ctx->codec->name));
  bitdepth = ctx->bits_per_raw_sample ? ctx->bits_per_raw_sample
                                      : ctx->bits_per_coded_sample;
  meta->data.stream_info.bits_per_sample = bitdepth <= 16 ? 16 :
                                           bitdepth <= 20 ? 20
                                                          : 24;
  meta->data.stream_info.bits_per_sample = 16;

  meta->data.stream_info.total_samples = format->duration *
    meta->data.stream_info.sample_rate /
    AV_TIME_BASE;

  bzero (meta->data.stream_info.md5sum, sizeof (meta->data.stream_info.md5sum));
}

int audiofile_audit (const char * filename, FLAC__StreamMetadata * meta) {
  int stream;
  AVFormatContext * format = NULL;
  
  stream = open_file (filename, &format);
  if (stream>=0) {
    codec_to_meta (format, stream, meta);
    avcodec_close (format->streams[stream]->codec);
  }
  if (format)
    avformat_close_input (&format);

  return stream>=0;
}

/*
 * Real C++ stuff
 */
lavReader::lavReader (const char * filename, unsigned char * buf, uint32_t size, int alignUnit)
  : lpcmReader (buf, size) {
  format = NULL;
  audioframe = NULL;
  if (!(inputconv = avresample_alloc_context ()))
    FATAL (_f ("Can't allocate avresample context\n", filename));
  packets[0].data = packets[1].data = NULL;
  packets[0].size = packets[1].size = 0;
  if (!(md5 = av_md5_alloc()))
    FATAL (_f ("Can't allocate md5 buffer for %s\n", filename));

  reset (filename, alignUnit);
}

lavReader::~lavReader () {
  close();

  av_free (md5);
  avresample_free (&inputconv);
}

void lavReader::close () {
  if (packets[0].size) {
    av_free_packet (&packets[1]);
    packets[0].data = NULL;
    packets[0].size = 0;
  }
  if (format) {
    if (stream>0)
      avcodec_close (format->streams[stream]->codec);
    avformat_close_input (&format);
  }
  if (audioframe) {
    avcodec_free_frame (&audioframe);
    if (audioframe)
      FATAL ("avcodec_free_frame does not set frame to NULL\n");
  }
    
  avresample_close (inputconv);
}

uint16_t lavReader::reset (const char * filename, int alignUnit) {
  AVCodecContext * ctx;
  int layout, samplerate, bitdepth;
  
  close();
  stream = open_file (filename, &format);

  if (stream>=0) {
    codec_to_meta (format, stream, &fmeta);
    pos.max = unread = flacHeader::bytesUncompressed( &fmeta );
    surplus = ( alignment ? unread % alignment : 0 );
    gcount = bufPos = 0;

    ctx = format->streams[stream]->codec;
    // /4 since we need an even number of samples for 20/24bit formatting
    alignment = alignUnit ? alignUnit 
                          : ctx->channels * (ctx->bits_per_raw_sample ? ctx->bits_per_raw_sample
					                              : ctx->bits_per_coded_sample) / 4;
    /* lplex compat */
    pos.max = unread = flacHeader::bytesUncompressed( &fmeta );
    surplus = ( alignment ? unread % alignment : 0 );
    gcount = bufPos = 0;
    /* lplex compat end */

    av_md5_init (md5);

    layout = ctx->channel_layout ? ctx->channel_layout
                                 : av_get_default_channel_layout (ctx->channels);
    samplerate = ctx->sample_rate <= 48000 ? 48000 : 96000;
    bitdepth = ctx->bits_per_raw_sample ? ctx->bits_per_raw_sample
                                        : ctx->bits_per_coded_sample;
    //    sampleformat = bitdepth <= 16 ? AV_SAMPLE_FMT_S16 : AV_SAMPLE_FMT_S32;
    sampleformat = AV_SAMPLE_FMT_S16;

    av_opt_set_int (inputconv, "in_channel_layout",  layout,            0);
    av_opt_set_int (inputconv, "out_channel_layout", layout,            0);
    av_opt_set_int (inputconv, "in_sample_fmt",      ctx->sample_fmt,   0);
    av_opt_set_int (inputconv, "out_sample_fmt",     sampleformat,      0);
    av_opt_set_int (inputconv, "in_sample_rate",     ctx->sample_rate,  0);
    av_opt_set_int (inputconv, "out_sample_rate",    samplerate,        0);

    if (avresample_open (inputconv))
      FATAL (_f ("Can't initialize audio resampling for %s\n", filename));

    if (soundCheck (this))
      return 0;
  }

  avresample_open (inputconv);

  return 1;
}

uint64_t lavReader::fillBuf (uint64_t limit, counter<uint64_t> *midCount) {
  uint64_t to_copy, count, i;
  uint8_t * buf, temp;
  int samplesize;

  if (midCount)
    ct = *midCount;
  else
    ct.start = ct.now = 0;
  ct.max = sizeofbigBuf / alignment * alignment;
  if (limit && limit<ct.max)
    ct.max = limit;

  gcount = 0;
  buf = (uint8_t *) bigBuf + ct.now;
  samplesize = format->streams[stream]->codec->channels *
               ((sampleformat == AV_SAMPLE_FMT_S16) ? sizeof (int16_t)
                                                    : sizeof (int32_t));
  to_copy = (ct.max - ct.now) / samplesize;
  
  do {
    if ((count = avresample_available (inputconv))) {
      // Empty the resample fifo, if applicable
      count = avresample_read (inputconv, &buf, FFMIN (to_copy, count));
    } else if (!get_decoded_frame (format, stream, &audioframe, packets)) {
      // get a decoded frame and feed it to the resampler
      count = avresample_convert (inputconv,
				  &buf, to_copy*samplesize, to_copy,
				  audioframe->extended_data, 0, audioframe->nb_samples);
    }
    to_copy -= count;
    gcount  += count;
    buf     += count * samplesize;
  } while (to_copy && count);

  gcount *= samplesize;
  if (gcount) {
    av_md5_update (md5, (uint8_t *) bigBuf + ct.now, gcount);
    ct.now += gcount;
    unread -= gcount;
  } else {
    state |= _eof;
    av_md5_final (md5, fmeta.data.stream_info.md5sum);
  }

  if (gcount) return ct.now;
  else return -1;
}
