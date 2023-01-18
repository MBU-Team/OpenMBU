//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

/// @addtogroup utility_macros Utility Macros
// @{

#undef  SAFE_DELETE

//-----------------------------------------------------------------------------
/// @brief Safely delete an object and set the pointer to NULL
///
/// @param a Object to delete
/// @see #SAFE_DELETE_ARRAY(), #SAFE_DELETE_OBJECT(), #SAFE_FREE(), #SAFE_FREE_REFERENCE()
//-----------------------------------------------------------------------------
#define SAFE_DELETE(a) {delete (a); (a) = NULL; }

#undef  SAFE_DELETE_ARRAY

//-----------------------------------------------------------------------------
/// @brief Safely delete an array and set the pointer to NULL
///
/// @param a Array to delete
/// @see #SAFE_DELETE(), #SAFE_DELETE_OBJECT(), #SAFE_FREE(), #SAFE_FREE_REFERENCE()
//-----------------------------------------------------------------------------
#define SAFE_DELETE_ARRAY(a) { delete [] (a); (a) = NULL; }

#undef  SAFE_DELETE_OBJECT

//-----------------------------------------------------------------------------
/// @brief Safely delete a SimObject and set the pointer to NULL
///
/// @param a Object to delete
/// @see #SAFE_DELETE_ARRAY(), #SAFE_DELETE(), #SAFE_FREE(), #SAFE_FREE_REFERENCE()
//-----------------------------------------------------------------------------
#define SAFE_DELETE_OBJECT(a) { if( (a) != NULL ) (a)->deleteObject(); (a) = NULL; }

#undef  SAFE_FREE

//-----------------------------------------------------------------------------
/// @brief Safely free memory and set the pointer to NULL
///
/// @param a Pointer to memory to free
/// @see #SAFE_DELETE_ARRAY(), #SAFE_DELETE_OBJECT(), #SAFE_DELETE(), #SAFE_FREE_REFERENCE()
//-----------------------------------------------------------------------------
#define SAFE_FREE(a) { if( (a) != NULL ) dFree ((void *)a); (a) = NULL; }

// CodeReview: Is the NULL conditional needed? [5/14/2007 Pat]

#undef  SAFE_FREE_REFERENCE

//-----------------------------------------------------------------------------
/// @brief Safely free a reference to a Message and set the pointer to NULL
///
/// @param a Pointer to message to free
/// @see #SAFE_DELETE_ARRAY(), #SAFE_DELETE_OBJECT(), #SAFE_FREE(), #SAFE_DELETE()
//-----------------------------------------------------------------------------
#define SAFE_FREE_REFERENCE(a) { if((a) != NULL) (a)->freeReference(); (a) = NULL; }

#undef  SAFE_DELETE_MESSAGE

//-----------------------------------------------------------------------------
/// @brief Synonym for SAFE_FREE_REFERENCE()
///
/// @param a Object to delete
/// @see #SAFE_DELETE(), #SAFE_DELETE_ARRAY(), #SAFE_DELETE_OBJECT(), #SAFE_FREE(), #SAFE_FREE_REFERENCE()
//-----------------------------------------------------------------------------
#define SAFE_DELETE_MESSAGE   SAFE_FREE_REFERENCE

// @}
