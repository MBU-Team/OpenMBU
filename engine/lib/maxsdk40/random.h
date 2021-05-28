/**********************************************************************
 *<
        FILE: random.h

        DESCRIPTION: Pseudo-random number generator

        CREATED BY: Jeff Kowalski

        HISTORY: Created 11 February 1999

 *>     Copyright (c) 1999, All Rights Reserved.
 **********************************************************************/

#ifndef __RANDOM__H
#define __RANDOM__H

/*
 * This class defines a Pseudo-random number generator that precisely
 * matches the behavior of the MSVCRT 6.0 random routines.
 * That is to say, for equivalent calls to ::srand() and Random::srand(),
 * both ::rand() and Random::rand() will produce the same results.
 *
 * The benefit, however, in having this class is that each instantiation
 * is independent, permitting several uncoupled random number generators
 * to be present in the system at once.  Moreover, each instantiation
 * is automatically "pre-seeded", making calls to Random::srand unnecessary
 * in most uses.  Even arrays of Random items will operate independently.
 *
 * In addition to providing the analogues to the "stdlib" functions, this
 * class also provides two useful member functions which can be used
 * to get a random number bounded in either a float or int interval.
 */
class Random {
    private:
        long   m_seed;

    public:
        // The constructor will automatically initialize the seed
        UtilExport Random ();

        // Analogues of the random rountines from MSVCRT:
        UtilExport void    srand (unsigned int seed = 1); // akin to global ::srand
        UtilExport int     rand  ();                      // akin to global ::rand
        UtilExport static const int s_rand_max;           // akin to global RAND_MAX

        // Returns a random number in the half-open interval [min, max)
        //      such that r=get(max, min)  :=  min <= r < max
        // Note that max is the first arg, and min is the second, permitting
        // one to select, for example, an int in [0,5) = [0,4] with "get(5)"
        // With no arguments, Random::get() is equivalent to Random::rand()
        inline int   get  (int   max_exclusive = s_rand_max+1,
                           int   min_inclusive = 0) {
            return this->rand() %
                     (max_exclusive - min_inclusive) + min_inclusive;
        }

        // Returns a random number in the half-open interval [min, max)
        //      such that r=get(max, min)  :=  min <= r < max
        // Note that max is the first arg, and min is the second, permitting
        // one to select, for example, a float in [0.0, 5.0) with "getf(5)"
        // With no arguments, Random::getf() returns a float in [0.0, 1.0)
        inline float getf (float max_exclusive = 1.0f,
                           float min_inclusive = 0.0f) {
            return this->rand() / (s_rand_max+1.0f) *
                     (max_exclusive - min_inclusive) + min_inclusive;
        }
};

#endif // __RANDOM__H
