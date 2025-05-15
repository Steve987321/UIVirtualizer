#pragma once 

// #TODO: DOCUMENTATION

#include <stdint.h> 
#include <stdbool.h>

#ifndef VIRTUAL_DISPLAY
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "lcd_com.h"
#include "lcd_lib.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_jpeg.h"
#include "decode_png.h"
#include "pngle.h"
#endif 

#define BUTTON_COUNT 8
#define SLIDER_COUNT 8
#define TEXT_COUNT 8
#define WIDGET_MAX BUTTON_COUNT + SLIDER_COUNT + TEXT_COUNT
#define WIDGET_GROUP_MAX 5

// callback signatures 
typedef void (*FBUTTON_CALLBACK)(int);

typedef struct
{
    int x;
    int y;
} Vec2;

// a += b
// void AddVec2(Vec2* a, Vec2* b);

typedef enum
{
    NONE,
    TEXT,
    SLIDER,
    BUTTON,
} WIDGET_TYPE;

typedef struct
{
    uint8_t x;
    uint8_t y;
} Vec2u8;

typedef struct
{
    Vec2 cursor_pos;
    Vec2 previous_cursor_pos;
    Vec2 previous_widget_size;
    bool sameline;
    
    // whether to immediately draw widgets upon calling them 
    bool draw_widgets;
} UIContext;

typedef struct
{
    uint16_t bg_color;
    uint16_t button_color;
    uint16_t text_color;
    uint16_t slider_bg_color;
    uint16_t slider_drag_color;
    uint8_t button_padding;
    uint8_t slider_height;
    uint8_t slider_drag_width;
    uint8_t padding;
    Vec2u8 spacing;
    FontxFile* font;
    uint8_t char_size_x;
    uint8_t char_size_y;
} UIStyle;

typedef struct
{
    int id;
    Vec2 position;
    Vec2 size;
    const char* label;
    FontxFile* font_used;
} UITextInfo;

typedef struct
{
    FBUTTON_CALLBACK callback;
    int id;
    Vec2 position;
    Vec2 size;
    const char* label;
    FontxFile* font_used;
} UIButtonInfo;

typedef struct 
{
    int id;
    int percent;
    Vec2 position; // widget, slider + label
    Vec2 slider_pos; // slider widget
    Vec2 size;
    int* val;
    int min;
    int max;
    char label[16];
    const char* format;
    FontxFile* font_used;
} UISliderInfo;

typedef struct
{
    UISliderInfo sliders[SLIDER_COUNT];
    UIButtonInfo buttons[BUTTON_COUNT];
    UITextInfo texts[TEXT_COUNT];
    uint8_t sliders_size;
    uint8_t buttons_size;
    uint8_t text_size;

    WIDGET_TYPE widget_order[WIDGET_MAX];
    uint8_t widget_order_size;
} UIWidgetGroup;

// RRRRRGGGGGGBBBBB
uint16_t GetColor(uint8_t r, uint8_t g, uint8_t b);

void UISetNextWidgetPos(Vec2 pos);

void UIInit(TFT_t* display, UIStyle* ui_style);

void UIBeginGroup();

void UIEndGroup();

void UIWidgetGroupDraw();

void UISetGroup(int index);

void UISameline();

void UIText(const char* label);

void UIButton(const char* label, FBUTTON_CALLBACK press_callback, Vec2 button_size);

void UISlider(const char* label, int* val, int min, int max);

void UIColorPicker(const char* label, int color[3]);

void UIUpdate();
