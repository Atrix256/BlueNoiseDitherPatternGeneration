#include "generatebn_paniq2.h"
#include "vec.h"
#include "convert.h"

#define LEVEL 15U
#define WIDTH ( (1U << LEVEL) )
#define AREA ( WIDTH * WIDTH )

uint part1by1(uint x) {
    x = (x & 0x0000ffffu);
    x = ((x ^ (x << 8u)) & 0x00ff00ffu);
    x = ((x ^ (x << 4u)) & 0x0f0f0f0fu);
    x = ((x ^ (x << 2u)) & 0x33333333u);
    x = ((x ^ (x << 1u)) & 0x55555555u);
    return x;
}

uint pack_morton2x16(uvec2 v) {
    return part1by1(v[0]) | (part1by1(v[1]) << 1);
}

// from https://www.shadertoy.com/view/XtGBDW
uint HilbertIndex(uvec2 Position, size_t width)
{
    uint Index = 0U;
    for (uint CurLevel = WIDTH / 2U; CurLevel > 0U; CurLevel /= 2U)
    {
        uvec2 Region = uvec2(greaterThan((Position & uvec2{ CurLevel, CurLevel }), uvec2{ 0, 0 }));
        Index += CurLevel * CurLevel * ((3U * Region[0]) ^ Region[1]);
        if (Region[1] == 0U)
        {
            if (Region[0] == 1U)
            {
                Position = uvec2{ uint(width - 1), uint(width - 1) } - Position;
            }
            std::swap(Position[0], Position[1]);
        }
    }

    return Index;
}

float mainImage(vec2 fragCoord, vec2 iResolution)
{
    vec2 uv = fragCoord / iResolution;
#if 1
    uint x = HilbertIndex(uvec2{ uint(fragCoord[0]), uint(fragCoord[1]) }, size_t(iResolution[0])) % (1u << 17u);
#else
    uint x = pack_morton2x16(uvec2{ fragCoord[0], fragCoord[1] }) % (1u << 17u);
#endif

    float phi = 2.0f / (sqrt(5.0f) + 1.0f);
    float c = fract(0.5f + phi * float(x));

    /*
    if (uv[0] > 0.5) {
        c = step(c, uv[1]);
    }
    */

    return c;
}

void GenerateBN_Paniq2(
    std::vector<uint8_t>& blueNoise,
    size_t width
)
{
    blueNoise.resize(width*width);

    for (size_t y = 0; y < width; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            float value = mainImage(vec2{ float(x), float(y) }, vec2{ float(width), float(width) });
            blueNoise[y*width + x] = FromFloat<uint8_t>(value);
        }
    }
}

// NOTE: animating it? weird results... https://twitter.com/R4_Unit/status/1141138019488944129?s=03
