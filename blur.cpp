#include "sharptrails.h"
#include "d3d9.h"
#include "d3d9types.h"

HMODULE dllModule;
int gtaversion = -1;

static uint32_t DefinedState_A = AddressByVersion<uint32_t>(0x526330, 0x526570, 0x526500, 0x57F9C0, 0x57F9E0, 0x57F7F0);
WRAPPER void DefinedState(void) { VARJMP(DefinedState_A); }

//void *overridePS = NULL;
void *blurps = NULL;

class CMBlur {
public:
	static bool &ms_bJustInitialised;
	static bool &BlurOn;
	static float &Drunkness;
	static RwRaster *&pFrontBuffer;
	static void MotionBlurRender(RwCamera *cam, RwUInt8 red, RwUInt8 green, RwUInt8 blue, RwUInt8 alpha, int type);
	static void OverlayRenderVC_noblur(RwCamera *cam, RwRaster *raster, RwRGBA color, int type);
	static void OverlayRenderIII_noblur(RwCamera *cam, RwRaster *raster, RwRGBA color, int type, int alpha);
	static void OverlayRenderVC(RwCamera *cam, RwRaster *raster, RwRGBA color, int type);
	static void OverlayRenderIII(RwCamera *cam, RwRaster *raster, RwRGBA color, int type, int alpha);
	static void OverlayRenderFx(RwCamera *cam, RwRaster *raster);
};

//bool &CMBlur::ms_bJustInitialised = *(bool*)0xA10B71;
bool &CMBlur::BlurOn = *AddressByVersion<bool*>(0x95CDAD, 0x5FDB84, 0x60AB7C, 0x697D54, 0x697D54, 0x696D5C);
//float &CMBlur::Drunkness = *(float*)0x983B38;
RwRaster *&CMBlur::pFrontBuffer = *AddressByVersion<RwRaster**>(0x8E2C48, 0x8E2CFC, 0x8F2E3C, 0x9753A4, 0x9753AC, 0x9743AC);

static uint32_t OverlayRenderFx_A = AddressByVersion<uint32_t>(0, 0, 0, 0x55D870, 0x55D890, 0x55D760);
WRAPPER void CMBlur::OverlayRenderFx(RwCamera*, RwRaster*) { VARJMP(OverlayRenderFx_A); }
static uint32_t OverlayRenderVC_A = AddressByVersion<uint32_t>(0, 0, 0, 0x55E750, 0x55E770, 0x55E640);
WRAPPER void CMBlur::OverlayRenderVC(RwCamera*, RwRaster*, RwRGBA, int) { VARJMP(OverlayRenderVC_A); }
static uint32_t OverlayRenderIII_A = AddressByVersion<uint32_t>(0x50A9C0, 0x50AAA0, 0x50AA30, 0, 0, 0);
WRAPPER void CMBlur::OverlayRenderIII(RwCamera*, RwRaster*, RwRGBA, int, int) { VARJMP(OverlayRenderIII_A); }


/*
void
CMBlur::MotionBlurRender(RwCamera *cam, RwUInt8 red, RwUInt8 green, RwUInt8 blue, RwUInt8 alpha, int type)
{
	RwRGBA color = { red, green, blue, alpha };
	if(CMBlur::ms_bJustInitialised)
		CMBlur::ms_bJustInitialised = 0;
	else
		CMBlur::OverlayRender_noblur(cam, CMBlur::pFrontBuffer, color, type);
	if(CMBlur::BlurOn){
		RwRasterPushContext(CMBlur::pFrontBuffer);
		RwRasterRenderFast(cam->frameBuffer, 0, 0);
		RwRasterPopContext();
	}
}
*/

RwD3D8Vertex *blurVertices = AddressByVersion<RwD3D8Vertex*>(0x62F780, 0x62F780, 0x63F780, 0x7097A8, 0x7097A8, 0x7087A8);
//RwD3D8Vertex *blurVertices2 = (RwD3D8Vertex*)0x709818;
RwImVertexIndex *blurIndices = AddressByVersion<RwImVertexIndex*>(0x5FDD90, 0x5FDB78, 0x60AB70, 0x697D48, 0x697D48, 0x696D50);

void
setps(void)
{
	if(blurps == NULL){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(isVC() ? IDR_VCBLURPS : IDR_IIIBLURPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &blurps);
		assert(blurps);
		FreeResource(shader);
	}
	RwD3D9SetIm2DPixelShader(blurps);
}

int dontblur = 1;

void
CMBlur::OverlayRenderVC_noblur(RwCamera *cam, RwRaster *raster, RwRGBA color, int type)
{
	if(0 && type == 1){
		color.red = 180;
		color.green = 255;
		color.blue = 180;
		color.alpha = 120;

		DefinedState();
		RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)1);
		RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
		RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
		RwUInt32 emissiveColor = D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue);
		for(int i = 0; i < 4; i++)
			blurVertices[i].emissiveColor = emissiveColor;
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
		return;
	}

	if(type != 2 || !CMBlur::BlurOn || !dontblur || !RwD3D9Supported()){
		CMBlur::OverlayRenderVC(cam, raster, color, type);
		return;
	}

	RwRasterPushContext(CMBlur::pFrontBuffer);
	RwRasterRenderFast(cam->frameBuffer, 0, 0);
	RwRasterPopContext();

	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)1);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwUInt32 emissiveColor = D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue);
	for(int i = 0; i < 4; i++)
		blurVertices[i].emissiveColor = emissiveColor;
	setps();
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
	RwD3D9SetIm2DPixelShader(NULL);

	if(type != 1){
		RwRasterPushContext(CMBlur::pFrontBuffer);
		RwRasterRenderFast(cam->frameBuffer, 0, 0);
		RwRasterPopContext();
		CMBlur::OverlayRenderFx(cam, CMBlur::pFrontBuffer);
	}
}

