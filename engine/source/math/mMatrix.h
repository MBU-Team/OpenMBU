//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MMATRIX_H_
#define _MMATRIX_H_


#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

/// 4x4 Matrix Class
///
/// This runs at F32 precision.
class MatrixF
{
private:
    F32 m[16];     ///< Note: Torque uses column major matrices

public:
    /// Create an uninitialized matrix.
    ///
    /// @param   identity    If true, initialize to the identity matrix.
    explicit MatrixF(bool identity = false);

    /// Create a matrix to rotate about origin by e.
    /// @see set
    explicit MatrixF(const EulerF& e);

    /// Create a matrix to rotate about p by e.
    /// @see set
    MatrixF(const EulerF& e, const Point3F& p);

    /// Get the index in m to element in column i, row j
    ///
    /// This is necessary as we have m as a one dimensional array.
    ///
    /// @param   i   Column desired.
    /// @param   j   Row desired.
    static U32 idx(U32 i, U32 j) { return (i + j * 4); }

    /// Initialize matrix to rotate about origin by e.
    MatrixF& set(const EulerF& e);

    MatrixF& setEulerFromTrenchbroom(const EulerF& e);

    /// Initialize matrix to rotate about p by e.
    MatrixF& set(const EulerF& e, const Point3F& p);

    /// Initialize matrix with a cross product of p.
    MatrixF& setCrossProduct(const Point3F& p);

    /// Initialize matrix with a tensor product of p.
    MatrixF& setTensorProduct(const Point3F& p, const Point3F& q);

    operator F32* () { return (m); }              ///< Allow people to get at m.
    operator F32* () const { return (F32*)(m); }  ///< Allow people to get at m.

    bool isAffine() const;                       ///< Check to see if this is an affine matrix.
    bool isIdentity() const;                     ///< Checks for identity matrix.

    /// Make this an identity matrix.
    MatrixF& identity();

    /// Invert m.
    MatrixF& inverse();

    /// Take inverse without disturbing position data.
    ///
    /// Ie, take inverse of 3x3 submatrix.
    MatrixF& affineInverse();
    MatrixF& transpose();                        ///< Swap rows and columns.
    MatrixF& scale(const Point3F& p);            ///< M * Matrix(p) -> M
    Point3F getScale() const;                    ///< Return scale assuming scale was applied via mat.scale(s).

    EulerF toEuler() const;

    /// Compute the inverse of the matrix.
    ///
    /// Computes inverse of full 4x4 matrix. Returns false and performs no inverse if
    /// the determinant is 0.
    ///
    /// Note: In most cases you want to use the normal inverse function.  This method should
    ///       be used if the matrix has something other than (0,0,0,1) in the bottom row.
    bool fullInverse();

    /// Swaps rows and columns into matrix.
    void transposeTo(F32* matrix) const;

    /// Normalize the matrix.
    void normalize();

    /// Copy the requested column into a Point4F.
    void getColumn(S32 col, Point4F* cptr) const;

    /// Copy the requested column into a Point3F.
    ///
    /// This drops the bottom-most row.
    void getColumn(S32 col, Point3F* cptr) const;

    /// Set the specified column from a Point4F.
    void setColumn(S32 col, const Point4F& cptr);

    /// Set the specified column from a Point3F.
    ///
    /// The bottom-most row is not set.
    void setColumn(S32 col, const Point3F& cptr);

    /// Copy the specified row into a Point4F.
    void getRow(S32 row, Point4F* cptr) const;

    /// Copy the specified row into a Point3F.
    ///
    /// Right-most item is dropped.
    void getRow(S32 row, Point3F* cptr) const;

    /// Set the specified row from a Point4F.
    void setRow(S32 row, const Point4F& cptr);

    /// Set the specified row from a Point3F.
    ///
    /// The right-most item is not set.
    void setRow(S32 row, const Point3F& cptr);

    /// Get the position of the matrix.
    ///
    /// This is the 4th column of the matrix.
    Point3F   getPosition() const;

    /// Set the position of the matrix.
    ///
    /// This is the 4th column of the matrix.
    void      setPosition(const Point3F& pos) { setColumn(3, pos); }

