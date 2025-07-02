#ifndef __MY_MATH_H__
#define __MY_MATH_H__

template<typename T>
T clamp(T val, T minVal, T maxVal)
{
    if(val < minVal) return minVal;
    else if (val>maxVal) return maxVal;
    else return val;
}

CRGB lerp(CRGB a, CRGB b, float t)
{
    t = clamp(t, 0.0f, 1.0f);
    uint8_t red = static_cast<uint8_t>(a.r + (b.r - a.r) * t);
    uint8_t grn = static_cast<uint8_t>(a.g + (b.g - a.g) * t);
    uint8_t blu = static_cast<uint8_t>(a.b + (b.b - a.b) * t);

    return CRGB(red, grn, blu);
}

#endif