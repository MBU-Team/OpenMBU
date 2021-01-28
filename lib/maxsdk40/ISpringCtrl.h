/**********************************************************************
 *<
	FILE: jiggleAPI.h

	DESCRIPTION:	Public header file for Spring controller

	CREATED BY:		Adam Felt

	HISTORY: 

 *>	Copyright (c) 1999-2000, All Rights Reserved.
 **********************************************************************/


#ifndef IJIGGLE
#define IJIGGLE

#define JIGGLEPOS 0x79697d2a
#define JIGGLEP3 0x13892172

#define JIGGLE_POS_CLASS_ID	Class_ID(JIGGLEPOS, 0xf2b8a1c8)
#define JIGGLE_P3_CLASS_ID	Class_ID(JIGGLEP3, 0x68976279)

#define JIGGLE_CONTROL_REF  0
#define JIGGLE_PBLOCK_REF1	1
#define JIGGLE_PBLOCK_REF2  2

//parameter defaults
#define JIGGLE_DEFAULT_TENSION 2.0f
#define JIGGLE_DEFAULT_DAMPENING 0.5f
#define JIGGLE_DEFAULT_MASS 300.0f
#define JIGGLE_DEFAULT_DRAG 1.0f

#define SET_PARAMS_RELATIVE  0
#define SET_PARAMS_ABSOLUTE  1


class IJiggle : public FPMixinInterface
{
	public:
		enum { get_mass, set_mass, get_drag, set_drag, get_tension, set_tension, get_dampening, set_dampening,
			   add_spring, get_spring_count, remove_spring_by_index, remove_spring, get_spring_system, };

		BEGIN_FUNCTION_MAP
			FN_0	(get_mass, TYPE_FLOAT, GetMass);
			VFN_1	(set_mass, SetMass, TYPE_FLOAT);
			FN_0	(get_drag, TYPE_FLOAT, GetDrag);
			VFN_1	(set_drag, SetDrag, TYPE_FLOAT);
			FN_1	(get_tension, TYPE_FLOAT, GetTension, TYPE_INDEX);
			VFN_2	(set_tension, SetTension, TYPE_INDEX, TYPE_FLOAT);
			FN_1	(get_dampening, TYPE_FLOAT, GetDampening, TYPE_INDEX);
			VFN_2	(set_dampening, SetDampening, TYPE_INDEX, TYPE_FLOAT);

			FN_1	(add_spring, TYPE_BOOL, AddSpring, TYPE_INODE);
			FN_0	(get_spring_count, TYPE_INT, GetSpringCount);
			VFN_1	(remove_spring_by_index, RemoveSpring, TYPE_INDEX);
			VFN_1	(remove_spring, RemoveSpring, TYPE_INODE);
			//FN_0	(get_spring_system, TYPE_INTERFACE, GetSpringSystem);
		END_FUNCTION_MAP

		FPInterfaceDesc* GetDesc();    // <-- must implement 
		
		SpringSys* partsys;
		virtual SpringSys* GetSpringSystem()=0;

		virtual float	GetMass()=0;
		virtual void	SetMass(float mass, bool update=true)=0;
		virtual float	GetDrag()=0;
		virtual void	SetDrag(float drag, bool update=true)=0;
		virtual float	GetTension(int index)=0;
		virtual void	SetTension(int index, float tension, int absolute=1, bool update=true)=0;
		virtual float	GetDampening(int index)=0;
		virtual void	SetDampening(int index, float dampening, int absolute=1, bool update=true)=0;

		virtual BOOL	AddSpring(INode *node)=0;
		virtual INT		GetSpringCount()=0;
		virtual void	RemoveSpring(int which)=0;
		virtual void	RemoveSpring(INode *node)=0;
};

#endif //IJIGGLE