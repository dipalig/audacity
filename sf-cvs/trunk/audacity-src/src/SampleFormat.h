/**********************************************************************

  Audacity: A Digital Audio Editor

  SampleFormat.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_SAMPLE_FORMAT__
#define __AUDACITY_SAMPLE_FORMAT__

//
// Definitions / Meta-Information
//

typedef enum {
   int16Sample = 0x00020001,
   int24Sample = 0x00040001,
   floatSample = 0x0004000F
} sampleFormat;

#define SAMPLE_SIZE(SampleFormat) (SampleFormat >> 16)

typedef char *samplePtr;

const char *GetSampleFormatStr(sampleFormat format);

//
// Allocating/Freeing Samples
//

samplePtr NewSamples(int count, sampleFormat format);
void      DeleteSamples(samplePtr p);

//
// Copying, Converting and Clearing Samples
//

void      CopySamples(samplePtr src, sampleFormat srcFormat,
                      samplePtr dst, sampleFormat dstFormat,
                      unsigned int len, bool highQuality=true,
                      unsigned int stride=1);

void      ClearSamples(samplePtr buffer, sampleFormat format,
                       int start, int len);

//
// This must be called on startup and everytime new ditherers
// are set in preferences.
//

void      InitDitherers();

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 89cae057-838e-417c-b312-16dca9c38f43

