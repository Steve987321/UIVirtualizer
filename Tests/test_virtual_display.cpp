#include <Catch2/catch_all.hpp>

#include <SFML/Graphics.hpp>
#include "virtual_display.h"
#include <iostream>

static sf::Image GetRenderedDisplay()
{
    sf::RenderTexture target;
    target.create(240, 320);
    
    VirtualDisplayRender(target);
    target.display();
    
    sf::Image image = target.getTexture().copyToImage();
    return image;
}

TEST_CASE("Virtual display drawing shows correct pixels", "[virtual_display]")
{
    VirtualDisplayInit(); 
    
    SECTION("lcdFillScreen fills entire screen")
    {
        lcdFillScreen(nullptr, 31);
        uint8_t blue_result = 255;
        
        sf::Image image = GetRenderedDisplay();
        sf::Vector2 size = image.getSize();

        int colors_diff = 0;
        for (unsigned x = 0; x < size.x - 1; x++)
        {
            for (unsigned y = 0; y < size.y; y++)
            {
                sf::Color pixel = image.getPixel(x, y);
                if (pixel.b != blue_result)
                    colors_diff++;
            }
        }

        REQUIRE(colors_diff == 0);
    }
}

