#ifndef IMAGE_SLIDESHOW_H
#define IMAGE_SLIDESHOW_H

#include <TFT_eSPI.h>
#include <SD.h>
#include <SPI.h>

#define MAX_FILES 20

class ImageSlideshow {
public:
  ImageSlideshow(TFT_eSPI& tftRef, int sdCsPin, int w, int h);
  bool begin();
  void update();

private:
  bool displayBMP(const char* filename, int x, int y);
  uint16_t read16(File &f);
  uint32_t read32(File &f);

  TFT_eSPI& tft;
  int SD_CS;
  int DISPLAY_WIDTH;
  int DISPLAY_HEIGHT;

  String bmpFiles[MAX_FILES];
  int fileCount = 0;
  int currentIndex = 0;

  unsigned long lastSwitchTime = 0;
  uint8_t* sdbuffer = nullptr;
  uint16_t* pixels16 = nullptr;
};

#endif
