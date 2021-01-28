/**********************************************************************
 *<
	FILE: interval.h

	DESCRIPTION:  Defines TimeValue and Interval Classes

	CREATED BY: Rolf Berteig

	HISTORY: created 13 September 1994
             950818 - Added methods for setting start/end individually (gus)

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/


#ifndef _INTERVAL_H_
#define _INTERVAL_H_


class Interval {
	private:
		TimeValue	start;
		TimeValue	end;

	public:		
		/* 
		 Constructors:
		 */		
		CoreExport Interval( TimeValue s, TimeValue e );
		Interval() { SetEmpty(); } 

		int operator==( const Interval& i ) { return( i.start==start && i.end==end ); }
		CoreExport int InInterval(const TimeValue t);
		int InInterval(const Interval interval) { return InInterval( interval.Start() ) && InInterval( interval.End() ); }
		int Empty() { return (start == TIME_NegInfinity) && (end == TIME_NegInfinity); }

		void Set       ( TimeValue s, TimeValue e ) { start = s; end = e; }
		void SetStart  ( TimeValue s )              { start = s;          }
		void SetEnd    ( TimeValue e )              { end = e;            }

		void SetEmpty() { start = TIME_NegInfinity; end = TIME_NegInfinity; }
		void SetInfinite() { start = TIME_NegInfinity; end = TIME_PosInfinity; }
		void SetInstant(const TimeValue t) { start = end = t; }
		TimeValue Start() const { return start; }
		TimeValue End() const { return end; }
		TimeValue Duration() const { return end-start+TimeValue(1); } // end points included

		// intersection of intervals
		CoreExport Interval operator&(const Interval i) const;
		Interval& operator&=(const Interval i) { return (*this = (*this&i)); }
		Interval& operator+=(const TimeValue t) { if (t<start) start=t; if (t>end) end=t; return *this; }
};

#define FOREVER Interval(TIME_NegInfinity, TIME_PosInfinity)
#define NEVER Interval(TIME_NegInfinity, TIME_NegInfinity)

#endif


