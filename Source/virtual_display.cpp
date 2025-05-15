#include "virtual_display.h"
#include "font16x32.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <thread>

extern "C"
{

const static int display_width = 240;
const static int display_height = 320;

static int touch_posx = -1;
static int touch_posy = -1;

static uint16_t display[display_width * display_height];
static sf::Vertex pixels[display_width * display_height];

static void SetPixel(uint16_t x, uint16_t y, uint16_t color)
{
    display[y * display_width + x] = color;
}

void lcdFillScreen(TFT_t* device, uint16_t color)
{
    if (color == 0)
        memset(display, color, sizeof(display));
    else
        lcdDrawFillRect(device, 0, 0, 240, 320, color);
}

void lcdDrawFillRect(TFT_t* dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    for (uint16_t x = x1; x < x2; x++)
        for (uint16_t y = y1; y < y2; y++)
            SetPixel(x, y, color);
}

int lcdDrawString(TFT_t* dev, FontxFile* fx, uint16_t x, uint16_t y, uint8_t* ascii, uint16_t color)
{
    size_t n = strlen((const char*)ascii);
    for (size_t i = 0; i < n; i++)
        lcdDrawChar(dev, fx, x + i * fx->size_x, y, ascii[i], color);

    return 0;
}

int lcdDrawChar(TFT_t* dev, FontxFile* fx, uint16_t x, uint16_t y, uint8_t ascii, uint16_t color)
{
    // character is stored as 32 rows and 2 columns
    int char_index = ascii * 64;

    // size of a character in the font used in the display 
    const int virtual_font_size_x = 16;
    const int virtual_font_size_y = 32;

    // adjust for the fx font size 
    int step_row = virtual_font_size_y / fx->size_y;
    int step_col = virtual_font_size_x / fx->size_x;

    for (int row = 0; row < virtual_font_size_y; row += step_row) 
    {
        uint16_t bit_check = (console_font_16x32[char_index + row * 2] << 8) | console_font_16x32[char_index + row * 2 + 1];
        
        for (int col = 0; col < virtual_font_size_x; col += step_col)
        {
            if (bit_check & (1 << (15 - col))) 
            {
                int posx = x + col / step_col;
                int posy = y + row / step_row;

                // add the position y offset 
                posy -= fx->size_y;
                SetPixel(posx, posy, color); 
            }
        }
    }

    return 0;
}

void lcdDrawPixel(TFT_t * dev, uint16_t x, uint16_t y, uint16_t color)
{
    SetPixel(x, y, color);
}

void vTaskDelay(uint32_t ticks)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ticks));
}

bool GetFontx(FontxFile* fxs, uint8_t ascii, uint8_t* pGlyph, uint8_t* letter_width, uint8_t* letter_height)
{
    *letter_width = fxs->size_x;
    *letter_height = fxs->size_y;
    return true;
}

bool touch_getxy(TFT_t* dev, int* xp, int* yp)
{
    *xp = touch_posx;
    *yp = touch_posy;

    return touch_posx > 0 && touch_posy > 0;
}

}

static int map(int v, int in_min, int in_max, int out_min, int out_max) 
{
    return (v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void VirtualDisplayInit()
{
    // set positions and sprite sizes 
    for (int x = 0; x < display_width; x++)
    {     
        for (int y = 0; y < display_height; y++)
        {
            pixels[y * display_width + x].position = {(float)x, (float)y};
        }
    }
}

void VirtualDisplayRender(sf::RenderTarget& target)
{
    constexpr float col_multiplier = 255.f / 31;
    
    sf::Color col;

    for (int i = 0; i < display_width * display_height; i++)
    {
        uint16_t col16 = display[i];
        uint8_t r = (col16 >> 11) & 0x1f;
        uint8_t g = (col16 >> 5) & 0x3F;
        uint8_t b = col16 & 0x1f;
        col.r = r * col_multiplier;
        col.g = g * col_multiplier;
        col.b = b * col_multiplier;
        pixels[i].color = col;
    }
    
    target.draw(pixels, sizeof(pixels) / sizeof(pixels[0]), sf::Points);
}

void VirtualDisplayEventHandler(sf::Event& event, sf::WindowBase& window)
{
    static bool update_touch_pos = false;

    if (event.type == sf::Event::MouseButtonPressed)
        update_touch_pos = true;

    else if (event.type == sf::Event::MouseButtonReleased)
    {
        touch_posx = -1;
        touch_posy = -1;
        update_touch_pos = false;
    }

    if (update_touch_pos)
    {
        sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

        touch_posx = map(mouse_pos.x, 0, display_width, 300, 5500);
        touch_posy = map(mouse_pos.y, 0, display_height, 300, 6500);
    }
}
