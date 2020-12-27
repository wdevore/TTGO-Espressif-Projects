#include <string>

extern "C"
{
#include "tftspi.h"
#include "tft.h"
}

class Display
{
public:
  color_t drawColor;
  color_t clearColor;

  int width , height;

  // length = width * height
  // uint8_t, uint8_t,...
  color_t *backBuf;
  int backArea;
  int backSize;
  
  // Display buffer
  color_t *dispBuf;
  int dispArea;
  int dispSize;
  int dispWidth , dispHeight;

  void init(void);

  void drawHeader(std::string text);
  void drawSplash(void);
  void reset(void);

  void setClearColor(color_t color);
  void clear();
  void fillDisplay();

  void setAddrWindow(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);
  void setDrawColor(uint8_t r, uint8_t g, uint8_t b);
  void setDrawColor(color_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
  void drawPixel(int16_t x, int16_t y);
  void drawSqr(int16_t x, int16_t y, int16_t w, int16_t h);

  void blit(void);
  void blit2(void);
};