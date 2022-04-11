//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "editor/terrain/terraformerNoise.h"
#include "editor/terrain/terraformer.h"


//--------------------------------------
Noise2D::Noise2D()
{
    mSeed = 0;
}

Noise2D::~Noise2D()
{
}


//--------------------------------------
void Noise2D::normalize(F32 v[2])
{
    F32 s;

    s = mSqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] = v[0] / s;
    v[1] = v[1] / s;
}


//--------------------------------------
void Noise2D::setSeed(U32 seed)
{
    if (mSeed == seed)
        return;
    mSeed = seed;
    mRandom.setSeed(mSeed);

    S32 i, j, k;

    for (i = 0; i < SIZE; i++) {
        mPermutation[i] = i;

        for (j = 0; j < 2; j++)
            mGradient[i][j] = mRandom.randF(-1.0f, 1.0f);
        normalize(mGradient[i]);
    }

    while (--i) {
        k = mPermutation[i];
        j = mRandom.randI(0, SIZE - 1);
        mPermutation[i] = mPermutation[j];
        mPermutation[j] = k;
    }

    // extend the size of the arrays x2 to get rid of a bunch of MODs
    // we'd have to do later in the code
    for (i = 0; i < SIZE + 2; i++) {
        mPermutation[SIZE + i] = mPermutation[i];
        for (j = 0; j < 2; j++)
            mGradient[SIZE + i][j] = mGradient[i][j];
    }
}


//--------------------------------------
U32 Noise2D::getSeed()
{
    return mSeed;
}


inline F32 Noise2D::lerp(F32 t, F32 a, F32 b)
{
    return a + t * (b - a);
}


inline F32 Noise2D::curve(F32 t)
{
    return t * t * (3.0f - 2.0f * t);
}


inline F32 clamp(F32 f, F32 m)
{
    while (f > m)
        f -= m;
    while (f < 0.0f)
        f += m;
    return f;
}


//--------------------------------------
void Noise2D::fBm(Heightfield* dst, U32 size, U32 interval, F32 h, F32 octaves)
{
    interval = getMin(U32(128), getMax(U32(1), interval));
    F32 H = getMin(1.0f, getMax(0.0f, h));
    octaves = getMin(5.0f, getMax(1.0f, octaves));
    F32 lacunarity = 2.0f;

    F32 exponent_array[32];

    // precompute and store spectral weights
    // seize required memory for exponent_array
    F32 frequency = 1.0;
    for (U32 i = 0; i <= octaves; i++)
    {
        // compute weight for each frequency
        exponent_array[i] = mPow(frequency, -H);
        frequency *= lacunarity;
    }

    // initialize dst
    for (S32 k = 0; k < (size * size); k++)
        dst->val(k) = 0.0f;

    F32 scale = 1.0f / (F32)size * interval;
    for (S32 o = 0; o < octaves; o++)
    {
        F32 exp = exponent_array[o];
        for (S32 y = 0; y < size; y++)
        {
            F32 fy = (F32)y * scale;
            for (S32 x = 0; x < size; x++)
            {
                F32 fx = (F32)x * scale;
                F32 noise = getValue(fx, fy, interval);
                dst->val(x, y) += noise * exp;
            }
        }
        scale *= lacunarity;
        interval = (U32)(interval * lacunarity);
    }
}


//--------------------------------------
void Noise2D::rigidMultiFractal(Heightfield* dst, Heightfield* sig, U32 size, U32 interval, F32 h, F32 octaves)
{
    interval = getMin(U32(128), getMax(U32(1), interval));
    F32 H = getMin(1.0f, getMax(0.0f, h));
    octaves = getMin(5.0f, getMax(1.0f, octaves));
    F32 lacunarity = 2.0f;
    F32 offset = 1.0f;
    F32 gain = 2.0f;

    F32 exponent_array[32];

    // precompute and store spectral weights
    // seize required memory for exponent_array
    F32 frequency = 1.0;
    for (U32 i = 0; i <= octaves; i++)
    {
        // compute weight for each frequency
        exponent_array[i] = mPow(frequency, -H);
        frequency *= lacunarity;
    }

    F32 scale = 1.0f / (F32)size * interval;

    //--------------------------------------
    // compute first octave
    for (S32 y = 0; y < size; y++)
    {
        F32 fy = (F32)y * scale;
        for (S32 x = 0; x < size; x++)
        {
            F32 fx = (F32)x * scale;

            F32 signal = mFabs(getValue(fx, fy, interval));   // get absolute value of signal (this creates the ridges)
            //signal = mSqrt(signal);
            signal = offset - signal;  // invert and translate (note that "offset" should be ~= 1.0)
            signal *= signal + 0.1;          // square the signal, to increase "sharpness" of ridges

            // assign initial values
            dst->val(x, y) = signal;
            sig->val(x, y) = signal;
        }
    }

    //--------------------------------------
    // compute remaining octaves
    for (S32 o = 1; o < octaves; o++)
    {
        // increase the frequency
        scale *= lacunarity;
        interval = (U32)(interval * lacunarity);
        F32 exp = exponent_array[o];

        for (S32 y = 0; y < size; y++)
        {
            F32 fy = (F32)y * scale;
            for (S32 x = 0; x < size; x++)
            {
                F32 fx = (F32)x * scale;
                U32 index = dst->offset(x, y);
                F32 result = dst->val(index);
                F32 signal = sig->val(index);

                // weight successive contributions by previous signal
                F32 weight = mClampF(signal * gain, 0.0f, 1.0f);

                signal = mFabs(getValue(fx, fy, interval));

                signal = offset - signal;
                signal *= signal + 0.2;
                // weight the contribution
                signal *= weight;
                result += signal * exp;

                dst->val(index) = result;
                sig->val(index) = signal;
            }
        }
    }
    for (S32 k = 0; k < (size * size); k++)
        dst->val(k) = (dst->val(k) - 1.0f) / 2.0f;
}