/*
void
CMBlur::OverlayRender(RwCamera *cam, RwRaster *raster, RwRGBA color, int type)
{
	RwUInt8 red, green, blue, alpha;
	red = color.red;
	green = color.green;
	blue = color.blue;
	alpha = color.alpha;
	DefinedState();
	switch(type){
	case 3:
		red = 0;
		green = 0xFF;
		blue = 0;
		alpha = 0x80;
		break;
	case 5:
		red = 0x64;
		green = 0xDC;
		blue = 0xE6;
		alpha = 0x9E;
		break;
	case 6:
		red = 0x50;
		green = 0xFF;
		blue = 0xE6;
		alpha = 0x8A;
		break;
	case 8:
		red = 0xFF;
		green = 0x3C;
		blue = 0x3C;
		alpha = 0xC8;
		break;
	case 9:
		red = 0xFF;
		green = 0xB4;
		blue = 0xB4;
		alpha = 0x80;
		break;
	}
	printf("blurtype: %d\n", type);
	if(!CMBlur::BlurOn){
		assert(0 && "no blur");
	}
	RwUInt32 emissiveColor = D3DCOLOR_ARGB(alpha, red, green, blue);
	for(int i = 0; i < 4; i++)
		blurVertices[i].emissiveColor = blurVertices2[i].emissiveColor = emissiveColor;
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)1);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	if(CMBlur::BlurOn){
		if(type == 1){
			assert(0 && "type 1");
			emissiveColor = D3DCOLOR_ARGB(0x50, red, green, blue);
			for(int i = 0; i < 4; i++)
				blurVertices2[i].emissiveColor = emissiveColor;
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
			*(int*)0x97F888 = 0;	// what's that?
		}else{
			emissiveColor = D3DCOLOR_ARGB(0x1E, red*2, green*2, blue*2);
			for(int i = 0; i < 4; i++)
				blurVertices[i].emissiveColor = blurVertices2[i].emissiveColor = emissiveColor;
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
			RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
			RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
			emissiveColor = D3DCOLOR_ARGB(alpha, red, green, blue);
			for(int i = 0; i < 4; i++)
				blurVertices[i].emissiveColor = blurVertices2[i].emissiveColor = emissiveColor;
			setps();
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);	//
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
			overridePS = NULL;
		}
	}
	if(CMBlur::Drunkness * 175.0f){
		assert(0 && "drunk");
	}
	if((GetAsyncKeyState(VK_F4) & 0x8000) == 0 && type != 1)
		CMBlur::OverlayRenderFx(cam, raster);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}
*/

void
CMBlur::OverlayRenderIII_noblur(RwCamera *cam, RwRaster *raster, RwRGBA color, int type, int alpha)
{
	//{
	//	static bool keystate = false;
	//	if(GetAsyncKeyState(VK_F4) & 0x8000){
	//		if(!keystate){
	//			dontblur = !dontblur;
	//			keystate = true;
	//		}
	//	}else
	//		keystate = false;
	//}
	if(!CMBlur::BlurOn || !dontblur || !RwD3D9Supported()){
		CMBlur::OverlayRenderIII(cam, raster, color, type, alpha);
		return;
	}

	RwRasterPushContext(CMBlur::pFrontBuffer);
	RwRasterRenderFast(cam->frameBuffer, 0, 0);
	RwRasterPopContext();

	//float mult[3], add[3];
	//mult[0] = (color.red-64)/384.0f + 1.14f;
	//mult[1] = (color.green-64)/384.0f + 1.14f;
	//mult[2] = (color.blue-64)/384.0f + 1.14f;
	//add[0] = color.red/1536.f;
	//add[1] = color.green/1536.f;
	//add[2] = color.blue/1536.f;
	//RwD3D9SetPixelShaderConstant(3, &mult, 1);
	//RwD3D9SetPixelShaderConstant(4, &add, 1);

	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)1);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwUInt32 emissiveColor = D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue);
	for(int i = 0; i < 4; i++)
		blurVertices[i].emissiveColor = emissiveColor;
	setps();
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
	RwD3D9SetIm2DPixelShader(NULL);
}

void
patch(void)
{

	if(isVC())
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x55D838, 0x55D858, 0x55D728), CMBlur::OverlayRenderVC_noblur);
	else{
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x50ADDD, 0x50AEBD, 0x50AE4D, 0, 0, 0), CMBlur::OverlayRenderIII_noblur);
		MemoryVP::Nop(AddressByVersion<uint32_t>(0x50AE2F, 0x50AF0F, 0x50AE9F, 0, 0, 0), 5);
	}
}

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		dllModule = hInst;

/*		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);*/

		AddressByVersion<uint32_t>(0, 0, 0, 0, 0, 0);			

		//if(gtaversion == III_10 || gtaversion == III_11 || gtaversion == VC_10)
		if (gtaversion != -1)
		{
			if (isVC())
			{
				MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x48FEAF, 0x48FEBF, 0x48FDBF), enableTrailSetting, PATCH_CALL);

				//CMBlur::AddRenderFx Type 4
				//MemoryVP::Nop(AddressByVersion<uint32_t>(0, 0, 0, 0x560FF9, 0x561019, 0x560EE9), 5);
				//MemoryVP::Nop(AddressByVersion<uint32_t>(0, 0, 0, 0x561259, 0x561279, 0x561149), 5);

				char			dllName[MAX_PATH];
				GetModuleFileName(hInst, dllName, MAX_PATH);
				char* tempPointer = strrchr(dllName, '\\');
				if (strstr(tempPointer + 1, "sharp") != NULL) {
					patch();
				}
			}
			else
				patch();
		}
	}

	return TRUE;
}
