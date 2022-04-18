#ifdef IN_HLSL

#define VC_WORLD_PROJ      C0

#define VC_TEX_TRANS1      C4
#define VC_LIGHT_TRANS     C8
#define VC_OBJ_TRANS       C12

#define VC_CUBE_TRANS      C16
#define VC_CUBE_EYE_POS    C19    // in cubemap space
#define VC_EYE_POS         C20    // in object space
#define VC_MAT_SPECPOWER   C21

#define VC_FOGDATA         C22

#define VC_LIGHT_POS1      C23
#define VC_LIGHT_DIR1      C24
#define VC_LIGHT_DIFFUSE1  C25
#define VC_LIGHT_SPEC1     C26

#define VC_LIGHT_POS2      C27
//#define VC_LIGHT_DIR2      C28
//#define VC_LIGHT_DIFFUSE2  C29
//#define VC_LIGHT_SPEC2     C30
#define VC_LIGHT_TRANS2     C31

//#define VC_LIGHT_POS4      C35
//#define VC_LIGHT_DIR4      C36
//#define VC_LIGHT_DIFFUSE4  C37
//#define VC_LIGHT_SPEC4     C38

#define VC_DETAIL_SCALE    C40
    

#define PC_MAT_SPECCOLOR   C0
#define PC_MAT_SPECPOWER   C1
#define PC_DIFF_COLOR      C2
#define PC_AMBIENT_COLOR   C3
#define PC_ACCUM_TIME      C4
#define PC_DIFF_COLOR2     C5

#define PC_COLORMULTIPLY   C6

#define PC_USERDEF1        C7
#define PC_USERDEF2        C8

#define PC_VISIBILITY      C9

// Mirror of above.  Couldn't be cleaner because HLSL doesn't support function macros
#else

#define VC_WORLD_PROJ      0

#define VC_TEX_TRANS1      4
#define VC_LIGHT_TRANS     8
#define VC_OBJ_TRANS       12

#define VC_CUBE_TRANS      16
#define VC_CUBE_EYE_POS    19    // in cubemap space
#define VC_EYE_POS         20    // in object space
#define VC_MAT_SPECPOWER   21

#define VC_FOGDATA         22

#define VC_LIGHT_POS1      23
#define VC_LIGHT_DIR1      24
#define VC_LIGHT_DIFFUSE1  25
#define VC_LIGHT_SPEC1     26

#define VC_LIGHT_POS2      27
//#define VC_LIGHT_DIR2      28
//#define VC_LIGHT_DIFFUSE2  29
//#define VC_LIGHT_SPEC2     30
#define VC_LIGHT_TRANS2     31

//#define VC_LIGHT_POS4      35
//#define VC_LIGHT_DIR4      36
//#define VC_LIGHT_DIFFUSE4  37
//#define VC_LIGHT_SPEC4     38

#define VC_DETAIL_SCALE    40



#define PC_MAT_SPECCOLOR   0
#define PC_MAT_SPECPOWER   1
#define PC_DIFF_COLOR      2
#define PC_AMBIENT_COLOR   3
#define PC_ACCUM_TIME      4
#define PC_DIFF_COLOR2     5

#define PC_COLORMULTIPLY   6

#define PC_USERDEF1        7
#define PC_USERDEF2        8

#define PC_VISIBILITY      9

#endif

