/**********************************************************************
 *<
	FILE: IDX8PixelShader.h

	DESCRIPTION: DirectX 8 Pixel Shader Interface Definition

	CREATED BY: Nikolai Sander and Norbert Jeske

	HISTORY: Created 9/27/00

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#include <d3dx8.h>

#define DX8_PIXEL_SHADER_INTERFACE_ID Interface_ID(0x56df1953, 0xc6121a3)

class ID3DGraphicsWindow;
class IDX8VertexShader;
class Material;

class IDX8PixelShader: public BaseInterface
{
public:
	virtual Interface_ID GetID() { return DX8_PIXEL_SHADER_INTERFACE_ID; }

	// Confirm that the Direct3D Device can handle this PixelShader
	virtual HRESULT ConfirmDevice(ID3DGraphicsWindow *gw) = 0;

	// Confirm that an associated VertexShader will work with this PixelShader
	virtual HRESULT ConfirmVertexShader(IDX8VertexShader *pvs) = 0;

	// Load PixelShader instructions and textures.  PixelShader instructions
	// should be loaded once and shared among all the nodes using this
	// PixelShader.  In addition, any textures necessary for the PixelShader
	// effect should be loaded once and shared among all the nodes using this
	// PixelShader.
	virtual HRESULT Initialize(Material *mtl, INode *node) = 0;

	// Number of passes for the effect this PixelShader creates.  Note that
	// this value will depend on the hardware currently in use.
	virtual int GetNumMultiPass() = 0;

	// Retrieve the PixelShader handle for the specified pass for use in GFX
	virtual DWORD GetPixelShaderHandle(int numPass) = 0;

	// Set the PixelShader for the specified pass.  This call will be made at
	// least once per object to set the per object data for the PixelShader
	// such as the PixelShader constants.
	virtual HRESULT SetPixelShader(ID3DGraphicsWindow *gw, int numPass) = 0;
};
