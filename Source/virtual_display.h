#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define ESP_LOGI(tag, frmt, ...)    \
printf(frmt, __VA_ARGS__);          \
printf("\n");                       \

#define FontxGlyphBufSize 1 // value is unused in the virtual display so set to 1 

#if __cplusplus
namespace sf
{
    class RenderTarget;
    class Event;
    class WindowBase;
}
void VirtualDisplayInit();
void VirtualDisplayRender(sf::RenderTarget& target);
void VirtualDisplayEventHandler(sf::Event& event, sf::WindowBase& window);

extern "C"
{
#endif 

typedef struct
{
    // we don't simulate the communication
} TFT_t;

typedef struct 
{
    uint8_t size_x;
    uint8_t size_y;
} FontxFile;

// most lcd functions 
void lcdFillScreen(TFT_t* device, uint16_t color);
void lcdDrawFillRect(TFT_t * dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
int lcdDrawString(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t * ascii, uint16_t color);
int lcdDrawChar(TFT_t * dev, FontxFile *fx, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color);
void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color);

void vTaskDelay(uint32_t ticks);
bool GetFontx(FontxFile* fxs, uint8_t ascii, uint8_t* pGlyph, uint8_t* letter_width, uint8_t* letter_height);

// touch 
bool touch_getxy(TFT_t* dev, int* xp, int* yp);

#if __cplusplus
}
#endif 
