#ifndef _BUILD_VER_

#define _BUILD_VER_

// Don't use! Edu version of MAX now keyed off serial number
// Define EDU_VERSION to build the educational version of MAX
//#define EDU_VERSION

// Define BETA_VERSION to use Beta lock
//#define BETA_VERSION

// Define STUDENT_VER to build the student version of MAX
// #define STUDENT_VER
#ifdef STUDENT_VER
#define WIN95_ONLY
#endif

//TURN ON SNAPPING FOR INTEGRATION TO ATHENA
#define _OSNAP TRUE

//TURN ON PRIMITIVE CREATION WITH 3D SNAPPING
#define _3D_CREATE

// Turn on sub material assignment : 1/19/98 - CCJ
#define _SUBMTLASSIGNMENT

// Define DESIGN_VER to build the design version of MAX
// #define DESIGN_VER

// Define to build a version with no NURBS
// #define NO_NURBS

// Define APL_DBCS for double-byte character set versions (i.e. Japanese, Chinese)
//#define APL_DBCS

// no longer used by MAX
#if !defined(EDU_VERSION) && !defined(STUDENT_VER) && !defined(DESIGN_VER) && !defined(BETA_VERSION)
#define ORDINARY_VER
#endif

// errors that will no longer occur
#if defined(EDU_VERSION) && defined(STUDENT_VER)
#error "Both EDU_VERSION and STUDENT_VER defined in buildver.h!"
#endif

#if defined(EDU_VERSION) && defined(BETA_VERSION)
#error "Both EDU_VERSION and BETA_VERSION defined in buildver.h!"
#endif

#endif // _BUILD_VER_


