// 用户设置
#define USER_SETUP_INFO "User_Setup"

// 选择驱动芯片，ILI9341
#define ILI9341_DRIVER

// 选择显示屏类型
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// 引脚定义
#define TFT_MISO  12   // MISO引脚
#define TFT_MOSI  13   // MOSI引脚
#define TFT_SCLK  14   // SCK引脚
#define TFT_CS    15   // 芯片选择引脚
#define TFT_DC    2    // 数据/命令选择引脚
#define TFT_RST   27   // 复位引脚
#define TFT_BL    21   // 背光控制引脚

// 设定背光控制（如果使用背光控制引脚）
#define TFT_BACKLIGHT_ON HIGH
#define TFT_BACKLIGHT_OFF LOW

// 控制SPI速度
#define SPI_FREQUENCY  40000000   // SPI频率
#define SPI_READ_FREQUENCY  20000000  // 读取SPI频率

// 启用显示屏上的字形（你可以选择加载不同的字体）
#define LOAD_GLCD   // 加载默认的8x8字符字体
#define LOAD_FONT2  // 加载16像素字体
#define LOAD_FONT4  // 加载26像素字体
#define LOAD_FONT6  // 加载48像素字体
#define LOAD_FONT7  // 加载75像素字体
#define LOAD_FONT8  // 加载更大字体

// 启用平滑字体
#define SMOOTH_FONT

// 确保不使用外部触摸屏，或者定义触摸屏的CS引脚（如果有）
#undef TOUCH_CS
// #define TOUCH_CS 21  // 如果有触摸屏，设置触摸芯片选择引脚