//--------------------------------------
F32 Noise2D::turbulence(F32 x, F32 y, F32 freq)
{
    F32 t, x2, y2;

    for (t = 0.0f; freq >= 3.0f; freq /= 2.0f)
    {
        x2 = freq * x;
        y2 = freq * y;
        t += mFabs(getValue(x2, y2, (S32)freq)) / freq;
    }
    return t;
}


//--------------------------------------
inline void Noise2D::setup(F32 t, S32& b0, S32& b1, F32& r0, F32& r1)
{
    // find the bounding integers of u
    b0 = S32(t) & SIZE_MASK;
    b1 = (b0 + 1) & SIZE_MASK;

    // seperate the fractional components
    r0 = t - (S32)t;
    r1 = r0 - 1.0f;
}

inline F32 Noise2D::dot(const F32* q, F32 rx, F32 ry)
{
    return (rx * q[0] + ry * q[1]);
}



//--------------------------------------
F32 Noise2D::getValue(F32 x, F32 y, S32 interval)
{
    S32 bx0, bx1, by0, by1;
    F32 rx0, rx1, ry0, ry1;

    // Imagine having a square of the type
    //  p0---p1    Where p0 = (bx0, by0)   +----> U
    //  |(u,v)|          p1 = (bx1, by0)   |
    //  |     |          p2 = (bx0, by1)   |    Coordinate System
    //  p2---p3          p3 = (bx1, by1)   V
    // The u, v point in 2D texture space is bounded by this rectangle.

    // Goal, determine the scalar at the points p0, p1, p2, p3.
    // Then the scalar of the point (u, v) will be found by linear interpolation.

    // First step:  Get the 2D coordinates of the points p0, p1, p2, p3.
    // We also need vectors pointing from each point in the square above and
    // ending at the (u,v) coordinate located inside the square.
    // The vector (rx0, ry0) goes from P0 to the (u,v) coordinate.
    // The vector (rx1, ry0) goes from P1 to the (u,v) coordinate.
    // The vector (rx0, ry1) goes from P2 to the (u,v) coordinate.
    // The vector (rx1, ry1) goes from P3 to the (u,v) coordinate.

    setup(x, bx0, bx1, rx0, rx1);
    setup(y, by0, by1, ry0, ry1);

    // Make sure the box corners fall within the interval
    // so that the final output will wrap on itself
    bx0 = bx0 % interval;
    bx1 = bx1 % interval;
    by0 = by0 % interval;
    by1 = by1 % interval;

    S32 i = mPermutation[bx0];
    S32 j = mPermutation[bx1];

    S32 b00 = mPermutation[i + by0];
    S32 b10 = mPermutation[j + by0];
    S32 b01 = mPermutation[i + by1];
    S32 b11 = mPermutation[j + by1];

    // Next, calculate the dropoff component about the point p0.
    F32 sx = curve(rx0);
    F32 sy = curve(ry0);

    // Now, for each point in the square shown above, calculate the dot
    // product of the gradiant vector and the vector going from each square
    // corner point to the (u,v) point inside the square.
    F32 u = dot(mGradient[b00], rx0, ry0);
    F32 v = dot(mGradient[b10], rx1, ry0);

    // Interpolation along the X axis.
    F32 a = lerp(sx, u, v);

    u = dot(mGradient[b01], rx0, ry1);
    v = dot(mGradient[b11], rx1, ry1);

    // Interpolation along the Y axis.
    F32 b = lerp(sx, u, v);

    // Final Interpolation
    return lerp(sy, a, b);
}


