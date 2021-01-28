#ifndef VEDGE_H_DEFINED
#define  VEDGE_H_DEFINED

class VEdge {
	DWORD f[2];
	public:
		void SetFace(int i, DWORD n) { f[i] =(f[i]&0xc0000000)|n;}
		void SetWhichSide(int i, int s) { f[i] =(f[i]&0x3FFFFFFF)|((s&3)<<30); }
		DWORD GetFace(int i){ return f[i]&0x3fffffff; }
		int GetWhichSide(int i) { return (f[i]>>30)&3; }
	};

typedef struct {
	unsigned short flags;
	DWORD v[2];  /* indices of two vertices defining edge */
	DWORD f[2];  /* indices of two neighboring faces  */
	} Edge;


// Moved from VEDGE.CPP -TH
#define UNDEF 0xFFFFFFFF
#define UNDEF_FACE 0x3FFFFFFF

#endif //  VEDGE_H_DEFINED