    MatrixF& mul(const MatrixF& a);                    ///< M * a -> M
    MatrixF& mul(const MatrixF& a, const MatrixF& b);  ///< a * b -> M

    // Scalar multiplies
    MatrixF& mul(const F32 a);                         ///< M * a -> M
    MatrixF& mul(const MatrixF& a, const F32 b);       ///< a * b -> M

    MatrixF& mulL(const MatrixF& a);                   ///< a * M -> M
    void mul(Point4F& p) const;                       ///< M * p -> p (full [4x4] * [1x4])
    void mulP(Point3F& p) const;                      ///< M * p -> p (assume w = 1.0f)
    void mulP(const Point3F& p, Point3F* d) const;     ///< M * p -> d (assume w = 1.0f)
    void mulV(VectorF& p) const;                      ///< M * v -> v (assume w = 0.0f)
    void mulV(const VectorF& p, Point3F* d) const;     ///< M * v -> d (assume w = 0.0f)

    void mul(Box3F& b) const;                           ///< Axial box -> Axial Box

    /// Convenience function to allow people to treat this like an array.
    F32& operator ()(S32 row, S32 col) { return m[idx(col, row)]; }

    void dumpMatrix(const char* caption = NULL) const;

    // Math operator overloads
    //------------------------------------
    friend MatrixF operator * (const MatrixF& m1, const MatrixF& m2);
    MatrixF& operator *= (const MatrixF& m);

    // Static identity matrix
    const static MatrixF Identity;

};

//--------------------------------------
// Inline Functions

inline MatrixF::MatrixF(bool _identity)
{
    if (_identity)
        identity();
}

inline MatrixF::MatrixF(const EulerF& e)
{
    set(e);
}

inline MatrixF::MatrixF(const EulerF& e, const Point3F& p)
{
    set(e, p);
}

inline MatrixF& MatrixF::set(const EulerF& e)
{
    m_matF_set_euler(e, *this);
    return (*this);
}

inline MatrixF& MatrixF::setEulerFromTrenchbroom(const EulerF& e)
{
    auto pitch = e.x;
    auto yaw = e.y;
    auto roll = e.z;

    constexpr auto I = 1.0f;
    constexpr auto O = 0.0f;

    const auto Cr = mCos(roll);
    const auto Sr = mSin(roll);
    MatrixF R;
    R.m[0] = +I; R.m[1] = +O; R.m[2] = +O; R.m[3] = +O;
    R.m[4] = +O; R.m[5] = +Cr; R.m[6] = -Sr; R.m[7] = +O;
    R.m[8] = +O; R.m[9] = +Sr; R.m[10] = +Cr; R.m[11] = +O;
    R.m[12] = +O; R.m[13] = +O; R.m[14] = +O; R.m[15] = +I;

    const auto Cp = mCos(pitch);
    const auto Sp = mSin(pitch);
    MatrixF P;
    P.m[0] = +Cp; P.m[1] = +O; P.m[2] = +Sp; P.m[3] = +O;
    P.m[4] = +O; P.m[5] = +I; P.m[6] = +O; P.m[7] = +O;
    P.m[8] = -Sp; P.m[9] = +O; P.m[10] = +Cp; P.m[11] = +O;
    P.m[12] = +O; P.m[13] = +O; P.m[14] = +O; P.m[15] = +I;

    const auto Cy = mCos(yaw);
    const auto Sy = mSin(yaw);
    MatrixF Y;
    Y.m[0] = +Cy; Y.m[1] = -Sy; Y.m[2] = +O; Y.m[3] = +O;
    Y.m[4] = +Sy; Y.m[5] = +Cy; Y.m[6] = +O; Y.m[7] = +O;
    Y.m[8] = +O; Y.m[9] = +O; Y.m[10] = +I; Y.m[11] = +O;
    Y.m[12] = +O; Y.m[13] = +O; Y.m[14] = +O; Y.m[15] = +I;

    *this = Y * P * R;

    return (*this);
}

inline MatrixF& MatrixF::set(const EulerF& e, const Point3F& p)
{
    m_matF_set_euler_point(e, p, *this);
    return (*this);
}

