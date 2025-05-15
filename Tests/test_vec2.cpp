#include <catch2/catch_all.hpp>

typedef struct
{

}FontxFile;
typedef struct 
{

}TFT_t;

extern "C"
{
#include "ui_widgets.h"
}

TEST_CASE("Vector2 Size", "[vec2]")
{
    Vec2 a;
    a.x = 0;
    a.y = 0;
    Vec2 b;
    REQUIRE(a.x == 0);
}
