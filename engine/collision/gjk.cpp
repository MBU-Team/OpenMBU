//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//
// The core algorithms in this file are based on code written
// by G. van den Bergen for his interference detection library,
// "SOLID 2.0"
//-----------------------------------------------------------------------------

#include "gfx/gfxDevice.h"
#include "core/dataChunker.h"
#include "collision/collision.h"
#include "sim/sceneObject.h"
#include "collision/convex.h"
#include "collision/gjk.h"


//----------------------------------------------------------------------------

static F32 rel_error = 1E-5;     // relative error in the computed distance
static F32 sTolerance = 1E-3;    // Distance tolerance
static F32 sEpsilon2 = 1E-20;    // Zero length vector
static U32 sIteration = 15;     // Stuck in a loop?

S32 num_iterations = 0;
S32 num_irregularities = 0;


//----------------------------------------------------------------------------

GjkCollisionState::GjkCollisionState()
{
    a = b = 0;
}

GjkCollisionState::~GjkCollisionState()
{
}


//----------------------------------------------------------------------------

void GjkCollisionState::swap()
{
    Convex* t = a; a = b; b = t;
    CollisionStateList* l = mLista; mLista = mListb; mListb = l;
    v.neg();
}


//----------------------------------------------------------------------------

void GjkCollisionState::compute_det()
{
    // Dot new point with current set
    for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1)
        if (bits & bit)
            dp[i][last] = dp[last][i] = mDot(y[i], y[last]);
    dp[last][last] = mDot(y[last], y[last]);

    // Calulate the determinent
    det[last_bit][last] = 1;
    for (int j = 0, sj = 1; j < 4; ++j, sj <<= 1) {
        if (bits & sj) {
            int s2 = sj | last_bit;
            det[s2][j] = dp[last][last] - dp[last][j];
            det[s2][last] = dp[j][j] - dp[j][last];
            for (int k = 0, sk = 1; k < j; ++k, sk <<= 1) {
                if (bits & sk) {
                    int s3 = sk | s2;
                    det[s3][k] = det[s2][j] * (dp[j][j] - dp[j][k]) +
                        det[s2][last] * (dp[last][j] - dp[last][k]);
                    det[s3][j] = det[sk | last_bit][k] * (dp[k][k] - dp[k][j]) +
                        det[sk | last_bit][last] * (dp[last][k] - dp[last][j]);
                    det[s3][last] = det[sk | sj][k] * (dp[k][k] - dp[k][last]) +
                        det[sk | sj][j] * (dp[j][k] - dp[j][last]);
                }
            }
        }
    }

    if (all_bits == 15) {
        det[15][0] = det[14][1] * (dp[1][1] - dp[1][0]) +
            det[14][2] * (dp[2][1] - dp[2][0]) +
            det[14][3] * (dp[3][1] - dp[3][0]);
        det[15][1] = det[13][0] * (dp[0][0] - dp[0][1]) +
            det[13][2] * (dp[2][0] - dp[2][1]) +
            det[13][3] * (dp[3][0] - dp[3][1]);
        det[15][2] = det[11][0] * (dp[0][0] - dp[0][2]) +
            det[11][1] * (dp[1][0] - dp[1][2]) +
            det[11][3] * (dp[3][0] - dp[3][2]);
        det[15][3] = det[7][0] * (dp[0][0] - dp[0][3]) +
            det[7][1] * (dp[1][0] - dp[1][3]) +
            det[7][2] * (dp[2][0] - dp[2][3]);
    }
}


//----------------------------------------------------------------------------

inline void GjkCollisionState::compute_vector(int bits, VectorF& v)
{
    F32 sum = 0;
    v.set(0, 0, 0);
    for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) {
        if (bits & bit) {
            sum += det[bits][i];
            v += y[i] * det[bits][i];
        }
    }
    v *= 1 / sum;
}


//----------------------------------------------------------------------------

