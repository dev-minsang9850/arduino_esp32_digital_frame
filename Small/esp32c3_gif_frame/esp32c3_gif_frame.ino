#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <AnimatedGIF.h>
#include "webapp.h"

// 객체 생성
WebServer server(80);
TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

// 핫스팟 접속 정보 (사용자 변경 가능)
const char* AP_SSID = "rainbow";
const char* AP_PASS = "intelailab";

// 전역 상태 변수
bool hasNewFile = false;
String currentFilename = "";

// ==========================================
// TJpg_Decoder 렌더링 콜백 (JPEG 전용)
// ==========================================
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height() || x >= tft.width()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// ==========================================
// AnimatedGIF 파일 시스템 콜백 (메모리 절약)
// ==========================================
void * GIFOpenFile(const char *fname, int32_t *pSize) {
  File *f = new File;
  *f = SPIFFS.open(fname, "r");
  if (*f) {
    *pSize = f->size();
    return (void *)f;
  }
  delete f;
  return NULL;
}

void GIFCloseFile(void *pHandle) {
  File *f = (File *)pHandle;
  if (f) {
    f->close();
    delete f;
  }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead = iLen;
  File *f = (File *)pFile->fHandle;
  if (!f) return 0;
  iBytesRead = f->read(pBuf, iLen);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *f = (File *)pFile->fHandle;
  if (!f) return 0;
  pFile->iPos = iPosition;
  return f->seek(iPosition);
}

// ==========================================
// AnimatedGIF 렌더링 콜백
// ==========================================
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > TFT_WIDTH) iWidth = TFT_WIDTH;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // 현재 라인 위치

  if (y >= TFT_HEIGHT || pDraw->iX >= TFT_WIDTH || iWidth < 1) return;

  s = pDraw->pPixels;
  // 투명도 처리 방식 2 (백그라운드 색상으로 복원)
  if (pDraw->ucDisposalMethod == 2) {
    for (x=0; x<iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // 투명 픽셀이 있는 경우
  if (pDraw->ucHasTransparency) {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0;
    while(x < iWidth) {
      c = ucTransparent-1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) s--;
        else {
          *d++ = usPalette[c];
          iCount++;
        }
      }
      if (iCount) {
        tft.setWindow(pDraw->iX+x, y, pDraw->iX+x+iCount-1, y);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) x++;
        else s--;
      }
    }
  } else {
    // 투명 픽셀이 없는 경우 (최고 속도 렌더링)
    s = pDraw->pPixels;
    for (x=0; x<iWidth; x++) usTemp[x] = usPalette[*s++];
    tft.setWindow(pDraw->iX, y, pDraw->iX+iWidth-1, y);
    tft.pushPixels(usTemp, iWidth);
  }
}

// ==========================================
// 메인 셋업
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // 1. 디스플레이 초기화
  tft.init();
  tft.invertDisplay(0); // Spotpear 보드의 경우 이 설정이 화면 색상 반전을 해결합니다.
  tft.setRotation(2);   // Spotpear 보드의 기본 방향
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Loading System...", 64, 64, 2);

  // 2. 내부 파일 시스템 마운트
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("SPIFFS ERROR", 64, 64, 2);
    return;
  }

  // 3. AP 핫스팟 모드 활성화
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // 화면에 접속 정보 출력
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.drawString("Wi-Fi:", 64, 30, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(AP_SSID, 64, 50, 2);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
  tft.drawString("IP Address:", 64, 80, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(IP.toString(), 64, 100, 2);
  
  // 4. JPEG 및 GIF 디코더 초기화
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true); // TFT_eSPI는 BGR/RGB 바이트 스왑 필요
  TJpgDec.setCallback(tft_output);
  
  gif.begin(LITTLE_ENDIAN_PIXELS);

  // 5. 웹 서버 라우팅 설정
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", webapp_html);
  });

  // 파일 업로드 처리
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "Upload Complete");
  }, []() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Receiving: %s\n", upload.filename.c_str());
      currentFilename = "/" + upload.filename;
      uploadFile = SPIFFS.open(currentFilename, "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) {
        uploadFile.write(upload.buf, upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
      }
      Serial.printf("Upload Success: %u bytes\n", upload.totalSize);
      hasNewFile = true; // 루프에서 재생 시작 플래그 켜기
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

// ==========================================
// 메인 루프
// ==========================================
void loop() {
  server.handleClient(); // 웹 서버 클라이언트 요청 처리 유지
  
  static String activeFile = "";
  
  // 새 파일이 업로드 된 경우
  if (hasNewFile) {
    activeFile = currentFilename;
    hasNewFile = false;
    tft.fillScreen(TFT_BLACK);
  }

  // 화면 표출 로직
  if (activeFile != "") {
    String lowerName = activeFile;
    lowerName.toLowerCase();
    
    // 1. JPEG 표출
    if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) {
      TJpgDec.drawFsJpg(0, 0, activeFile);
      
      // 다음 파일이 올 때까지 대기
      while(!hasNewFile) {
        server.handleClient();
        delay(10);
      }
    } 
    // 2. GIF 표출 (루프)
    else if (lowerName.endsWith(".gif")) {
      File f = SPIFFS.open(activeFile, "r");
      if (f) {
        size_t size = f.size();
        uint8_t *gif_buf = (uint8_t*)malloc(size);
        
        if (gif_buf) {
          // 메모리가 충분하면 RAM에 전체 GIF를 올려서 재생 (SPIFFS 충돌 방지)
          f.read(gif_buf, size);
          f.close();
          
          if (gif.open(gif_buf, size, GIFDraw)) {
            while(!hasNewFile) {
              int t = 0;
              while (gif.playFrame(true, &t)) {
                server.handleClient(); // RAM 재생이므로 웹 요청 받아도 안전함
                if(hasNewFile) break;
              }
              gif.reset();
            }
            gif.close();
          }
          free(gif_buf);
          
        } else {
          // 메모리가 부족하면 어쩔 수 없이 파일 스트리밍 사용 
          // 단, 스트리밍 중에는 SPIFFS 충돌을 막기 위해 웹 서버를 잠시 멈춤
          f.close();
          if (gif.open(activeFile.c_str(), GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
            while(!hasNewFile) {
              int t = 0;
              while (gif.playFrame(true, &t)) {
                // SPIFFS 동시 접근 에러(Store access fault) 방지를 위해 서버 처리 생략
                delay(1);
              }
              gif.reset();
              // GIF 재생이 한 바퀴 끝났을 때만 잠깐 서버 요청 처리
              for (int i=0; i<10; i++) {
                server.handleClient();
                delay(10);
                if(hasNewFile) break;
              }
            }
            gif.close();
          }
        }
      }
    }
  }
}