// SdBrowse.h

#ifndef SDBROWSE_H
#define SDBROWSE_H

#include "Config.h"
#include "Globals.h"
#include <ResponsiveAnalogRead.h>


class SDBrowse {
public:
  void scanSdForWavFiles(const char *folder);
  void previewWavFile(const String &path);
  void sampleBrowse();
  void updatePreview();

private:

};


#endif
