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

  int width, height;

  // Color buffer: width x height
  // width
  // xxxx
  // xxxx  height
  // xxxx

  int colorBufSize;
  int colorBufArea;
  color_t *colorBuf;

  void init(void);
  void reset(void);

  int Width();
  int Height();

  void setClearColor(color_t color);
  void clear();
  void fillDisplay();

  void setDrawColor(uint8_t r, uint8_t g, uint8_t b);
  void setDrawColor(color_t color);

  // The Set methods render to the color buffer.
  // Blitting is required to move the color buffer to the display.
  void setPixel(int x, int y);
  void setRect(int x, int y, int w, int h);

  // The Draw methods render directly to the display
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
  void drawPixel(int16_t x, int16_t y);
  void drawSqr(int16_t x, int16_t y, int16_t w, int16_t h);

  void blit(void);

private:
  void setAddrWindow(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2);
};