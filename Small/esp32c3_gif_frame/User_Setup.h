#define USER_SETUP_INFO "User_Setup"

// 드라이버 설정
#define ST7735_DRIVER

// 해상도 및 컬러 순서 설정
#define TFT_WIDTH  128
#define TFT_HEIGHT 128
#define TFT_RGB_ORDER TFT_BGR

// ST7735의 경우 화면에 노이즈가 보일 때 아래 옵션 중 하나를 주석 해제하여 테스트 (일반적으로 GREENTAB3 또는 GREENTAB2 사용)
#define ST7735_GREENTAB3
// #define ST7735_GREENTAB2
// #define ST7735_GREENTAB160x80

// ESP32-C3 1.44인치 보드 핀맵 설정
#define TFT_MOSI  4
#define TFT_SCLK  3
#define TFT_CS    2
#define TFT_DC    0
#define TFT_RST   5

// 폰트 설정 (기본 제공)
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// SPI 통신 속도 (너무 빠르면 화면이 깨질 수 있으므로 27MHz 권장)
#define SPI_FREQUENCY  27000000