inline MatrixF& MatrixF::setCrossProduct(const Point3F& p)
{
    m[1] = -(m[4] = p.z);
    m[8] = -(m[2] = p.y);
    m[6] = -(m[9] = p.x);
    m[0] = m[3] = m[5] = m[7] = m[10] = m[11] =
        m[12] = m[13] = m[14] = 0;
    m[15] = 1;
    return (*this);
}

inline MatrixF& MatrixF::setTensorProduct(const Point3F& p, const Point3F& q)
{
    m[0] = p.x * q.x;
    m[1] = p.x * q.y;
    m[2] = p.x * q.z;
    m[4] = p.y * q.x;
    m[5] = p.y * q.y;
    m[6] = p.y * q.z;
    m[8] = p.z * q.x;
    m[9] = p.z * q.y;
    m[10] = p.z * q.z;
    m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0;
    m[15] = 1;
    return (*this);
}

inline bool MatrixF::isIdentity() const
{
    return
        m[0] == 1.0f &&
        m[1] == 0.0f &&
        m[2] == 0.0f &&
        m[3] == 0.0f &&
        m[4] == 0.0f &&
        m[5] == 1.0f &&
        m[6] == 0.0f &&
        m[7] == 0.0f &&
        m[8] == 0.0f &&
        m[9] == 0.0f &&
        m[10] == 1.0f &&
        m[11] == 0.0f &&
        m[12] == 0.0f &&
        m[13] == 0.0f &&
        m[14] == 0.0f &&
        m[15] == 1.0f;
}

inline MatrixF& MatrixF::identity()
{
    m[0] = 1.0f;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 1.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = 1.0f;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    return (*this);
}


inline MatrixF& MatrixF::inverse()
{
    m_matF_inverse(m);
    return (*this);
}

inline MatrixF& MatrixF::affineInverse()
{
    //   AssertFatal(isAffine() == true, "Error, this matrix is not an affine transform");
    m_matF_affineInverse(m);
    return (*this);
}

inline MatrixF& MatrixF::transpose()
{
    m_matF_transpose(m);
    return (*this);
}

inline MatrixF& MatrixF::scale(const Point3F& p)
{
    m_matF_scale(m, p);
    return *this;
}

inline Point3F MatrixF::getScale() const
{
    Point3F scale;
    scale.x = mSqrt(m[0]*m[0] + m[4] * m[4] + m[8] * m[8]);
    scale.y = mSqrt(m[1]*m[1] + m[5] * m[5] + m[9] * m[9]);
    scale.z = mSqrt(m[2]*m[2] + m[6] * m[6] + m[10] * m[10]);
    return scale;
}

inline void MatrixF::normalize()
{
    m_matF_normalize(m);
}

inline MatrixF& MatrixF::mul(const MatrixF& a)
{  // M * a -> M
    AssertFatal(&a != this, "MatrixF::mul - a.mul(a) is invalid!");

    MatrixF tempThis(*this);
    m_matF_x_matF(tempThis, a, *this);
    return (*this);
}


inline MatrixF& MatrixF::mul(const MatrixF& a, const MatrixF& b)
{  // a * b -> M
    AssertFatal((&a != this) && (&b != this), "MatrixF::mul - a.mul(a, b) a.mul(b, a) a.mul(a, a) is invalid!");

    m_matF_x_matF(a, b, *this);
    return (*this);
}


inline MatrixF& MatrixF::mul(const F32 a)
{
    for (U32 i = 0; i < 16; i++)
        m[i] *= a;

    return *this;
}


inline MatrixF& MatrixF::mul(const MatrixF& a, const F32 b)
{
    *this = a;
    mul(b);

    return *this;
}

inline void MatrixF::mul(Point4F& p) const
{
    Point4F temp;
    m_matF_x_point4F(*this, &p.x, &temp.x);
    p = temp;
}

inline void MatrixF::mulP(Point3F& p) const
{
    // M * p -> d
    Point3F d;
    m_matF_x_point3F(*this, &p.x, &d.x);
    p = d;
}

inline void MatrixF::mulP(const Point3F& p, Point3F* d) const
{
    // M * p -> d
    m_matF_x_point3F(*this, &p.x, &d->x);
}

inline void MatrixF::mulV(VectorF& v) const
{
    // M * v -> v
    VectorF temp;
    m_matF_x_vectorF(*this, &v.x, &temp.x);
    v = temp;
}

