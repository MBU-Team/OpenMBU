/**********************************************************************
 *<
	FILE: RandGenerator.h

	DESCRIPTION: Generator of random numbers

	CREATED BY: Oleg Bayborodin

	HISTORY: Nov.17 2000 - created

	GOALS: To solve the need in satisfactory generator of random numbers.

	DESCRIPTION: The class has interfaces for srand/rand methods of VC++ 
		and other functions for random number generation.

		Srand/rand methods from stdlib.h have two main problems:
		a) It's not satisfactory random. Since rand() function returns a 
			pseudorandom integer in the range 0 to 0x7fff=32767, if we
			need a lot of random numbers (i.e. for generating 100,000 
			particles), we are running out of continuity of random num-
			bers. Generated random numbers became too discrete.
		b) rand() method is global function, not class object. Hence it's
			shared between all modules of your plug-in. Changes in one
			module may change randomness pattern in other independent 
			module. To solve this contradiction, rand methods have to be
			implemented as a class object.

		RandGenerator does exactly that. It has much more random numbers:
		RAND_MAX = 0xFFFFFFFF = 4,294,967,295. Also, using instance of 
		the class, it's much easier to create separate thread of random 
		numbers for specific module.

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef _RANDGENERATOR_H 

#define _RANDGENERATOR_H 


// to fix conflict between RandGenerator and stdlib.h do the following:
#undef RAND_MAX
const unsigned int RAND_MAX = 0x7fff; // as defined in stdlib.h

class RandGenerator 
{
	public:
		CoreExport RandGenerator(); // constructor
	
	// override for VC++ rand methods
		CoreExport static const DWORD32 RAND_MAX;

		// The srand function sets the starting point for generating a 
		// series of pseudorandom integers. To reinitialize the generator, 
		// use 1 as the seed argument. Any other value for seed sets the 
		// generator to a random starting point. rand() retrieves the 
		// pseudorandom numbers that are generated. Calling rand() before 
		// any call to srand() generates the same sequence as calling srand() 
		// with seed passed as 1.
		CoreExport void	srand(DWORD32 seed);
		// The rand function returns a pseudorandom integer in the range 0 to RAND_MAX
		CoreExport DWORD32 rand( void );

	// other useful functions:
		// random sign { -1, 1 }
		CoreExport int		RandSign( void );
		// random number between 0.0f and 1.0f
		CoreExport float	Rand01( void );
		// random number between -1.0f and 1.0f
		CoreExport float	Rand11( void );
		// random number between -0.5f and 0.5f
		CoreExport float	Rand55( void );
		// integer random number between 0 and maxnum
		CoreExport int		Rand0X(int maxnum);

	// to check out if the generator has been explicitely initialized
	// by srand() method
		const bool	Valid( void ) const { return m_explicitelyInitialized; }

	private:
		static const DWORD32 kMagicNumber1, kMagicNumber2;
		static const DWORD32 HALF_RAND_MAX;
		static const double kIntMax, kIntMax1, kHalfIntMax;
		DWORD32 m_randar[256];
		DWORD32 m_randX, m_randY;
		bool m_explicitelyInitialized;
};



#endif
