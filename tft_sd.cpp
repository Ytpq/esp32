#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// 定义SD卡和TFT屏幕的CS、RST、DC引脚
#define SD_CS   4   // SD 卡片选引脚
#define TFT_CS   5   // TFT 屏幕片选引脚
#define TFT_RST  22  // TFT 屏幕复位引脚
#define TFT_DC   21  // TFT 屏幕数据/命令引脚

// 创建 TFT 对象
TFT_eSPI tft = TFT_eSPI();

// 从文件读取16位数据（小端模式）
uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();   // LSB (最低有效字节)
  ((uint8_t *)&result)[1] = f.read();   // MSB (最高有效字节)
  return result;
}

// 从文件读取32位数据（小端模式）
uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();   // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();   // MSB
  return result;
}

// 显示 BMP 图片函数
bool displayBMP(const char *filename, int x, int y) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) return false;  // 错误：文件未找到

  // 读取 BMP 文件头 (14 字节)
  if (read16(bmpFile) != 0x4D42) {   // 检查文件类型标识符 "BM" (0x424D)
    bmpFile.close();
    return false;
  }

  (void)read32(bmpFile);   // 文件大小，忽略
  (void)read32(bmpFile);   // 保留字段，忽略
  uint32_t bmpImageOffset = read32(bmpFile);   // 图像数据偏移

  // 读取位图信息头 (40 字节)
  (void)read32(bmpFile);   // 位图信息头大小，忽略
  int bmpWidth = read32(bmpFile);   // 图像宽度
  int bmpHeight = read32(bmpFile);   // 图像高度
  if (read16(bmpFile) != 1) {   // 检查平面数，应为 1
    bmpFile.close();
    return false;
  }
  uint16_t bmpColors = read16(bmpFile);   // 像素位数（16 或 24）
  uint32_t bmpCompression = read32(bmpFile);   // 压缩方式

  // 检查压缩方式，只支持未压缩的BMP (BI_RGB = 0)
  if (bmpCompression != 0) {
    bmpFile.close();
    return false;
  }

  // 检查颜色深度，仅支持 24 位 BMP 图像
  if (bmpColors != 24) {
    bmpFile.close();
    return false;
  }

  // 跳转到图像数据偏移处
  bmpFile.seek(bmpImageOffset);

  // 计算每行字节数 (BMP行数据需要4字节对齐)
  uint16_t rowBytes = ((bmpWidth * bmpColors + 31) / 32) * 4;

  // 分配内存用于存储一行BMP像素数据
  uint8_t *sdbuffer = (uint8_t *)malloc(rowBytes);
  if (!sdbuffer) {
    bmpFile.close();
    return false;
  }

  // TFT_eSPI 的 pushImage 函数需要 RGB565 格式的像素数组
  uint16_t *pixels16 = (uint16_t *)malloc(bmpWidth * sizeof(uint16_t));
  if (!pixels16) {
    free(sdbuffer);
    bmpFile.close();
    return false;
  }

  // BMP 图片通常是自底向上存储，因此从最后一行开始读取并显示
  for (int row = bmpHeight - 1; row >= 0; row--) {
    // 检查当前行是否在屏幕显示范围内
    if (y + row < 0 || y + row >= tft.height()) {
      bmpFile.seek(rowBytes, SeekCur);   // 跳过这一行，不进行显示
      continue;
    }

    // 从SD卡读取一行像素数据到缓冲区
    int bytesRead = bmpFile.read(sdbuffer, rowBytes);
    if (bytesRead != rowBytes) {
      bmpFile.close();
      free(sdbuffer);
      free(pixels16);
      return false;  // 数据读取错误
    }

    uint8_t *ptr = sdbuffer;   // 指向当前行的起始像素

    for (int col = 0; col < bmpWidth; col++) {
      uint8_t b_bmp = *ptr++; // 读取蓝色分量
      uint8_t g_bmp = *ptr++; // 读取绿色分量
      uint8_t r_bmp = *ptr++; // 读取红色分量

      uint16_t color = tft.color565(r_bmp, g_bmp, b_bmp); // 转换为 RGB565 格式

      // !!! 关键修改：尝试字节交换 !!!
      color = (color << 8) | (color >> 8); // 交换高低字节

      pixels16[col] = color;   // 存储转换后的颜色
    }

    // 使用 pushImage 函数将转换后的 RGB565 像素行显示到TFT屏幕上
    tft.pushImage(x, y + row, bmpWidth, 1, pixels16);
  }

  free(sdbuffer);   // 释放行缓冲区内存
  free(pixels16);   // 释放转换后的像素缓冲区内存
  bmpFile.close();   // 关闭 BMP 文件
  return true;
}

void setup() {
  // 初始化串口波特率
  Serial.begin(115200); 
  if (!SD.begin(SD_CS, SPI)) {
    while (1);   // SD 卡初始化失败时停止
  }

  // 初始化 TFT 屏幕
  tft.init();
  tft.setRotation(0);  
  tft.fillScreen(TFT_BLACK);  

  // 尝试读取并显示图片
  const char *imagePath = "/1.bmp";  // BMP 文件路径
  if (!displayBMP(imagePath, 0, 0)) {
    Serial.println("Failed to display image from SD card.");
  }
}

void loop() {
  // 循环部分留空，或者可以添加其他逻辑
}
