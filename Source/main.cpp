#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "virtual_display.h"

extern "C"
{
#include "ui_widgets.h"
}

#include <iostream>

FontxFile* fx32M = nullptr;
FontxFile* fx24M = nullptr;
FontxFile* fx16M = nullptr;
static TFT_t dev;
static UIStyle style;
static int slider_value1 = 50;
static int group = 0;
static void ButtonCallback(int id)
{
    group ^= 1;
    UISetGroup(group);
    UIWidgetGroupDraw();
}

static void ui()
{
    style.button_color = GetColor(31, 0, 0);
	style.text_color = GetColor(31, 31, 31);
    style.font = fx32M;
	style.slider_bg_color = GetColor(0, 31, 0);
	style.slider_height = 35;
	style.slider_drag_color = GetColor(0, 10, 0);
	style.slider_drag_width = 5;
	style.spacing = (Vec2u8){5, 5};
    style.padding = 10;

    UIInit(&dev, &style);

    Vec2 size;
    size.x = 0;
    size.y = 0;

    UIBeginGroup();
    
    style.font = fx32M;
    UIButton("Go to second", ButtonCallback, size);
    UISlider("label %d", &slider_value1, 0, 100);
    
    UIEndGroup();

    UIBeginGroup();
    
    style.font = fx32M;
    UIButton("Go to first", ButtonCallback, size);
    UIText("Information");
    
    UIEndGroup();

    UISetGroup(0);
    UIWidgetGroupDraw();
}

int main()
{
    sf::RenderWindow window;
    window.create(sf::VideoMode{240, 320}, {});
    window.setFramerateLimit(30);
    
    sf::RenderTexture display_texture;
    display_texture.create(240, 320);
    
    // use a sprite for transformations
    sf::Sprite display_sprite;
    display_sprite.setTexture(display_texture.getTexture());
    
    // set fonts
    FontxFile font_8x16 {8, 16};
    FontxFile font_12_24 {12, 24};
    FontxFile font_16_32 {16, 32};
    
    fx16M = &font_8x16;
    fx24M = &font_12_24;
    fx32M = &font_16_32;

    VirtualDisplayInit();

    ui();

    while (window.isOpen())
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            if (e.type == sf::Event::Closed)
            {
                window.close();
                break;
            }

            VirtualDisplayEventHandler(e, window);
        }

        UIUpdate();

        VirtualDisplayRender(display_texture);
        display_texture.display();

        window.draw(display_sprite);
        window.display();

        window.clear();
    }

    window.close();
}
