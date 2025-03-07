// FlashStorageHandler.h

#ifndef FLASH_STORAGE_HANDLER_H
#define FLASH_STORAGE_HANDLER_H

#include "Config.h"
bool doSdToFlashTransfer(const String &sdWavPath, int fsrIndex);
void continueCopyingToFlash(SerialFlashFile &file, unsigned long fileSize);
#endif
