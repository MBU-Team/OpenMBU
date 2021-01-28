/**********************************************************************
 *<
	FILE: stack.h

	DESCRIPTION: Simple stack using Tab.

	CREATED BY:	Rolf Berteig

	HISTORY: Created 22 November 1994

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/

#ifndef __STACK__
#define __STACK__

template<class T> class Stack {
	private:
		Tab<T> s;		
	
	public:		
		// access the stack indexing from the top down.
		T& operator[](const int i) const { 
			assert(s.Count()-i>0);
			return s[s.Count()-i-1];
			}

		void Push( T *el ) { 
			s.Append( 1, el ); 			
			}

		void Pop( T *el ) { 
			assert( s.Count() );	
			*el = s[s.Count()-1];
			s.Delete( s.Count()-1, 1 );			
			}

		void Pop() { 
			assert( s.Count() );				
			s.Delete( s.Count()-1, 1 );			
			}

		void GetTop( T *el ) {
			assert( s.Count() );	
			*el = s[s.Count()-1];
			}

		void Clear() {
			s.Delete(0,s.Count());			
			}

		int Count() {
			return s.Count(); 
			}

		int Remove( int i ) {
			assert(i<s.Count());
			return s.Delete(s.Count()-1-i,1);
			}
	};

#endif // __STACK__

