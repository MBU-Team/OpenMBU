/*********************************************************************
 *<
	FILE: SetKeyMode.h

	DESCRIPTION: Interface to set-key mode related APIs

	CREATED BY:	Rolf Berteig

	HISTORY: Created November 29, 2000

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __SETKEYMODE_H__
#define __SETKEYMODE_H__

#define I_SETKEYMODE  0x00002000

// Gets a pointer to the SetKeyModeInterface interface, the caller should pass a pointer to "Interface"
#define GetSetKeyModeInterface(i)  ((SetKeyModeInterface*)i->GetInterface(I_SETKEYMODE))


class SetKeyModeCallback : public InterfaceServer {
	public:
		virtual void SetKey()=0;				// Called when user executes set key command
		virtual void ShowUI()=0;				// Display set key floater window
		virtual void SetKeyModeStateChanged()=0;// Called when user enters or exits set-key mode
	};

class SetKeyModeInterface : public InterfaceServer {
	public:
		virtual void RegisterSetKeyModeCallback(SetKeyModeCallback *cb)=0;
		virtual void UnRegisterSetKeyModeCallback(SetKeyModeCallback *cb)=0;
		
		virtual void ActivateSetKeyMode(BOOL onOff)=0;
		virtual void AllTracksCommitSetKeyBuffer()=0;
		virtual void AllTracksRevertSetKeyBuffer()=0;
		virtual BOOL AllTracksSetKeyBufferPresent()=0;
	};

#endif //__SETKEYMODE_H__