inline void MatrixF::mulV(const VectorF& v, Point3F* d) const
{
    // M * v -> d
    m_matF_x_vectorF(*this, &v.x, &d->x);
}

inline void MatrixF::mul(Box3F& b) const
{
    m_matF_x_box3F(*this, &b.min.x, &b.max.x);
}

inline MatrixF& MatrixF::mulL(const MatrixF& a)
{  // a * M -> M
    AssertFatal(&a != this, "MatrixF::mulL - a.mul(a) is invalid!");

    MatrixF tempThis(*this);
    m_matF_x_matF(a, tempThis, *this);
    return (*this);
}

inline void MatrixF::getColumn(S32 col, Point4F* cptr) const
{
    cptr->x = m[col];
    cptr->y = m[col + 4];
    cptr->z = m[col + 8];
    cptr->w = m[col + 12];
}

inline void MatrixF::getColumn(S32 col, Point3F* cptr) const
{
    cptr->x = m[col];
    cptr->y = m[col + 4];
    cptr->z = m[col + 8];
}

inline void MatrixF::setColumn(S32 col, const Point4F& cptr)
{
    m[col] = cptr.x;
    m[col + 4] = cptr.y;
    m[col + 8] = cptr.z;
    m[col + 12] = cptr.w;
}

inline void MatrixF::setColumn(S32 col, const Point3F& cptr)
{
    m[col] = cptr.x;
    m[col + 4] = cptr.y;
    m[col + 8] = cptr.z;
}


inline void MatrixF::getRow(S32 col, Point4F* cptr) const
{
    col *= 4;
    cptr->x = m[col++];
    cptr->y = m[col++];
    cptr->z = m[col++];
    cptr->w = m[col];
}

inline void MatrixF::getRow(S32 col, Point3F* cptr) const
{
    col *= 4;
    cptr->x = m[col++];
    cptr->y = m[col++];
    cptr->z = m[col];
}

inline void MatrixF::setRow(S32 col, const Point4F& cptr)
{
    col *= 4;
    m[col++] = cptr.x;
    m[col++] = cptr.y;
    m[col++] = cptr.z;
    m[col] = cptr.w;
}

inline void MatrixF::setRow(S32 col, const Point3F& cptr)
{
    col *= 4;
    m[col++] = cptr.x;
    m[col++] = cptr.y;
    m[col] = cptr.z;
}

// not too speedy, but convienient
inline Point3F MatrixF::getPosition() const
{
    Point3F pos;
    getColumn(3, &pos);
    return pos;
}

// Matrix Rotation
inline void rotateMatrix(MatrixF& mat, Point3F w)
{
    float deltaTheta = w.len();
    if (deltaTheta == 0.0f)
        return;

    float cosDeltaTheta = 1.0f / deltaTheta;
    w *= cosDeltaTheta;

    float sinDeltaTheta;
    mSinCos(deltaTheta, sinDeltaTheta, cosDeltaTheta);

    float *mat8 = &mat[8];

    Point3F r;
    Point3F rpw;
    Point3F rorw;
    Point3F rporw;
    int it = 3;
    do
    {
        r.x = *(mat8 - 8);
        r.y = *(mat8 - 4);
        r.z = *mat8;
        rpw = w;
        deltaTheta = mDot(w, r);
        rpw *= deltaTheta;
        rorw = r - rpw;
        mCross(w, rorw, &rporw);
        rorw *= cosDeltaTheta;
        rporw *= sinDeltaTheta;
        rpw += rorw;
        rpw += rporw;

        *(mat8 - 8) = rpw.x;
        *(mat8 - 4) = rpw.y;
        *mat8++ = rpw.z;

        --it;
    } while (it);

    m_matF_normalize(mat);
}

//------------------------------------
// Math operator overloads
//------------------------------------
inline MatrixF operator * (const MatrixF& m1, const MatrixF& m2)
{
    // temp = m1 * m2
    MatrixF temp;
    m_matF_x_matF(m1, m2, temp);
    return temp;
}

inline MatrixF& MatrixF::operator *= (const MatrixF& m)
{
    MatrixF tempThis(*this);
    m_matF_x_matF(tempThis, m, *this);
    return (*this);
}



#endif //_MMATRIX_H_