inline bool GjkCollisionState::valid(int s)
{
    for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) {
        if (all_bits & bit) {
            if (s & bit) {
                if (det[s][i] <= 0)
                    return false;
            }
            else
                if (det[s | bit][i] > 0)
                    return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------

inline bool GjkCollisionState::closest(VectorF& v)
{
    compute_det();
    for (int s = bits; s; --s) {
        if ((s & bits) == s) {
            if (valid(s | last_bit)) {
                bits = s | last_bit;
                if (bits != 15)
                    compute_vector(bits, v);
                return true;
            }
        }
    }
    if (valid(last_bit)) {
        bits = last_bit;
        v = y[last];
        return true;
    }
    return false;
}


//----------------------------------------------------------------------------

inline bool GjkCollisionState::degenerate(const VectorF& w)
{
    for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1)
        if ((all_bits & bit) && y[i] == w)
            return true;
    return false;
}


//----------------------------------------------------------------------------

inline void GjkCollisionState::nextBit()
{
    last = 0;
    last_bit = 1;
    while (bits & last_bit) {
        ++last;
        last_bit <<= 1;
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

void GjkCollisionState::set(Convex* aa, Convex* bb,
    const MatrixF& a2w, const MatrixF& b2w)
{
    a = aa;
    b = bb;

    bits = 0;
    all_bits = 0;
    reset(a2w, b2w);

    // link
    mLista = CollisionStateList::alloc();
    mLista->mState = this;
    mListb = CollisionStateList::alloc();
    mListb->mState = this;
}


//----------------------------------------------------------------------------

void GjkCollisionState::reset(const MatrixF& a2w, const MatrixF& b2w)
{
    VectorF zero(0, 0, 0), sa, sb;
    a2w.mulP(a->support(zero), &sa);
    b2w.mulP(b->support(zero), &sb);
    v = sa - sb;
    dist = v.len();
}


//----------------------------------------------------------------------------

void GjkCollisionState::getCollisionInfo(const MatrixF& mat, Collision* info)
{
    AssertFatal(false, "GjkCollisionState::getCollisionInfo() - There remain scaling problems here.");
    // This assumes that the shapes do not intersect
    Point3F pa, pb;
    if (bits) {
        getClosestPoints(pa, pb);
        mat.mulP(pa, &info->point);
        b->getTransform().mulP(pb, &pa);
        info->normal = info->point - pa;
    }
    else {
        mat.mulP(p[last], &info->point);
        info->normal = v;
    }
    info->normal.normalize();
    info->object = b->getObject();
}

void GjkCollisionState::getClosestPoints(Point3F& p1, Point3F& p2)
{
    F32 sum = 0;
    p1.set(0, 0, 0);
    p2.set(0, 0, 0);
    for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) {
        if (bits & bit) {
            sum += det[bits][i];
            p1 += p[i] * det[bits][i];
            p2 += q[i] * det[bits][i];
        }
    }
    F32 s = 1 / sum;
    p1 *= s;
    p2 *= s;
}


//----------------------------------------------------------------------------

bool GjkCollisionState::intersect(const MatrixF& a2w, const MatrixF& b2w)
{
    num_iterations = 0;
    MatrixF w2a, w2b;

    w2a = a2w;
    w2b = b2w;
    w2a.inverse();
    w2b.inverse();
    reset(a2w, b2w);

    bits = 0;
    all_bits = 0;

    do {
        nextBit();

        VectorF va, sa;
        w2a.mulV(-v, &va);
        p[last] = a->support(va);
        a2w.mulP(p[last], &sa);

        VectorF vb, sb;
        w2b.mulV(v, &vb);
        q[last] = b->support(vb);
        b2w.mulP(q[last], &sb);

        VectorF w = sa - sb;
        if (mDot(v, w) > 0)
            return false;
        if (degenerate(w)) {
            ++num_irregularities;
            return false;
        }

        y[last] = w;
        all_bits = bits | last_bit;

        ++num_iterations;
        if (!closest(v) || num_iterations > sIteration) {
            ++num_irregularities;
            return false;
        }
    } while (bits < 15 && v.lenSquared() > sEpsilon2);
    return true;
}

F32 GjkCollisionState::distance(const MatrixF& a2w, const MatrixF& b2w,
    const F32 dontCareDist, const MatrixF* _w2a, const MatrixF* _w2b)
{
    num_iterations = 0;
    MatrixF w2a, w2b;

    if (_w2a == NULL || _w2b == NULL) {
        w2a = a2w;
        w2b = b2w;
        w2a.inverse();
        w2b.inverse();
    }
    else {
        w2a = *_w2a;
        w2b = *_w2b;
    }

    reset(a2w, b2w);
    bits = 0;
    all_bits = 0;
    F32 mu = 0;

    do {
        nextBit();

        VectorF va, sa;
        w2a.mulV(-v, &va);
        p[last] = a->support(va);
        a2w.mulP(p[last], &sa);

        VectorF vb, sb;
        w2b.mulV(v, &vb);
        q[last] = b->support(vb);
        b2w.mulP(q[last], &sb);

        VectorF w = sa - sb;
        F32 nm = mDot(v, w) / dist;
        if (nm > mu)
            mu = nm;
        if (mu > dontCareDist)
            return mu;
        if (mFabs(dist - mu) <= dist * rel_error)
            return dist;

        ++num_iterations;
        if (degenerate(w) || num_iterations > sIteration) {
            ++num_irregularities;
            return dist;
        }

        y[last] = w;
        all_bits = bits | last_bit;

        if (!closest(v)) {
            ++num_irregularities;
            return dist;
        }

        dist = v.len();
    } while (bits < 15 && dist > sTolerance);

    if (bits == 15 && mu <= 0)
        dist = 0;
    return dist;
}


//----------------------------------------------------------------------------

inline void renderCross(const Point3F x)
{
    /*
       glVertex3fv(x + VectorF(0,0,+0.1));
       glVertex3fv(x + VectorF(0,0,-0.1));
       glVertex3fv(x + VectorF(0,+0.1,0));
       glVertex3fv(x + VectorF(0,-0.1,0));
       glVertex3fv(x + VectorF(-0.1,0,0));
       glVertex3fv(x + VectorF(+0.1,0,0));
    */
}

void GjkCollisionState::render()
{
    /*
       glBegin(GL_LINES);
       glColor3f(1,1,0);

       // Cross for each witness point
       for (int i = 0, bit = 1; i < 4; ++i, bit <<= 1) {
          Point3F x;
          if (bits & bit) {
             a->getTransform().mulP(p[i],&x);
             renderCross(x);
             b->getTransform().mulP(q[i],&x);
             renderCross(x);
          }
       }

       // Cross and line for closest points
       Point3F sp,ep;
       if (bits) {
          Point3F p1,p2;
          getClosestPoints(p1,p2);
          a->getTransform().mulP(p1,&sp);
          b->getTransform().mulP(p2,&ep);
       }
       else {
          a->getTransform().mulP(p[0],&sp);
          b->getTransform().mulP(q[0],&ep);
       }
       renderCross(sp);
       renderCross(ep);
       glVertex3fv(sp);
       glVertex3fv(ep);

       glEnd();
    */
}
