#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

// 引脚定义
#define SD_CS 4
#define TFT_CS 5
#define TFT_RST 22
#define TFT_DC 21
#define button 25

TFT_eSPI tft = TFT_eSPI(); // 创建TFT对象

#define MAX_FILES 20
String bmpFiles[MAX_FILES]; // 存储图片路径
int fileCount = 0; // 图片数量
int currentIndex = 0; // 当前显示索引

// 定义你的屏幕/图片尺寸
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320

// 全局标志，指示是否需要重新绘制图片
bool needsRedraw = true; // 初始为 true，以便在启动时绘制第一张图片

// --- 按钮去抖和状态追踪变量 ---
unsigned long lastDebounceTime = 0;  // 上次按钮状态变化的时间
const unsigned long debounceDelay = 50; // 去抖动延迟，50毫秒
int buttonState;                  // 按钮的当前读取状态
int lastButtonState = HIGH;       // 按钮的上一个状态（初始为未按下）
bool buttonPressedFlag = false;   // 标记按钮是否被有效按下（用于单次触发）

void drawCurrentPage() {
  if (fileCount == 0) {
    Serial.println("没有找到图片，无法显示。");
    return;
  }
  const char* filename = bmpFiles[currentIndex].c_str();
  Serial.print("正在显示图片: ");
  Serial.println(filename);

  if (!displayBMP(filename, 0, 0)) {
    Serial.println("ERROR: 图片显示失败！");
    // 如果图片显示失败，可以选择在这里尝试加载下一张图片或做其他错误处理
    // 例如：currentIndex = (currentIndex + 1) % fileCount;
    //       needsRedraw = true; // 强制尝试下一张
  }
}

void checkButtons() {
  // 读取当前按钮的状态
  int reading = digitalRead(button);

  // 如果按钮状态发生了变化（从高到低或从低到高）
  if (reading != lastButtonState) {
    // 重置去抖定时器
    lastDebounceTime = millis();
  }

  // 检查是否过了去抖动时间
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // 只有当按钮状态稳定下来（通过去抖动）并且与当前已确认的状态不同时，才更新状态
    if (reading != buttonState) {
      buttonState = reading;

      // 如果按钮状态变为 LOW（被按下）并且之前没有标记为已按下
      if (buttonState == LOW && !buttonPressedFlag) {
        // 标记按钮为已按下，准备触发一次动作
        buttonPressedFlag = true;

        // 执行按钮按下时的操作（切换图片）
        if (fileCount > 0) { // 确保有图片才能切换
          currentIndex = (currentIndex + 1) % fileCount;
          needsRedraw = true; // 标记需要重新绘制
          Serial.print("按钮按下，切换到图片索引: ");
          Serial.println(currentIndex);
        }
      } 
      // 如果按钮状态变为 HIGH（被释放）
      else if (buttonState == HIGH) {
        // 重置按钮按下标志，以便下次按下时可以再次触发
        buttonPressedFlag = false;
      }
    }
  }
  // 保存当前读取到的状态，用于下一次循环判断是否发生变化
  lastButtonState = reading;
}


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

  (void)read32(bmpFile); // 跳过文件大小
  (void)read32(bmpFile); // 跳过保留字段
  uint32_t bmpImageOffset = read32(bmpFile); // 图像数据偏移量

  (void)read32(bmpFile); // 位图信息头大小
  int bmpWidth = read32(bmpFile); // 宽度
  int bmpHeight = read32(bmpFile); // 高度



  if (read16(bmpFile) != 1) { // 检查平面数
    Serial.println("ERROR: BMP 平面数不正确");
    bmpFile.close();
    return false;
  }
  uint16_t bmpColors = read16(bmpFile); // 色深
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
  pinMode(button, INPUT_PULLUP); // 假设按钮一端接地，使用上拉输入


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
}

// --- 轮播逻辑 ---
void loop() {
  // 首先检查按钮状态（现在是非阻塞的）
  checkButtons();

  // 只有当需要重新绘制时才执行图片显示逻辑
  if (needsRedraw) {
    if (fileCount == 0) {
      Serial.println("没有找到任何图片，无法显示。");
    } else {
      drawCurrentPage(); // 调用绘制当前图片的函数
    }
    needsRedraw = false; // 图片已绘制，重置标志
  }

  // 这里可以添加其他非阻塞任务，例如：
  // checkSensorData();
  // updateWiFiStatus();

  // 保持 loop 循环运行，同时避免在没有操作时占用过多CPU
  // 对于大部分 ESP32 应用，loop里不放delay(10)也行，但放一个短delay有助于降低功耗，
  // 也可以让其他 FreeRTOS 任务（如果ESP32跑了RTOS）有机会运行。
  delay(1); 
}
