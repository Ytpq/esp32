#include "ImageSlideshow.h"

ImageSlideshow::ImageSlideshow(TFT_eSPI& tftRef, int sdCsPin, int w, int h)
  : tft(tftRef), SD_CS(sdCsPin), DISPLAY_WIDTH(w), DISPLAY_HEIGHT(h) {}

bool ImageSlideshow::begin() {
  Serial.println("系统启动...");

  if (!SD.begin(SD_CS, SPI, 40000000)) {
    Serial.println("ERROR: SD 卡初始化失败！");
    return false;
  }

  Serial.println("SD 卡初始化成功。");

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  uint16_t sdbuffer_size = ((DISPLAY_WIDTH * 24 + 31) / 32) * 4;
  uint16_t pixels16_size = DISPLAY_WIDTH * sizeof(uint16_t);
  sdbuffer = (uint8_t*)malloc(sdbuffer_size);
  pixels16 = (uint16_t*)malloc(pixels16_size);

  if (!sdbuffer || !pixels16) {
    Serial.println("内存分配失败！");
    return false;
  }

  File root = SD.open("/");
  while (true) {
    File file = root.openNextFile();
    if (!file) break;
    if (!file.isDirectory()) {
      String name = file.name();
      if (name.endsWith(".bmp") || name.endsWith(".BMP")) {
        if (fileCount < MAX_FILES) {
          bmpFiles[fileCount++] = "/" + name;
        }
      }
    }
    file.close();
  }
  root.close();
  lastSwitchTime = millis();

  Serial.print("共找到 ");
  Serial.print(fileCount);
  Serial.println(" 张 BMP 图片。");

  return true;
}

  // 第一次显示或达到切换时间时显示图片
void ImageSlideshow::update() {
    if (fileCount == 0) return;

    static bool firstDisplay = true;
    static int lastDisplayedIndex = -1;

    // 检查是否需要切换到下一张
    bool needSwitch = (millis() - lastSwitchTime >= 5000) || firstDisplay;
    
    if (needSwitch || lastDisplayedIndex != currentIndex) {
        const char* filename = bmpFiles[currentIndex].c_str();
        Serial.print("显示图片: ");
        Serial.println(filename);
        
        if (displayBMP(filename, 0, 0)) {
            lastDisplayedIndex = currentIndex;
            firstDisplay = false;
            
            // 如果是时间到了才切换
            if (millis() - lastSwitchTime >= 5000) {
                currentIndex = (currentIndex + 1) % fileCount;
                lastSwitchTime = millis();
            }
        } else {
            Serial.println("ERROR: 图片显示失败！");
        }
    }
}

uint16_t ImageSlideshow::read16(File &f) {
  uint16_t result;
  ((uint8_t*)&result)[0] = f.read();
  ((uint8_t*)&result)[1] = f.read();
  return result;
}

uint32_t ImageSlideshow::read32(File &f) {
  uint32_t result;
  ((uint8_t*)&result)[0] = f.read();
  ((uint8_t*)&result)[1] = f.read();
  ((uint8_t*)&result)[2] = f.read();
  ((uint8_t*)&result)[3] = f.read();
  return result;
}

bool ImageSlideshow::displayBMP(const char* filename, int x, int y) {
  File bmpFile = SD.open(filename);
  if (!bmpFile || read16(bmpFile) != 0x4D42) {
    bmpFile.close();
    return false;
  }

  (void)read32(bmpFile);
  (void)read32(bmpFile);
  uint32_t bmpImageOffset = read32(bmpFile);
  (void)read32(bmpFile);
  int bmpWidth = read32(bmpFile);
  int bmpHeight = read32(bmpFile);

  if (read16(bmpFile) != 1 || read16(bmpFile) != 24 || read32(bmpFile) != 0) {
    bmpFile.close();
    return false;
  }

  bmpFile.seek(bmpImageOffset);
  uint16_t rowBytes = ((bmpWidth * 24 + 31) / 32) * 4;

  for (int row = bmpHeight - 1; row >= 0; row--) {
    if (y + row < 0 || y + row >= tft.height()) {
      bmpFile.seek(rowBytes, SeekCur);
      continue;
    }

    int bytesRead = bmpFile.read(sdbuffer, rowBytes);
    if (bytesRead != rowBytes) {
      bmpFile.close();
      return false;
    }

    uint8_t* ptr = sdbuffer;
    for (int col = 0; col < bmpWidth; col++) {
      uint8_t b = *ptr++;
      uint8_t g = *ptr++;
      uint8_t r = *ptr++;
      uint16_t color = tft.color565(r, g, b);
      color = (color << 8) | (color >> 8);
      pixels16[col] = color;
    }

    tft.pushImage(x, y + row, bmpWidth, 1, pixels16);
  }

  bmpFile.close();
  return true;
}
