#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// 引脚定义
#define SD_CS   4
#define TFT_CS  5
#define TFT_RST 22
#define TFT_DC  21

TFT_eSPI tft = TFT_eSPI();  // 创建TFT对象

#define MAX_FILES 20
String bmpFiles[MAX_FILES];      // 存储图片路径
int fileCount = 0;               // 图片数量
int currentIndex = 0;            // 当前显示索引
unsigned long lastSwitchTime = 0;  // 上次切换时间

// 定义你的屏幕/图片尺寸
#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320

// 全局缓冲区，避免在循环中反复分配/释放
// 根据 240x320 24位BMP图像计算每行所需的字节数（需要32位对齐）
// ((宽度 * 色深 + 31) / 32) * 4
uint8_t *sdbuffer_global;
uint16_t *pixels16_global;


// --- 工具函数 ---
uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}

// --- 显示 BMP ---
bool displayBMP(const char *filename, int x, int y) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) {
    Serial.print("ERROR: 无法打开文件: ");
    Serial.println(filename);
    return false;
  }

  // 检查 BMP 文件头
  if (read16(bmpFile) != 0x4D42) {
    Serial.println("ERROR: 不是有效的 BMP 文件 (Magic Number)");
    bmpFile.close();
    return false;
  }

  (void)read32(bmpFile);      // 跳过文件大小
  (void)read32(bmpFile);      // 跳过保留字段
  uint32_t bmpImageOffset = read32(bmpFile); // 图像数据偏移量

  (void)read32(bmpFile);          // 位图信息头大小
  int bmpWidth = read32(bmpFile); // 宽度
  int bmpHeight = read32(bmpFile); // 高度



  if (read16(bmpFile) != 1) {      // 检查平面数
    Serial.println("ERROR: BMP 平面数不正确");
    bmpFile.close();
    return false;
  }
  uint16_t bmpColors = read16(bmpFile);      // 色深
  uint32_t bmpCompression = read32(bmpFile); // 压缩方式

  if (bmpCompression != 0 || bmpColors != 24) {
    Serial.println("ERROR: 不支持的 BMP 格式 (非24位或已压缩)");
    bmpFile.close();
    return false;
  }

  // 跳转到图像数据起始位置
  bmpFile.seek(bmpImageOffset);

  // 计算每行在文件中的字节数 (24位BMP需要32位对齐)
  uint16_t rowBytes = ((bmpWidth * bmpColors + 31) / 32) * 4;

  // 逐行读取和显示
  // BMP 图像数据通常是倒序存储的（从下往上），所以我们从 bmpHeight - 1 开始
  for (int row = bmpHeight - 1; row >= 0; row--) {
    // 理论上，对于 240x320 的全屏图片，y+row 不会超出屏幕范围，但保留了条件检查
    if (y + row < 0 || y + row >= tft.height()) {
      bmpFile.seek(rowBytes, SeekCur); // 跳过这一行数据
      continue;
    }

    // 读取一整行 BMP 原始数据到全局缓冲区 sdbuffer_global
    int bytesRead = bmpFile.read(sdbuffer_global, rowBytes);
    if (bytesRead != rowBytes) {
      Serial.println("ERROR: 读取 BMP 行数据失败");
      bmpFile.close();
      return false;
    }

    uint8_t *ptr = sdbuffer_global; // 指向行数据缓冲区的起始
    for (int col = 0; col < bmpWidth; col++) {
      uint8_t b = *ptr++; // 蓝色分量
      uint8_t g = *ptr++; // 绿色分量
      uint8_t r = *ptr++; // 红色分量

      uint16_t color = tft.color565(r, g, b); // 转换为 16 位 565 格式

      // TFT_eSPI 的 pushImage 通常不需要字节交换，可以尝试移除下面这行
      // 如果移除后颜色显示不正确（例如偏绿或偏紫），请取消注释这行
      color = (color << 8) | (color >> 8); // 字节交换，根据需要启用或禁用

      pixels16_global[col] = color; // 存储转换后的 16 位颜色到全局缓冲区
    }

    // 将转换后的 16 位颜色数据推送到 TFT 屏幕上
    tft.pushImage(x, y + row, bmpWidth, 1, pixels16_global);
  }

  bmpFile.close(); // 关闭文件
  return true;
}

