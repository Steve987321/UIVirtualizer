#ifdef VIRTUAL_DISPLAY
#include "virtual_display.h"
#endif 

#include "ui_widgets.h"
#include <stdio.h>

#define SLEEP(TICKS) vTaskDelay(TICKS)

const static uint8_t display_width = 240;
const static uint16_t display_height = 320;

// set device
static TFT_t* tft_dev;
static UIContext context;
static UIStyle* style;

static int button_pointer = 0;
static int slider_pointer = 0;
static int text_pointer = 0;
static int widget_order_pointer = 0;

static int widget_group_pointer = 0;
static UIWidgetGroup widget_groups[WIDGET_GROUP_MAX];

static UIWidgetGroup* current_widget_group;

// https://docs.arduino.cc/language-reference/en/functions/math/map/
static int map(int v, int in_min, int in_max, int out_min, int out_max) 
{
    return (v - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static int clamp(int v, int min, int max)
{
    if (v < min)
        return min;
    if (v > max)
        return max;

    return v;
}

static bool IsPointInRect(int x, int y, Vec2* rect_pos, Vec2* rect_size)
{
    return x >= rect_pos->x && x <= rect_pos->x + rect_size->x &&
           y >= rect_pos->y && y <= rect_pos->y + rect_size->y;
}

// Translates touch coordinates to ui pixel coordinates
static void TranslateCoordinates(int* x, int* y)
{
    // #TODO: changed the y clamp max from 240 to display_height
    // check if it still works
    *x = clamp(map(*x, 300, 5500, 0, display_width), 0, display_width);
    *y = clamp(map(*y, 300, 6500, 0, display_height), 0, display_height);
}

static uint16_t ColorFromFlt3(float col[3])
{
    return 0;
}

static void DrawString(const char* label, int x, int y)
{
    lcdDrawString(tft_dev, style->font, x, y, (uint8_t*)label, style->text_color);
}

static void GetTextSize(const char* txt, int* x, int* y)
{
    uint8_t buffer[FontxGlyphBufSize];
	uint8_t letter_width = 0;
	uint8_t letter_height = 0;
	GetFontx(style->font, 0, buffer, &letter_width, &letter_height);

    if (txt)
    {
        size_t n = strlen(txt);
        *x = letter_width * n;
    }

    *y = letter_height;
}

static void UpdateCursorPos(Vec2* add)
{
    context.previous_cursor_pos = context.cursor_pos;
    context.previous_widget_size = *add;
    
    if (!context.sameline)
    {
        context.cursor_pos.y += add->y;
        context.cursor_pos.y += style->spacing.y;
    }
    
    context.sameline = false;
}

static void UIUpdateSlider(UISliderInfo* slider, int press_x)
{
    // clear old grabber
    Vec2 drag_pos = slider->slider_pos;
    drag_pos.x = map(*slider->val, slider->min, slider->max, slider->position.x, slider->size.x);
    
    lcdDrawFillRect(tft_dev, 
        drag_pos.x, drag_pos.y, 
        drag_pos.x + style->slider_drag_width, drag_pos.y + style->slider_height, 
        style->slider_bg_color);

    *slider->val = map(press_x, slider->position.x, slider->size.x, slider->min, slider->max);
    *slider->val = clamp(*slider->val, slider->min, slider->max);

    // clear label 
    int label_size_x = 0;
    int label_size_y = 0;
    GetTextSize(slider->label, &label_size_x, &label_size_y);
    lcdDrawFillRect(tft_dev, 
        slider->position.x, slider->position.y + 2,
        slider->position.x + label_size_x, slider->position.y + label_size_y - 1,
        style->bg_color);

    snprintf(slider->label, 16, slider->format, *slider->val);
    DrawString(slider->label, slider->position.x, slider->position.y + label_size_y);

    ESP_LOGI(__FUNCTION__, "%d val:%d", drag_pos.x, *slider->val);

    // draw with new 
    drag_pos = slider->slider_pos;
    drag_pos.x = map(*slider->val, slider->min, slider->max, slider->position.x, slider->size.x);
    lcdDrawFillRect(tft_dev, 
    drag_pos.x, drag_pos.y, 
    drag_pos.x + style->slider_drag_width, drag_pos.y + style->slider_height, 
    style->slider_drag_color);
}

static void ResetContext(UIContext* context)
{
    context->cursor_pos.x = 0;
    context->cursor_pos.y = 0;
    context->previous_cursor_pos.x = 0;
    context->previous_cursor_pos.y = 0;
    context->previous_widget_size.x = 0;
    context->previous_widget_size.y = 0;
    context->draw_widgets = false;
    context->sameline = false;
}

uint16_t GetColor(uint8_t r, uint8_t g, uint8_t b)
{
    r &= 0x1F;
    g &= 0x3F;
    b &= 0x1F;

    return (r << 11) | (g << 5) | (b);
}

void UISetNextWidgetPos(Vec2 pos)
{
    context.previous_cursor_pos = context.cursor_pos;
    context.cursor_pos = pos;
}

void UIInit(TFT_t *dev, UIStyle *ui_style)
{
    tft_dev = dev;

    style = ui_style;
    
    context.cursor_pos.x = 0;
    context.cursor_pos.y = 0;

    context.draw_widgets = false;

    lcdFillScreen(tft_dev, style->bg_color);
}

void UIBeginGroup()
{
    if (widget_group_pointer > WIDGET_GROUP_MAX)
        return;

    button_pointer = 0;
    slider_pointer = 0;
    current_widget_group = &widget_groups[widget_group_pointer];
}

void UIEndGroup()
{
    current_widget_group->buttons_size = button_pointer;
    current_widget_group->sliders_size = slider_pointer;
    current_widget_group->text_size = text_pointer;
    current_widget_group->widget_order_size = button_pointer + slider_pointer + text_pointer;

    ESP_LOGI("UI", "EndGroup widget count: buttons:%d sliders:%d texts:%d", button_pointer, slider_pointer, text_pointer);
    
    widget_group_pointer++;

    button_pointer = 0;
    slider_pointer = 0;
    text_pointer = 0;
    widget_order_pointer = 0;

    context.cursor_pos = (Vec2){0, 0};
    context.previous_cursor_pos = (Vec2){0, 0};
}

void UIWidgetGroupDraw()
{
    lcdFillScreen(tft_dev, 1);
    
    ESP_LOGI("UI", "Drawing group, widgets %d", current_widget_group->widget_order_size);
    
    ResetContext(&context);
    context.draw_widgets = true;
    int button_index = 0;
    int slider_index = 0;
    int text_index = 0;

    widget_order_pointer = 0;
    button_pointer = 0;
    slider_pointer = 0;
    text_pointer = 0;

    UIWidgetGroup widget_group = *current_widget_group;

    for (int i = 0; i < widget_group.widget_order_size; i++)
    {
        WIDGET_TYPE w = widget_group.widget_order[i];
        ESP_LOGI("UI", "%d WIDGETTYPE: %d", i, w);
        switch(w)
        {
        case BUTTON:
        {
            UIButtonInfo* button = &widget_group.buttons[button_index];
            style->font = button->font_used;
            button_index++;
            UISetNextWidgetPos(button->position);
            UIButton(button->label, button->callback, button->size);
            ESP_LOGI("UI", "button draw: index: %d, label: %s", button_index, button->label);
            break;
        }
        case SLIDER:
        {  
            UISliderInfo* slider = &widget_group.sliders[slider_index];
            style->font = slider->font_used;
            slider_index++;
            UISetNextWidgetPos(slider->position);
            UISlider(slider->format, slider->val, slider->min, slider->max);
            ESP_LOGI("UI", "slider draw: index: %d, label: %s posy: %d", slider_index, slider->label, slider->position.y);
            break;
        }
        case TEXT:
        {
            UITextInfo* text = &widget_group.texts[text_index];
            style->font = text->font_used;
            text_index++;
            UISetNextWidgetPos(text->position);
            UIText(text->label);
            ESP_LOGI("UI", "text draw: index: %d, label: %s", text_index, text->label);
            break;
        }
        default:
            break;
        }
    }

    context.draw_widgets = false;
}

void UISetGroup(int index)
{
    current_widget_group = &widget_groups[index];
}

void UISameline()
{
    context.sameline = true;
}

void UIText(const char* label)
{
    if (text_pointer > TEXT_COUNT)
    {
        ESP_LOGI(__FUNCTION__, "Max text reached: %d", text_pointer);
        return;
    }
    
    if (!current_widget_group)
    {
        ESP_LOGI(__FUNCTION__, "No widget group to add text to. Text: %s", label);
        return;
    }
    
    UITextInfo text;
    text.label = label;
    text.position = context.cursor_pos;
    text.font_used = style->font;
    
    GetTextSize(label, &text.size.x, &text.size.y);
    
    if (context.sameline)
    {
        // from the position of the previous widget, place it next to it
        text.position = context.previous_cursor_pos;
        text.position.x += context.previous_widget_size.x + style->spacing.x;
    }
    
    if (context.draw_widgets)
        DrawString(label, text.position.x, text.position.y + text.size.y);
        
     current_widget_group->texts[text_pointer] = text;
     text_pointer++;

     current_widget_group->widget_order[widget_order_pointer] = TEXT;
     widget_order_pointer++;

     UpdateCursorPos(&text.size);
}

void UIButton(const char* label, FBUTTON_CALLBACK press_callback, Vec2 button_size)
{
    if (button_pointer >= BUTTON_COUNT)
    {
        ESP_LOGI("UIButton", "Max buttons reached: %d", button_pointer);
        return;
    }

    if (!current_widget_group)
    {
        ESP_LOGI("UIButton", "No widget group to add button to. Button: %s", label);
        return;
    }

    UIButtonInfo button;
    button.label = label;
    button.callback = press_callback;
    button.position = context.cursor_pos;
    button.font_used = style->font;

    if (context.sameline)
    {
        button.position = context.previous_cursor_pos;
        button.position.x += context.previous_widget_size.x + style->spacing.x;
    }

    ESP_LOGI(__FUNCTION__, "cursor pos %d %d", context.cursor_pos.x, context.cursor_pos.y);
    
    if (button_size.x == 0 || button_size.y == 0)
    {
        // use label to set frame size
        GetTextSize(label, &button.size.x, &button.size.y);
    }
    else
    {
        button.size.x = button_size.x;
        button.size.y = button_size.y;
    }

    if (context.draw_widgets)
    {
        // button padding for y is not needed as the anchor of the text is not exactly left bottom
        lcdDrawFillRect(tft_dev, 
            button.position.x, button.position.y,
            button.position.x + button.size.x + style->button_padding, button.position.y + button.size.y,
            style->button_color);

        // the anchor point of the text is left bottom 
        DrawString(button.label, button.position.x + style->button_padding, button.position.y + button.size.y);
    }
   
    current_widget_group->buttons[button_pointer] = button;
    button_pointer++;

    current_widget_group->widget_order[widget_order_pointer] = BUTTON;
    widget_order_pointer++;

    UpdateCursorPos(&button.size);
}

void UISlider(const char* label, int* val, int min, int max)
{
    if (slider_pointer >= SLIDER_COUNT)
    {
        ESP_LOGI("UISlider", "Max sliders reached: %d", slider_pointer);
        return;
    }

    if (!current_widget_group)
    {
        ESP_LOGI("UISlider", "No widget group to add slider to. Button: %s", label);
        return;
    }

    UISliderInfo slider;
    slider.id = slider_pointer;
    slider.position = context.cursor_pos;
    slider.val = val;
    slider.min = min;
    slider.max = max;
    slider.size.x = display_width;
    slider.size.y = style->slider_height;
    slider.format = label;
    slider.font_used = style->font;
    snprintf(slider.label, 16, label, *val);

    // actual slider dragger position
    slider.slider_pos = slider.position;
    int _, label_size_y = 0;
    GetTextSize(label, &_, &label_size_y);
    slider.slider_pos.y += label_size_y;
    
    // label
    // (  <-------*---->  )
    // spacing links en rechts omdat de touch aan de randen slecht is 

    if (context.draw_widgets)
    {
        DrawString(slider.label, slider.position.x, slider.position.y + label_size_y);
        lcdDrawFillRect(tft_dev,
            slider.slider_pos.x, slider.slider_pos.y, 
            slider.size.x, slider.slider_pos.y + slider.size.y, 
            style->slider_bg_color);
            
        // draw grabber
        Vec2 drag_pos = slider.slider_pos;
        drag_pos.x = map(*slider.val, min, max, slider.position.x, slider.size.x);
        lcdDrawFillRect(tft_dev, 
            drag_pos.x, drag_pos.y, 
            drag_pos.x + style->slider_drag_width, drag_pos.y + style->slider_height,
            style->slider_drag_color);
    }
  
    slider.size.y += label_size_y;
    
    current_widget_group->sliders[slider_pointer] = slider;
    slider_pointer++;

    current_widget_group->widget_order[widget_order_pointer] = SLIDER;
    widget_order_pointer++;

    UpdateCursorPos(&slider.size);
}

static void UIColorPickerDraw(const char* label, int color[3])
{
    // touch is not very precise, uses 3 sliders 
    int* r = &color[0];
    int* g = &color[1];
    int* b = &color[2];
    
    // opens a new widget group 
}

void UIUpdate()
{
    if (!current_widget_group)
    {
        ESP_LOGI("UI", "no widget group selected, nothing will be drawn. Widget pointer: %d", widget_group_pointer);
        return;
    }
    
    static bool is_clicked = false;
    static int click_timer = 0;
    
    // get touch
    int x;
    int y;
    if (!touch_getxy(tft_dev, &x, &y))
    {
        click_timer = 0;
        return;
    }
    
    if (click_timer == 0)
    {
        is_clicked = true;
        click_timer++;
    }
    else
        is_clicked = false;
    
    ESP_LOGI(__FUNCTION__, "PRESSED AT: %d %d", x, y);

    // translate to ui coordinates 
    TranslateCoordinates(&x, &y);
    
    ESP_LOGI(__FUNCTION__, "TRANSLATED: %d %d", x, y);
    
    // update interactable widgets 

    if (is_clicked)
    {
        // check if we pressed a button
        for (int i = 0; i < current_widget_group->buttons_size; i++)
        {
            UIButtonInfo* button = &current_widget_group->buttons[i];
            if (!button)
                continue;
        
            if (IsPointInRect(x, y, &button->position, &button->size))
            {
                ESP_LOGI(__FUNCTION__, "BUTTON PRESSED. ID:%d POS(%d, %d) SIZE(%d, %d)", i,
                    button->position.x, button->position.y, button->size.x, button->size.y);

                
                
                button->callback(i);
            }
        }
    }

    for (int i = 0; i < current_widget_group->sliders_size; i++)
    {
        UISliderInfo* slider = &current_widget_group->sliders[i];
        if (!slider)
            continue;

        // ESP_LOGI(__FUNCTION__, "SLIDER CHECKPRESSED. ID:%d POS(%d, %d) SIZE(%d, %d)", i,
        //                slider->position.x, slider->position.y, slider->size.x, slider->size.y);

        if (IsPointInRect(x, y, &slider->position, &slider->size))
            UIUpdateSlider(slider, x);
    }
        
    SLEEP(50);
}
