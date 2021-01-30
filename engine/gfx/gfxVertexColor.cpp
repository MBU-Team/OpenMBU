#include "gfx/gfxVertexColor.h"

GFXVertexColor::ColorOrdering GFXVertexColor::sVertexColorOrdering = GFXVertexColor::RGBA;

GFXVertexColor::GFXVertexColor(const ColorI& color)
{
    set(color.red, color.green, color.blue, color.alpha);
}

GFXVertexColor::GFXVertexColor(const ColorF& color)
{
    set(color.red * 255.0f, color.green * 255.0f, color.blue * 255.0f, color.alpha * 255.0f);
}

// Color ordering goes least significant to most significant bit, so
// RGBA is R as least significant byte and A as most significant
void GFXVertexColor::set(U8 red, U8 green, U8 blue, U8 alpha)
{
    if (sVertexColorOrdering == GFXVertexColor::RGBA)
    {
        packedColorData = (U32(red) << 0) |
            (U32(green) << 8) |
            (U32(blue) << 16) |
            (U32(alpha) << 24);
    }
    else
    {

        packedColorData = (U32(alpha) << 24) |
            (U32(red) << 16) |
            (U32(green) << 8) |
            (U32(blue) << 0);

    }
}

//-----------------------------------------------------------------------------

void GFXVertexColor::getColor(ColorI* color) const
{
    if (sVertexColorOrdering == GFXVertexColor::RGBA)
    {
        color->alpha = (packedColorData >> 24) & 0x000000FF;
        color->blue = (packedColorData >> 16) & 0x000000FF;
        color->green = (packedColorData >> 8) & 0x000000FF;
        color->red = packedColorData & 0x000000FF;
    }
    else
    {
        color->alpha = (packedColorData >> 24) & 0x000000FF;
        color->red = (packedColorData >> 16) & 0x000000FF;
        color->green = (packedColorData >> 8) & 0x000000FF;
        color->blue = packedColorData & 0x000000FF;
    }
}