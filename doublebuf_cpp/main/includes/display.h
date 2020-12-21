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
  int bufArea;
  int bufSize;
  
  void init(void);

  void drawHeader(std::string text);
  void drawSplash(void);
  void reset(void);

  void setClearColor(color_t color);
  void clear();

  void setDrawColor(uint8_t r, uint8_t g, uint8_t b);
  void setDrawColor(color_t color);
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

  void blit(void);
};