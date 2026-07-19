#define USER_SETUP_LOADED

// 디스플레이 칩셋 드라이버 설정
#define ILI9341_DRIVER

// ⭐ [중요] 색상 표현 방식 정의
// ILI9341 칩셋 중 일부는 RGB 순서가 아닌 BGR 순서를 사용합니다.
// 이 코드를 추가하여 색상을 정상적으로 돌립니다.
#define TFT_RGB_ORDER TFT_BGR

// CYD 보드 전용 핀 맵 설정
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1
#define TFT_BL   21  // 백라이트 핀

// 폰트 및 기타 설정 (필요 시 수정)
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  55000000
#define SPI_READ_FREQUENCY  20000000