// --- 初始化 ---
void setup() {
  Serial.begin(115200);
  Serial.println("系统启动...");

  // 初始化 SD 卡，并尝试设置更高的 SPI 速度以加快读取（例如 40MHz）
  // 如果遇到不稳定，可以尝试降低到 20000000 (20MHz) 或 10000000 (10MHz)
  if (!SD.begin(SD_CS, SPI, 40000000)) {
    Serial.println("ERROR: SD 卡初始化失败！请检查接线和卡片。");
    while (1); // 致命错误，停止程序
  }
  Serial.println("SD 卡初始化成功。");

  // 初始化 TFT 屏幕
  tft.init();
  tft.setRotation(0); // 设置屏幕旋转方向，确保 240x320 图片能正确铺满
  tft.fillScreen(TFT_BLACK); // 首次清屏，初始化背景

  // 根据 240x320 图像尺寸，一次性分配所需的全局缓冲区
  uint16_t required_sdbuffer_size = ((DISPLAY_WIDTH * 24 + 31) / 32) * 4;
  uint16_t required_pixels16_size = DISPLAY_WIDTH * sizeof(uint16_t);

  sdbuffer_global = (uint8_t *)malloc(required_sdbuffer_size);
  pixels16_global = (uint16_t *)malloc(required_pixels16_size);

  if (!sdbuffer_global || !pixels16_global) {
    Serial.println("FATAL ERROR: 全局图像缓冲区内存分配失败！请检查堆内存。");
    while (1); // 致命错误，停止程序
  }
  Serial.println("全局图像缓冲区分配成功。");
  Serial.print("SD Buffer Size: "); Serial.print(required_sdbuffer_size); Serial.println(" bytes");
  Serial.print("Pixels16 Buffer Size: "); Serial.print(required_pixels16_size); Serial.println(" bytes");


  // 扫描 SD 卡根目录下的所有 BMP 文件
  Serial.println("开始扫描 BMP 文件...");
  File root = SD.open("/");
  while (true) {
    File file = root.openNextFile();
    if (!file) {
      // 没有更多文件了
      break;
    }
    if (!file.isDirectory()) {
      String name = file.name();
      if (name.endsWith(".bmp") || name.endsWith(".BMP")) {
        if (fileCount < MAX_FILES) {
          bmpFiles[fileCount++] = "/" + name; // 存储文件路径
          Serial.print("找到图片: ");
          Serial.println(name);
        } else {
          Serial.println("警告: 达到最大图片数量限制，忽略更多图片。");
          break; // 达到最大文件数，停止扫描
        }
      }
    }
    file.close(); // 每次使用完文件对象后关闭
  }
  root.close(); // 关闭根目录文件对象

  Serial.print("总共找到 ");
  Serial.print(fileCount);
  Serial.println(" 张 BMP 图片。");

  lastSwitchTime = millis();  // 初始化计时器
}

// --- 轮播逻辑 ---
void loop() {
  if (fileCount == 0) {
    // 如果没有图片，什么也不做
    return;
  }

  // 每 5 秒切换一次图片
  if (millis() - lastSwitchTime >= 5000) {
    // *** 优化：移除 tft.fillScreen(TFT_BLACK); ***
    // 因为图片是 240x320 且覆盖整个屏幕，新图片会直接覆盖旧图片，
    // 移除这行可以消除切换时的闪烁，让过渡更平滑。
    // tft.fillScreen(TFT_BLACK);

    const char* filename = bmpFiles[currentIndex].c_str();
    Serial.print("正在显示图片: ");
    Serial.println(filename);

    // 显示图片，x=0, y=0 表示从屏幕左上角开始
    if (!displayBMP(filename, 0, 0)) {
      Serial.println("ERROR: 图片显示失败！");
    }

    // 切换到下一张图片
    currentIndex = (currentIndex + 1) % fileCount;
    lastSwitchTime = millis(); // 更新上次切换时间
  }
}
