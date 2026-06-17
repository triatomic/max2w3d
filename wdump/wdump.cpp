#include <commdlg.h>
#include <windowsx.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cwctype>
#include <commctrl.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <shobjidl.h>
#include <objbase.h>
#include <shellapi.h>
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#include "BufferedFileClass.h"
#include "chunkclass.h"
#include "w3d.h"
#include "vector.h"
#include "simplevec.h"
#include "vector3i.h"
#include "w3dobsolete.h"
#include "vector3.h"
#include "vector2.h"
#include "resource.h"
HWND mainwnd;
HWND treewnd;
HWND listwnd;
HWND filterwnd;
HWND statuswnd;
HWND progresswnd;
HMENU menu;
HACCEL accel;
int mainwidth;
int mainheight;
int splitterX = 300;
bool splitterDragging = false;
static const int SPLITTER_WIDTH = 5;
static const int FILTER_HEIGHT = 22;
static int statusHeight = 0;

enum ViewMode
{
    VIEW_ORIGINAL = 0,
    VIEW_ALPHABETICAL,
    VIEW_BY_TYPE,
    VIEW_FLAT,
};
static ViewMode g_viewMode = VIEW_ORIGINAL;
static std::wstring g_filterText;
#define CLASS_NAME L"WDUMP"
#define WND_TITLE L"wdump"
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
struct ChunkInfo
{
	StringClass name;
	StringClass type;
	StringClass value;
};

struct ChunkData
{
	StringClass name;
	SimpleDynVecClass<ChunkInfo *> data;
	SimpleDynVecClass<ChunkData *> subchunks;
	DynamicVectorClass<StringClass> unknowndata;
	~ChunkData()
	{
		for (int i = 0; i < data.Count(); i++)
		{
			delete data[i];
		}
		for (int i = 0; i < subchunks.Count(); i++)
		{
			delete subchunks[i];
		}
	}
};

struct ChunkDumper
{
	const char *name;
	void(*function) (ChunkLoadClass &cload, ChunkData *data);
};

std::unordered_map<int, ChunkDumper> chunks;
void AddString(ChunkData *data, const char *name, const char *value, const char *type)
{
	ChunkInfo *c = new ChunkInfo;
	c->value = value;
	c->name = name;
	c->type = type;
	data->data.Add(c);
}

void AddVersion(ChunkData *data, uint32 value)
{
	char c[64];
	sprintf(c, "%u.%hu", value >> 16, (uint16)value);
	AddString(data, "Version", c, "string");
}

void AddInt32(ChunkData *data, const char *name, uint32 value)
{
	char c[256];
	sprintf(c, "%u", value);
	AddString(data, name, c, "int32");
}

void AddInt16(ChunkData *data, const char *name, uint16 value)
{
	char c[256];
	sprintf(c, "%hu", value);
	AddString(data, name, c, "int16");
}

void AddInt8(ChunkData *data, const char *name, uint8 value)
{
	char c[256];
	sprintf(c, "%hhu", value);
	AddString(data, name, c, "int8");
}

void AddInt8Array(ChunkData *data, const char *name, uint8 *values, int count)
{
	StringClass str;
	StringClass str2;
	for (int i = 0; i < count; i++)
	{
		str2.Format("%hhu ", values[i]);
		str += str2;
	}
	char c[256];
	sprintf(c, "int8[%d]", count);
	AddString(data, name, str, c);
}

void AddFloat(ChunkData *data, const char *name, float value)
{
	char c[256];
	sprintf(c, "%f", value);
	AddString(data, name, c, "float");
}

void AddInt32Array(ChunkData *data, const char *name, uint32 *values, int count)
{
	StringClass str;
	StringClass str2;
	for (int i = 0; i < count; i++)
	{
		str2.Format("%u ", values[i]);
		str += str2;
	}
	char c[256];
	sprintf(c, "int32[%d]", count);
	AddString(data, name, str, c);
}

void AddFloatArray(ChunkData *data, const char *name, float *values, int count)
{
	StringClass str;
	StringClass str2;
	for (int i = 0; i < count; i++)
	{
		str2.Format("%f ", values[i]);
		str += str2;
	}
	char c[256];
	sprintf(c, "float[%d]", count);
	AddString(data, name, str, c);
}

void AddVector(ChunkData *data, const char *name, W3dVectorStruct *value)
{
	char c[256];
	sprintf(c, "%f %f %f", value->X, value->Y, value->Z);
	AddString(data, name, c, "vector");
}

void AddQuaternion(ChunkData *data, const char *name, W3dQuaternionStruct *value)
{
	char c[256];
	sprintf(c, "%f %f %f %f", value->Q[0], value->Q[1], value->Q[2], value->Q[3]);
	AddString(data, name, c, "quaternion");
}

void AddRGB(ChunkData *data, const char *name, W3dRGBStruct *value)
{
	StringClass str;
	str.Format("(%hhu %hhu %hhu) ", value->R, value->G, value->B);
	AddString(data, name, str, "RGB");
}

void AddRGBArray(ChunkData *data, const char *name, W3dRGBStruct *values, int count)
{
	StringClass str;
	StringClass str2;
	for (int i = 0; i < count; i++)
	{
		str2.Format("(%hhu %hhu %hhu) ", values[i].R, values[i].G, values[i].B);
		str += str2;
	}
	char c[256];
	sprintf(c, "float[%d]", count);
	AddString(data, name, str, c);
}

void AddRGBA(ChunkData *data, const char *name, W3dRGBAStruct *value)
{
	StringClass str;
	str.Format("(%hhu %hhu %hhu %hhu) ", value->R, value->G, value->B, value->A);
	AddString(data, name, str, "RGBA");
}

void AddTexCoord(ChunkData *data, const char *name, W3dTexCoordStruct *value)
{
	char c[256];
	sprintf(c, "%f %f", value->U, value->V);
	AddString(data, name, c, "UV");
}

void AddTexCoordArray(ChunkData *data, const char *name, W3dTexCoordStruct *values, int count)
{
	StringClass str;
	for (int i = 0; i < count; i++)
	{
		char c[256];
		sprintf(c, "%s.TexCoord[%d]", name, i);
		AddTexCoord(data, c, &values[i]);
	}
}

const char *DepthCompareValues[] = { "Pass Never", "Pass Less", "Pass Equal", "Pass Less or Equal", "Pass Greater", "Pass Not Equal", "Pass Greater or Equal", "Pass Always" };
const char *DepthMaskValues[] = { "Write Disable", "Write Enable", "Write Disable", "Write Enable" };
const char *DestBlendValues[] = { "Zero", "One", "Src Color", "One Minus Src Color", "Src Alpha", "One Minus Src Alpha", "Src Color Prefog", "Disable", "Enable", "Scale Fragment", "Replace Fragment" };
const char *PriGradientValues[] = { "Disable", "Modulate", "Add", "Bump-Environment" , "Bump-Environment Luminance", "Modulate 2x" };
const char *SecGradientValues[] = { "Disable", "Enable" };
const char *SrcBlendValues[] = { "Zero", "One", "Src Alpha", "One Minus Src Alpha" };
const char *TexturingValues[] = { "Disable", "Enable" };
const char *DetailColorValues[] = { "Disable", "Detail", "Scale", "InvScale", "Add", "Sub", "SubR", "Blend", "DetailBlend" , "Add Signed", "Add Signed 2x", "Scale 2x", "Mod Alpha Add Color" };
const char *DetailAlphaValues[] = { "Disable", "Detail", "Scale", "InvScale", "Disable", "Enable", "Smooth", "Flat" };
const char *AlphaTestValues[] = { "Alpha Test Disable", "Alpha Test Enable" };

void AddShader(ChunkData *data, const char *name, W3dShaderStruct *value)
{
	char c[256];
	sprintf(c, "%s.DepthCompare", name);
	if (value->DepthCompare < W3DSHADER_DEPTHCOMPARE_PASS_MAX)
	{
		AddString(data, c, DepthCompareValues[value->DepthCompare], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Depth Compare type %x", c, value->DepthCompare);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.DepthMask", name);
	if (value->DepthMask < W3DSHADER_DEPTHMASK_WRITE_MAX)
	{
		AddString(data, c, DepthMaskValues[value->DepthMask], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Depth Mask type %x", c, value->DepthMask);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.DestBlend", name);
	if (value->DestBlend < W3DSHADER_DESTBLENDFUNC_MAX)
	{
		AddString(data, c, DestBlendValues[value->DestBlend], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Dest Blend type %x", c, value->DestBlend);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.PriGradient", name);
	if (value->PriGradient < W3DSHADER_PRIGRADIENT_MAX)
	{
		AddString(data, c, PriGradientValues[value->PriGradient], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Primary Gradient type %x", c, value->PriGradient);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.SecGradient", name);
	if (value->SecGradient < W3DSHADER_SECGRADIENT_MAX)
	{
		AddString(data, c, SecGradientValues[value->SecGradient], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Secondary Gradient type %x", c, value->SecGradient);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.SrcBlend", name);
	if (value->SrcBlend < W3DSHADER_SRCBLENDFUNC_MAX)
	{
		AddString(data, c, SrcBlendValues[value->SrcBlend], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Src Blend type %x", c, value->SrcBlend);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.Texturing", name);
	if (value->Texturing < W3DSHADER_TEXTURING_MAX)
	{
		AddString(data, c, TexturingValues[value->Texturing], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Texturing type %x", c, value->Texturing);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.DetailColor", name);
	if (value->DetailColorFunc < W3DSHADER_DETAILCOLORFUNC_MAX)
	{
		AddString(data, c, DetailColorValues[value->DetailColorFunc], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Detail Color Func type %x", c, value->DetailColorFunc);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.DetailAlpha", name);
	if (value->DetailAlphaFunc < W3DSHADER_DETAILALPHAFUNC_MAX)
	{
		AddString(data, c, DetailAlphaValues[value->DetailAlphaFunc], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Detail Alpha Func type %x", c, value->DetailAlphaFunc);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
	sprintf(c, "%s.AlphaTest", name);
	if (value->AlphaTest < W3DSHADER_ALPHATEST_MAX)
	{
		AddString(data, c, AlphaTestValues[value->AlphaTest], "string");
	}
	else
	{
		StringClass str;
		str.Format("%s Shader unknown Alpha Test type %x", c, value->AlphaTest);
		data->unknowndata.Add(str);
		AddString(data, c, "Unknown", "string");
	}
}

const char *PS2DepthCompareValues[] = { "Pass Never", "Pass Less", "Pass Always", "Pass Less or Equal" };
const char *PS2DepthMaskValues[] = { "Write Disable", "Write Enable", "Write Disable", "Write Enable" };
const char *PS2ABDParamValues[] = { "Src Color", "Dest Color", "Zero" };
const char *PS2CParamValues[] = { "Src Alpha", "Dest Alpha", "One", "Disable", "Enable", "Scale Fragment", "Replace Fragment" };
const char *PS2PriGradientValues[] = { "Disable", "Modulate", "Highlight", "Highlight2", "Disable", "Enable" };
const char *PS2TexturingValues[] = { "Disable", "Enable", "Disable", "Detail", "Scale", "InvScale", "Add", "Sub", "SubR", "Blend", "DetailBlend", "Disable", "Detail", "Scale", "InvScale", "Disable", "Enable", "Smooth", "Flat" };

void AddPS2Shader(ChunkData *data, const char *name, W3dPS2ShaderStruct *value)
{
	char c[256];
	sprintf(c, "%s.DepthCompare", name);
	AddString(data, c, PS2DepthCompareValues[value->DepthCompare], "string");
	sprintf(c, "%s.DepthMask", name);
	AddString(data, c, PS2DepthMaskValues[value->DepthMask], "string");
	sprintf(c, "%s.PriGradient", name);
	AddString(data, c, PS2PriGradientValues[value->PriGradient], "string");
	sprintf(c, "%s.Texturing", name);
	AddString(data, c, PS2TexturingValues[value->Texturing], "string");
	sprintf(c, "%s.AParam", name);
	AddString(data, c, PS2ABDParamValues[value->AParam], "string");
	sprintf(c, "%s.BParam", name);
	AddString(data, c, PS2ABDParamValues[value->BParam], "string");
	sprintf(c, "%s.CParam", name);
	AddString(data, c, PS2CParamValues[value->CParam], "string");
	sprintf(c, "%s.DParam", name);
	AddString(data, c, PS2ABDParamValues[value->DParam], "string");
}

void AddIJK(ChunkData *data, const char *name, Vector3i *value)
{
	char c[256];
	sprintf(c, "%d %d %d", value->I, value->J, value->K);
	AddString(data, name, c, "IJK");
}

void AddIJK16(ChunkData *data, const char *name, Vector3i16 *value)
{
	char c[256];
	sprintf(c, "%d %d %d", value->I, value->J, value->K);
	AddString(data, name, c, "IJK");
}

char *ReadChunkData(ChunkLoadClass &cload)
{
	if (!cload.Cur_Chunk_Length())
	{
		return "";
	}
	char *c = new char[cload.Cur_Chunk_Length()];
	cload.Read(c, cload.Cur_Chunk_Length());
	return c;
}

char table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
void ParseSubchunks(ChunkLoadClass &cload, ChunkData *data)
{
	while (cload.Open_Chunk())
	{
		auto iter = chunks.find(cload.Cur_Chunk_ID());
		if (iter == chunks.end())
		{
			StringClass str;
			str.Format("Unknown Chunk %x", cload.Cur_Chunk_ID());
			data->unknowndata.Add(str);
			ChunkData *d = new ChunkData;
			data->subchunks.Add(d);
			StringClass str2;
			str2.Format("%x", cload.Cur_Chunk_ID());
			d->name = str2;
			char *chunkdata = ReadChunkData(cload);
			StringClass str3;
			for (unsigned int i = 0; i < cload.Cur_Chunk_Length(); i++)
			{
				unsigned char n = chunkdata[i];
				str3 += table[n >> 4];
				str3 += table[n & 0xf];
				str3 += ' ';
			}
			AddString(d, "Chunk Data", str3, "Unknown");
		}
		else
		{
			ChunkData *d = new ChunkData;
			data->subchunks.Add(d);
			d->name = iter->second.name;
			iter->second.function(cload, d);
		}
		cload.Close_Chunk();
	}
}

#define FUNC(id) void dump##id (ChunkLoadClass &cload, ChunkData *data)
FUNC(O_W3D_CHUNK_MATERIALS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMaterialStruct *materials = (W3dMaterialStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dMaterialStruct); i++)
	{
		char c[256];
		sprintf(c, "Material[%d].MaterialName", i);
		AddString(data, c, materials[i].MaterialName, "string");
		sprintf(c, "Material[%d].PrimaryName", i);
		AddString(data, c, materials[i].PrimaryName, "string");
		sprintf(c, "Material[%d].SecondaryName", i);
		AddString(data, c, materials[i].SecondaryName, "string");
		sprintf(c, "Material[%d].RenderFlags", i);
		AddInt32(data, c, materials[i].RenderFlags);
		sprintf(c, "Material[%d].Red", i);
		AddInt8(data, c, materials[i].Red);
		sprintf(c, "Material[%d].Green", i);
		AddInt8(data, c, materials[i].Green);
		sprintf(c, "Material[%d].Blue", i);
		AddInt8(data, c, materials[i].Blue);
	}
	delete[] chunkdata;
}
FUNC(O_W3D_CHUNK_MATERIALS2)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMaterial2Struct *materials = (W3dMaterial2Struct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dMaterial2Struct); i++)
	{
		char c[256];
		sprintf(c, "Material[%d].MaterialName", i);
		AddString(data, c, materials[i].MaterialName, "string");
		sprintf(c, "Material[%d].PrimaryName", i);
		AddString(data, c, materials[i].PrimaryName, "string");
		sprintf(c, "Material[%d].SecondaryName", i);
		AddString(data, c, materials[i].SecondaryName, "string");
		sprintf(c, "Material[%d].RenderFlags", i);
		AddInt32(data, c, materials[i].RenderFlags);
		sprintf(c, "Material[%d].Red", i);
		AddInt8(data, c, materials[i].Red);
		sprintf(c, "Material[%d].Green", i);
		AddInt8(data, c, materials[i].Green);
		sprintf(c, "Material[%d].Blue", i);
		AddInt8(data, c, materials[i].Blue);
		sprintf(c, "Material[%d].Alpha", i);
		AddInt8(data, c, materials[i].Alpha);
		sprintf(c, "Material[%d].PrimaryNumFrames", i);
		AddInt16(data, c, materials[i].PrimaryNumFrames);
		sprintf(c, "Material[%d].SecondaryNumFrames", i);
		AddInt16(data, c, materials[i].SecondaryNumFrames);
	}
	delete[] chunkdata;
}
FUNC(O_W3D_CHUNK_POV_QUADRANGLES)
{
	AddString(data, "Contact Greg if you need to look at this!", "unsupported", "string");
}
FUNC(O_W3D_CHUNK_POV_TRIANGLES)
{
	AddString(data, "Contact Greg if you need to look at this!", "unsupported", "string");
}
FUNC(O_W3D_CHUNK_QUADRANGLES)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Outdated structure", "", "string");
	delete[] chunkdata;
}
FUNC(O_W3D_CHUNK_SURRENDER_TRIANGLES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dSurrenderTriangleStruct *triangles = (W3dSurrenderTriangleStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dSurrenderTriangleStruct); i++)
	{
		char c[256];
		sprintf(c, "Triangle[%d].Attributes", i);
		AddInt32(data, c, triangles[i].Attributes);
		sprintf(c, "Triangle[%d].Gouraud", i);
		AddRGBArray(data, c, triangles[i].Gourad, 3);
		sprintf(c, "Triangle[%d].VertexIndices", i);
		AddInt32Array(data, c, triangles[i].VertexIndices, 3);
		sprintf(c, "Triangle[%d].MaterialIdx", i);
		AddInt32(data, c, triangles[i].MaterialIdx);
		sprintf(c, "Triangle[%d].Normal", i);
		AddVector(data, c, &triangles[i].Normal);
		sprintf(c, "Triangle[%d].TexCoord", i);
		AddTexCoordArray(data, c, triangles[i].TexCoord, 3);
	}
	delete[] chunkdata;
}
FUNC(O_W3D_CHUNK_TRIANGLES)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Obsolete structure", "", "string");
	delete[] chunkdata;
}
FUNC(OBSOLETE_W3D_CHUNK_EMITTER_COLOR_KEYFRAME)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterColorKeyframeStruct *frame = (W3dEmitterColorKeyframeStruct *)chunkdata;
	AddFloat(data, "Time", frame->Time);
	AddRGBA(data, "Color", &frame->Color);
	delete[] chunkdata;
}
FUNC(OBSOLETE_W3D_CHUNK_EMITTER_OPACITY_KEYFRAME)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterOpacityKeyframeStruct *frame = (W3dEmitterOpacityKeyframeStruct *)chunkdata;
	AddFloat(data, "Time", frame->Time);
	AddFloat(data, "Opacity", frame->Opacity);
	delete[] chunkdata;
}
FUNC(OBSOLETE_W3D_CHUNK_EMITTER_SIZE_KEYFRAME)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterSizeKeyframeStruct *frame = (W3dEmitterSizeKeyframeStruct *)chunkdata;
	AddFloat(data, "Time", frame->Time);
	AddFloat(data, "Size", frame->Size);
	delete[] chunkdata;
}
FUNC(OBSOLETE_W3D_CHUNK_SHADOW_NODE)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHModelNodeStruct *node = (W3dHModelNodeStruct *)chunkdata;
	AddString(data, "ShadowMeshName", node->RenderObjName, "string");
	AddInt16(data, "PivotIdx", node->PivotIdx);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_AABTREE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_AABTREE_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMeshAABTreeHeader *header = (W3dMeshAABTreeHeader *)chunkdata;
	AddInt32(data, "NodeCount", header->NodeCount);
	AddInt32(data, "PolyCount", header->PolyCount);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_AABTREE_NODES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMeshAABTreeNode *nodes = (W3dMeshAABTreeNode *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dMeshAABTreeNode); i++)
	{
		char c[256];
		sprintf(c, "Node[%d].Min", i);
		AddVector(data, c, &nodes[i].Min);
		sprintf(c, "Node[%d].Max", i);
		AddVector(data, c, &nodes[i].Max);
		if ((nodes[i].FrontOrPoly0 & 0x80000000) == 0)
		{
			sprintf(c, "Node[%d].Front", i);
			AddInt32(data, c, nodes[i].FrontOrPoly0);
			sprintf(c, "Node[%d].Back", i);
		}
		else
		{
			sprintf(c, "Node[%d].Poly0", i);
			AddInt32(data, c, nodes[i].FrontOrPoly0 & 0x7FFFFFFF);
			sprintf(c, "Node[%d].PolyCount", i);
		}
		AddInt32(data, c, nodes[i].BackOrPolyCount);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_AABTREE_POLYINDICES)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *indices = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Polygon Index[%d]", i);
		AddInt32(data, c, indices[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_AGGREGATE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_AGGREGATE_CLASS_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dAggregateMiscInfo *info = (W3dAggregateMiscInfo *)chunkdata;
	AddInt32(data, "OriginalClassID", info->OriginalClassID);
	AddInt32(data, "Flags", info->Flags);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_AGGREGATE_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dAggregateHeaderStruct *header = (W3dAggregateHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_AGGREGATE_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dAggregateInfoStruct *info = (W3dAggregateInfoStruct *)chunkdata;
	AddString(data, "BaseModelName", info->BaseModelName, "string");
	AddInt32(data, "SubobjectCount", info->SubobjectCount);
	W3dAggregateSubobjectStruct *subobj = (W3dAggregateSubobjectStruct *)(chunkdata + sizeof(W3dAggregateInfoStruct));
	for (unsigned int i = 0; i < info->SubobjectCount; i++)
	{
		char c[256];
		sprintf(c, "SubObject[%u].SubobjectName", i);
		AddString(data, c, subobj[i].SubobjectName, "string");
		sprintf(c, "SubObject[%u].BoneName", i);
		AddString(data, c, subobj[i].BoneName, "string");
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_ANIMATION)
{
	ParseSubchunks(cload, data);
}

const char *ChannelTypes[] = { "X Translation", "Y Translation", "Z Translation", "X Rotation", "Y Rotation", "Z Rotation", "Quaternion", "Timecoded X Translation", "Timecoded Y Translation", "Timecoded Z Translation", "Timecoded Quaternion", "Adaptive Delta X Translation", "Adaptive Delta Y Translation", "Adaptive Delta Z Translation", "Adaptive Delta Quaternion", "Vis" };
FUNC(W3D_CHUNK_ANIMATION_CHANNEL)
{
	char *chunkdata = ReadChunkData(cload);
	W3dAnimChannelStruct *channel = (W3dAnimChannelStruct *)chunkdata;
	AddInt16(data, "FirstFrame", channel->FirstFrame);
	AddInt16(data, "LastFrame", channel->LastFrame);
	if (channel->Flags <= ANIM_CHANNEL_VIS)
	{
		AddString(data, "ChannelType", ChannelTypes[channel->Flags], "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_ANIMATION_CHANNEL Unknown Animation Channel Type %x", channel->Flags);
		data->unknowndata.Add(str);
		AddString(data, "Channel Type", "Unknown", "string");
	}
	AddInt16(data, "Pivot", channel->Pivot);
	AddInt16(data, "VectorLen", channel->VectorLen);
	for (int i = 0; i < channel->LastFrame - channel->FirstFrame; i++)
	{
		for (int j = 0; j < channel->VectorLen; j++)
		{
			StringClass str;
			str.Format("Data[%d][%d]", i, j);
			AddFloat(data, str, channel->Data[j + i * channel->VectorLen]);
		}
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_ANIMATION_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dAnimHeaderStruct *header = (W3dAnimHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddString(data, "HierarchyName", header->HierarchyName, "string");
	AddInt32(data, "NumFrames", header->NumFrames);
	AddInt32(data, "FrameRate", header->FrameRate);
	delete[] chunkdata;
}

const char *BitChannelTypes[] = { "Visibility", "Timecoded Visibility" };
bool UnpackBitChannel(uint8 *data, uint32 bit)
{
	return (data[bit / 8] & (1 << bit % 8)) != 0;
}
FUNC(W3D_CHUNK_BIT_CHANNEL)
{
	char *chunkdata = ReadChunkData(cload);
	W3dBitChannelStruct *channel = (W3dBitChannelStruct *)chunkdata;
	AddInt16(data, "FirstFrame", channel->FirstFrame);
	AddInt16(data, "LastFrame", channel->LastFrame);
	if (channel->Flags <= BIT_CHANNEL_TIMECODED_VIS)
	{
		AddString(data, "ChannelType", BitChannelTypes[channel->Flags], "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_BIT_CHANNEL Unknown Animation Channel Type %x", channel->Flags);
		data->unknowndata.Add(str);
		AddString(data, "Channel Type", "Unknown", "string");
	}
	AddInt16(data, "Pivot", channel->Pivot);
	AddInt8(data, "Default Value", channel->DefaultVal);
	for (int i = 0; i < channel->LastFrame - channel->FirstFrame; i++)
	{
		StringClass str;
		str.Format("Data[%d]", i + channel->FirstFrame);
		bool b = UnpackBitChannel(channel->Data, i);
		AddInt8(data, str, b);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_BOX)
{
	char *chunkdata = ReadChunkData(cload);
	W3dBoxStruct *box = (W3dBoxStruct *)chunkdata;
	AddVersion(data, box->Version);
	AddInt32(data, "Attributes", box->Attributes);
	if (box->Attributes & W3D_BOX_ATTRIBUTE_ORIENTED)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBUTE_ORIENTED", "flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBUTE_ALIGNED)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBUTE_ALIGNED", "flag");
	}
	if (box->Attributes & 4)
	{
		StringClass str;
		str.Format("W3D_CHUNK_BOX Unknown Attribute 0x00000004");
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	if (box->Attributes & 8)
	{
		StringClass str;
		str.Format("W3D_CHUNK_BOX Unknown Attribute 0x00000008");
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PHYSICAL)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PHYSICAL", "flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PROJECTILE)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBTUE_COLLISION_TYPE_PROJECTILE", "flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VIS)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VIS", "flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_CAMERA)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBTUE_COLLISION_TYPE_CAMERA", "flag");
	}
	if (box->Attributes & W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VEHICLE)
	{
		AddString(data, "Attributes", "W3D_BOX_ATTRIBTUE_COLLISION_TYPE_VEHICLE", "flag");
	}
	if (box->Attributes & 0xFFFFFE00)
	{
		StringClass str;
		str.Format("W3D_CHUNK_BOX Unknown Attributes %x", box->Attributes & 0xFFFFFE00);
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	AddString(data, "Name", box->Name, "string");
	AddRGB(data, "Color", &box->Color);
	AddVector(data, "Center", &box->Center);
	AddVector(data, "Extent", &box->Extent);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_COLLECTION)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_COLLECTION_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dCollectionHeaderStruct *header = (W3dCollectionHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddInt32(data, "RenderObjectCount", header->RenderObjectCount);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_COLLECTION_OBJ_NAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Render Object Name", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_COLLISION_NODE)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHModelNodeStruct *node = (W3dHModelNodeStruct *)chunkdata;
	AddString(data, "CollisionMeshName", node->RenderObjName, "string");
	AddInt16(data, "PivotIdx", node->PivotIdx);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DAMAGE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_DAMAGE_COLORS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dDamageColorStruct *colors = (W3dDamageColorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dDamageColorStruct); i++)
	{
		char c[256];
		sprintf(c, "DamageColorStruct[%d].VertexIndex", i);
		AddInt32(data, c, colors[i].VertexIndex);
		sprintf(c, "DamageColorStruct[%d].NewColor",i);
		AddRGB(data, c, &colors[i].NewColor);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DAMAGE_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dDamageStruct *damage = (W3dDamageStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dDamageStruct); i++)
	{
		char c[256];
		sprintf(c, "DamageStruct[%d].NumDamageMaterials", i);
		AddInt32(data, c, damage[i].NumDamageMaterials);
		sprintf(c, "DamageStruct[%d].NumDamageVerts", i);
		AddInt32(data, c, damage[i].NumDamageVerts);
		sprintf(c, "DamageStruct[%d].NumDamageColors", i);
		AddInt32(data, c, damage[i].NumDamageColors);
		sprintf(c, "DamageStruct[%d].DamageIndex", i);
		AddInt32(data, c, damage[i].DamageIndex);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DAMAGE_VERTICES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dDamageVertexStruct *vertices = (W3dDamageVertexStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dDamageVertexStruct); i++)
	{
		char c[256];
		sprintf(c, "DamageVertexStruct[%d].VertexIndex", i);
		AddInt32(data, c, vertices[i].VertexIndex);
		sprintf(c, "DamageVertexStruct[%d].NewVertex", i);
		AddInt32(data, c, vertices[i].VertexIndex);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DAZZLE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_DAZZLE_NAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Dazzle Name", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DAZZLE_TYPENAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Dazzle Type Name", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DCG)
{
	char *chunkdata = ReadChunkData(cload);
	W3dRGBAStruct *colors = (W3dRGBAStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dRGBAStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d].DCG", i);
		AddRGBA(data, c, &colors[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_DIG)
{
	char *chunkdata = ReadChunkData(cload);
	W3dRGBStruct *colors = (W3dRGBStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dRGBStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d].DIG", i);
		AddRGB(data, c, &colors[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_EMITTER_BLUR_TIME_KEYFRAMES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterBlurTimeHeaderStruct *header = (W3dEmitterBlurTimeHeaderStruct *)chunkdata;
	AddInt32(data, "KeyframeCount", header->KeyframeCount);
	AddFloat(data, "Random", header->Random);
	W3dEmitterBlurTimeKeyframeStruct *blurtime = (W3dEmitterBlurTimeKeyframeStruct *)(chunkdata + sizeof(W3dEmitterBlurTimeHeaderStruct));
	for (unsigned int i = 0; i < header->KeyframeCount + 1; i++)
	{
		char c[256];
		sprintf(c, "Time[%u]", i);
		AddFloat(data, c, blurtime[i].Time);
		sprintf(c, "BlurTime[%u]", i);
		AddFloat(data, c, blurtime[i].BlurTime);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_FRAME_KEYFRAMES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterFrameHeaderStruct *header = (W3dEmitterFrameHeaderStruct *)chunkdata;
	AddInt32(data, "KeyframeCount", header->KeyframeCount);
	AddFloat(data, "Random", header->Random);
	W3dEmitterFrameKeyframeStruct *frame = (W3dEmitterFrameKeyframeStruct *)(chunkdata + sizeof(W3dEmitterFrameHeaderStruct));
	for (unsigned int i = 0; i < header->KeyframeCount + 1; i++)
	{
		char c[256];
		sprintf(c, "Time[%u]", i);
		AddFloat(data, c, frame[i].Time);
		sprintf(c, "Frame[%u]", i);
		AddFloat(data, c, frame[i].Frame);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterHeaderStruct *header = (W3dEmitterHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterInfoStruct *info = (W3dEmitterInfoStruct *)chunkdata;
	AddString(data, "Texture Name", info->TextureFilename, "string");
	AddFloat(data, "StartSize", info->StartSize);
	AddFloat(data, "EndSize", info->EndSize);
	AddFloat(data, "Lifetime", info->Lifetime);
	AddFloat(data, "EmissionRate", info->EmissionRate);
	AddFloat(data, "MaxEmissions", info->MaxEmissions);
	AddFloat(data, "VelocityRandom", info->VelocityRandom);
	AddFloat(data, "PositionRandom", info->PositionRandom);
	AddFloat(data, "FadeTime", info->FadeTime);
	AddFloat(data, "Gravity", info->Gravity);
	AddFloat(data, "Elasticity", info->Elasticity);
	AddVector(data, "Velocity", &info->Velocity);
	AddVector(data, "Acceleration", &info->Acceleration);
	AddRGBA(data, "StartColor", &info->StartColor);
	AddRGBA(data, "EndColor", &info->EndColor);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_INFOV2)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterInfoStructV2 *info = (W3dEmitterInfoStructV2 *)chunkdata;
	AddInt32(data, "BurstSize", info->BurstSize);
	AddInt32(data, "CreationVolume.ClassID", info->CreationVolume.ClassID);
	AddFloat(data, "CreationVolume.Value1", info->CreationVolume.Value1);
	AddFloat(data, "CreationVolume.Value2", info->CreationVolume.Value2);
	AddFloat(data, "CreationVolume.Value3", info->CreationVolume.Value3);
	AddInt32(data, "VelRandom.ClassID", info->VelRandom.ClassID);
	AddFloat(data, "VelRandom.Value1", info->VelRandom.Value1);
	AddFloat(data, "VelRandom.Value2", info->VelRandom.Value2);
	AddFloat(data, "VelRandom.Value3", info->VelRandom.Value3);
	AddFloat(data, "OutwardVel", info->OutwardVel);
	AddFloat(data, "VelInherit", info->VelInherit);
	AddShader(data, "Shader", &info->Shader);
	AddInt32(data, "RenderMode", info->RenderMode);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_PROPS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterPropertyStruct *props = (W3dEmitterPropertyStruct *)chunkdata;
	AddInt32(data, "ColorKeyframes", props->ColorKeyframes);
	AddInt32(data, "OpacityKeyframes", props->OpacityKeyframes);
	AddInt32(data, "SizeKeyframes", props->SizeKeyframes);
	AddRGBA(data, "ColorRandom", &props->ColorRandom);
	AddFloat(data, "OpacityRandom", props->OpacityRandom);
	AddFloat(data, "SizeRandom", props->SizeRandom);
	W3dEmitterColorKeyframeStruct *color = (W3dEmitterColorKeyframeStruct *)(chunkdata + sizeof(W3dEmitterPropertyStruct));
	for (unsigned int i = 0; i < props->ColorKeyframes; i++)
	{
		char c[256];
		sprintf(c, "Time[%u]", i);
		AddFloat(data, c, color[i].Time);
		sprintf(c, "Color[%u]", i);
		AddRGBA(data, c, &color[i].Color);
	}
	W3dEmitterOpacityKeyframeStruct *opacity = (W3dEmitterOpacityKeyframeStruct *)(chunkdata + sizeof(W3dEmitterPropertyStruct) + (props->ColorKeyframes * sizeof(W3dEmitterColorKeyframeStruct)));
	for (unsigned int i = 0; i < props->OpacityKeyframes; i++)
	{
		char c[256];
		sprintf(c, "Time[%u]", i);
		AddFloat(data, c, opacity[i].Time);
		sprintf(c, "Opacity[%u]", i);
		AddFloat(data, c, opacity[i].Opacity);
	}
	W3dEmitterSizeKeyframeStruct *size = (W3dEmitterSizeKeyframeStruct *)(chunkdata + sizeof(W3dEmitterPropertyStruct) + (props->ColorKeyframes * sizeof(W3dEmitterColorKeyframeStruct)) + (props->OpacityKeyframes * sizeof(W3dEmitterOpacityKeyframeStruct)));
	for (unsigned int i = 0; i < props->SizeKeyframes; i++)
	{
		char c[256];
		sprintf(c, "Time[%u]", i);
		AddFloat(data, c, size[i].Time);
		sprintf(c, "Size[%u]", i);
		AddFloat(data, c, size[i].Size);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_ROTATION_KEYFRAMES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterRotationHeaderStruct *header = (W3dEmitterRotationHeaderStruct *)chunkdata;
	AddInt32(data, "KeyframeCount", header->KeyframeCount);
	AddFloat(data, "Random", header->Random);
	AddFloat(data, "OrientationRandom", header->OrientationRandom);
	W3dEmitterRotationKeyframeStruct *frame = (W3dEmitterRotationKeyframeStruct *)(chunkdata + sizeof(W3dEmitterRotationHeaderStruct));
	for (unsigned int i = 0; i < header->KeyframeCount + 1; i++)
	{
		char c[256];
		sprintf(c, "Time[%u]", i);
		AddFloat(data, c, frame[i].Time);
		sprintf(c, "Rotation[%u]", i);
		AddFloat(data, c, frame[i].Rotation);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_USER_DATA)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "User Data", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_FAR_ATTENUATION)
{
	char *chunkdata = ReadChunkData(cload);
	W3dLightAttenuationStruct *atten = (W3dLightAttenuationStruct *)chunkdata;
	AddFloat(data, "Far Atten Start", atten->Start);
	AddFloat(data, "Far Atten End", atten->End);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_HIERARCHY)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_HIERARCHY_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHierarchyStruct *header = (W3dHierarchyStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddInt32(data, "NumPivots", header->NumPivots);
	AddVector(data, "Center", &header->Center);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_HLOD)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_HLOD_AGGREGATE_ARRAY)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_HLOD_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHLodHeaderStruct *header = (W3dHLodHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddInt32(data, "LodCount", header->LodCount);
	AddString(data, "Name", header->Name, "string");
	AddString(data, "HTree Name", header->HierarchyName, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_HLOD_LOD_ARRAY)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_HLOD_SUB_OBJECT_ARRAY_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHLodArrayHeaderStruct *header = (W3dHLodArrayHeaderStruct *)chunkdata;
	AddInt32(data, "ModelCount", header->ModelCount);
	AddFloat(data, "MaxScreenSize", header->MaxScreenSize);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_HLOD_PROXY_ARRAY)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_HLOD_LIGHT_ARRAY)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_HLOD_SUB_OBJECT)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHLodSubObjectStruct *obj = (W3dHLodSubObjectStruct *)chunkdata;
	AddString(data, "Name", obj->Name, "string");
	AddInt32(data, "BoneIndex", obj->BoneIndex);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_HMODEL)
{
	ParseSubchunks(cload, data);
}
FUNC(OBSOLETE_W3D_CHUNK_HMODEL_AUX_DATA)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHModelAuxDataStruct *auxdata = (W3dHModelAuxDataStruct *)chunkdata;
	AddInt32(data, "Attributes", auxdata->Attributes);
	AddInt32(data, "MeshCount", auxdata->MeshCount);
	AddInt32(data, "CollisionCount", auxdata->CollisionCount);
	AddInt32(data, "SkinCount", auxdata->SkinCount);
	AddInt32Array(data, "FutureCounts", auxdata->FutureCounts, 8);
	AddFloat(data, "LODMin", auxdata->LODMin);
	AddFloat(data, "LODMax", auxdata->LODMax);
	AddInt32Array(data, "FutureUse", auxdata->FutureUse, 32);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_HMODEL_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHModelHeaderStruct *header = (W3dHModelHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddString(data, "HierarchyName", header->HierarchyName, "string");
	AddInt16(data, "NumConnections", header->NumConnections);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_LIGHT)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_LIGHT_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dLightStruct *light = (W3dLightStruct *)chunkdata;
	int type = light->Attributes & W3D_LIGHT_ATTRIBUTE_TYPE_MASK;
	if (type == W3D_LIGHT_ATTRIBUTE_POINT)
	{
		AddString(data, "Attributes", "W3D_LIGHT_ATTRIBUTE_POINT", "string");
	}
	else if (type == W3D_LIGHT_ATTRIBUTE_SPOT)
	{
		AddString(data, "Attributes", "W3D_LIGHT_ATTRIBUTE_SPOT", "string");
	}
	else if (type == W3D_LIGHT_ATTRIBUTE_DIRECTIONAL)
	{
		AddString(data, "Attributes", "W3D_LIGHT_ATTRIBUTE_DIRECTIONAL", "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_LIGHT_INFO Unknown Light Type %x", type);
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	if (light->Attributes & 0x100)
	{
		AddString(data, "Attributes", "W3D_LIGHT_ATTRIBUTE_CAST_SHADOWS", "flag");
	}
	if (light->Attributes & 0xFFFFFE00)
	{
		StringClass str;
		str.Format("W3D_CHUNK_LIGHT_INFO Unknown Light Flags %x", light->Attributes & 0xFFFFFE00);
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	AddRGBArray(data, "Ambient", &light->Ambient, 1);
	AddRGBArray(data, "Diffuse", &light->Diffuse, 1);
	AddRGBArray(data, "Specular", &light->Specular, 1);
	AddFloat(data, "Intensity", light->Intensity);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_LIGHT_TRANSFORM)
{
	char *chunkdata = ReadChunkData(cload);
	W3dLightTransformStruct *transform = (W3dLightTransformStruct *)chunkdata;
	AddFloatArray(data, "Transform", transform->Transform[0], 4);
	AddFloatArray(data, "Transform", transform->Transform[1], 4);
	AddFloatArray(data, "Transform", transform->Transform[2], 4);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_LIGHTSCAPE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_LIGHTSCAPE_LIGHT)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_LOD)
{
	char *chunkdata = ReadChunkData(cload);
	W3dLODStruct *lod = (W3dLODStruct *)chunkdata;
	AddString(data, "Render Object Name", lod->RenderObjName, "string");
	AddFloat(data, "LOD Min Distance", lod->LODMin);
	AddFloat(data, "LOD Max Distance", lod->LODMax);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_LODMODEL)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_LODMODEL_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dLODModelHeaderStruct *header = (W3dLODModelHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddInt16(data, "NumLODs", header->NumLODs);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MAP3_FILENAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Texture Filename:", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MAP3_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMap3Struct *map = (W3dMap3Struct *)chunkdata;
	AddInt16(data, "Mapping Type", map->MappingType);
	AddInt16(data, "Frame Count", map->FrameCount);
	AddFloat(data, "Frame Rate", map->FrameRate);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MATERIAL_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMaterialInfoStruct *info = (W3dMaterialInfoStruct *)chunkdata;
	AddInt32(data, "PassCount", info->PassCount);
	AddInt32(data, "VertexMaterialCount", info->VertexMaterialCount);
	AddInt32(data, "ShaderCount", info->ShaderCount);
	AddInt32(data, "TextureCount", info->TextureCount);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MATERIAL_PASS)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MATERIAL3)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MATERIAL3_DC_MAP)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MATERIAL3_DI_MAP)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MATERIAL3_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMaterial3Struct *material = (W3dMaterial3Struct *)chunkdata;
	AddInt32(data, "Attributes", material->Attributes);
	if (material->Attributes & W3DMATERIAL_USE_ALPHA)
	{
		AddString(data, "Attributes", "W3DMATERIAL_USE_ALPHA", "string");
	}
	if (material->Attributes & W3DMATERIAL_USE_SORTING)
	{
		AddString(data, "Attributes", "W3DMATERIAL_USE_SORTING", "string");
	}
	if (material->Attributes & W3DMATERIAL_HINT_DIT_OVER_DCT)
	{
		AddString(data, "Attributes", "W3DMATERIAL_HINT_DIT_OVER_DCT", "string");
	}
	if (material->Attributes & W3DMATERIAL_HINT_SIT_OVER_SCT)
	{
		AddString(data, "Attributes", "W3DMATERIAL_HINT_SIT_OVER_SCT", "string");
	}
	if (material->Attributes & W3DMATERIAL_HINT_DIT_OVER_DIG)
	{
		AddString(data, "Attributes", "W3DMATERIAL_HINT_DIT_OVER_DIG", "string");
	}
	if (material->Attributes & W3DMATERIAL_HINT_SIT_OVER_SIG)
	{
		AddString(data, "Attributes", "W3DMATERIAL_HINT_SIT_OVER_SIG", "string");
	}
	if (material->Attributes & W3DMATERIAL_HINT_FAST_SPECULAR_AFTER_ALPHA)
	{
		AddString(data, "Attributes", "W3DMATERIAL_HINT_FAST_SPECULAR_AFTER_ALPHA", "string");
	}
	if (material->Attributes & W3DMATERIAL_PSX_TRANS_100)
	{
		AddString(data, "Attributes", "W3DMATERIAL_PSX_TRANS_100", "string");
	}
	if (material->Attributes & W3DMATERIAL_PSX_TRANS_50)
	{
		AddString(data, "Attributes", "W3DMATERIAL_PSX_TRANS_50", "string");
	}
	if (material->Attributes & W3DMATERIAL_PSX_TRANS_25)
	{
		AddString(data, "Attributes", "W3DMATERIAL_PSX_TRANS_25", "string");
	}
	if (material->Attributes & W3DMATERIAL_PSX_TRANS_MINUS_100)
	{
		AddString(data, "Attributes", "W3DMATERIAL_PSX_TRANS_MINUS_100", "string");
	}
	if (material->Attributes & W3DMATERIAL_PSX_NO_RT_LIGHTING)
	{
		AddString(data, "Attributes", "W3DMATERIAL_PSX_NO_RT_LIGHTING", "string");
	}
	AddRGB(data, "Diffuse Color", &material->DiffuseColor);
	AddRGB(data, "Specular Color", &material->SpecularColor);
	AddRGB(data, "Emissive Coefficients", &material->EmissiveCoefficients);
	AddRGB(data, "Ambient Coefficients", &material->AmbientCoefficients);
	AddRGB(data, "Diffuse Coefficients", &material->DiffuseCoefficients);
	AddRGB(data, "Specular Coefficients", &material->SpecularCoefficients);
	AddFloat(data, "Shininess", material->Shininess);
	AddFloat(data, "Opacity", material->Opacity);
	AddFloat(data, "Translucency", material->Translucency);
	AddFloat(data, "Fog Coefficient", material->FogCoeff);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MATERIAL3_NAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Material Name:", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MATERIAL3_SC_MAP)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MATERIAL3_SI_MAP)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MATERIALS3)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MESH)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MESH_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMeshHeaderStruct *header = (W3dMeshHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "MeshName", header->MeshName, "string");
	AddInt32(data, "Attributes", header->Attributes);
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_BOX)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_BOX", "string");
	}
	if (header->Attributes & W3D_MESH_FLAG_SKIN)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_SKIN", "string");
	}
	if (header->Attributes & W3D_MESH_FLAG_SHADOW)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_SHADOW", "string");
	}
	if (header->Attributes & W3D_MESH_FLAG_ALIGNED)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_ALIGNED", "string");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL", "string");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE", "string");
	}
	AddInt32(data, "NumTris", header->NumTris);
	AddInt32(data, "NumQuads", header->NumQuads);
	AddInt32(data, "NumSrTris", header->NumSrTris);
	AddInt32(data, "NumPovQuads", header->NumPovQuads);
	AddInt32(data, "NumVertices", header->NumVertices);
	AddInt32(data, "NumNormals", header->NumNormals);
	AddInt32(data, "NumSrNormals", header->NumSrNormals);
	AddInt32(data, "NumTexCoords", header->NumTexCoords);
	AddInt32(data, "NumMaterials", header->NumMaterials);
	AddInt32(data, "NumVertColors", header->NumVertColors);
	AddInt32(data, "NumVertInfluences", header->NumVertInfluences);
	AddInt32(data, "NumDamageStages", header->NumDamageStages);
	AddInt32Array(data, "FutureCounts", header->FutureCounts, 5);
	AddFloat(data, "LODMin", header->LODMin);
	AddFloat(data, "LODMax", header->LODMax);
	AddVector(data, "Min", &header->Min);
	AddVector(data, "Max", &header->Max);
	AddVector(data, "SphCenter", &header->SphCenter);
	AddFloat(data, "SphRadius", header->SphRadius);
	AddVector(data, "Translation", &header->Translation);
	AddFloatArray(data, "Rotation", header->Rotation, 9);
	AddVector(data, "MassCenter", &header->MassCenter);
	AddFloatArray(data, "Inertia", header->Inertia, 9);
	AddFloat(data, "Volume", header->Volume);
	AddString(data, "HierarchyTreeName", header->HierarchyTreeName, "string");
	AddString(data, "HierarchyModelName", header->HierarchyModelName, "string");
	AddInt32Array(data, "FutureUse", header->FutureUse, 24);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MESH_HEADER3)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMeshHeader3Struct *header = (W3dMeshHeader3Struct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "MeshName", header->MeshName, "string");
	AddString(data, "ContainerName", header->ContainerName, "string");
	AddInt32(data, "Attributes", header->Attributes);
	int type = header->Attributes & W3D_MESH_FLAG_GEOMETRY_TYPE_MASK;
	switch (type)
	{
	case W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL", "flag");
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ALIGNED:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ALIGNED", "flag");
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_SKIN:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_SKIN", "flag");
		break;
	case OBSOLETE_W3D_MESH_FLAG_GEOMETRY_TYPE_SHADOW:
		AddString(data, "Attributes", "OBSOLETE_W3D_MESH_FLAG_GEOMETRY_TYPE_SHADOW", "flag");
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_AABOX:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_AABOX", "flag");
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_OBBOX:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_OBBOX", "flag");
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ORIENTED:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ORIENTED", "flag");
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_Z_ORIENTED:
		AddString(data, "Attributes", "W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_Z_ORIENTED", "flag");
		break;
	default:
		{
			StringClass str;
			str.Format("W3D_CHUNK_MESH_HEADER3 Unknown Mesh Type %x", type);
			data->unknowndata.Add(str);
			AddString(data, "Attributes", "Unknown", "string");
		}
		break;
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_VIS)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_VIS", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_CAMERA)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_CAMERA", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_VEHICLE)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_VEHICLE", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_USER1)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_USER1", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_TYPE_USER2)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_TYPE_USER2", "flag");
	}
	if (header->Attributes & 0x00000800)
	{
		StringClass str;
		str.Format("W3D_CHUNK_MESH_HEADER3 Unknown Attribute 0x00000800");
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	if (header->Attributes & W3D_MESH_FLAG_COLLISION_BOX)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_COLLISION_BOX", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_SKIN)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_SKIN", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_SHADOW)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_SHADOW", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_ALIGNED)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_ALIGNED", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_HIDDEN)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_HIDDEN", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_TWO_SIDED)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_TWO_SIDED", "flag");
	}
	if (header->Attributes & OBSOLETE_W3D_MESH_FLAG_LIGHTMAPPED)
	{
		AddString(data, "Attributes", "OBSOLETE_W3D_MESH_FLAG_LIGHTMAPPED", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_CAST_SHADOW)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_CAST_SHADOW", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_SHATTERABLE)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_SHATTERABLE", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_NPATCHABLE)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_NPATCHABLE", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_PRELIT)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_PRELIT", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_ALWAYSDYNLIGHT)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_ALWAYSDYNLIGHT", "flag");
	}

	if (header->Attributes & W3D_MESH_FLAG_PRELIT_UNLIT)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_PRELIT_UNLIT", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_PRELIT_VERTEX)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_PRELIT_VERTEX", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_PASS)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_PASS", "flag");
	}
	if (header->Attributes & W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_TEXTURE)
	{
		AddString(data, "Attributes", "W3D_MESH_FLAG_PRELIT_LIGHTMAP_MULTI_TEXTURE", "flag");
	}
	AddInt32(data, "NumTris", header->NumTris);
	AddInt32(data, "NumVertices", header->NumVertices);
	AddInt32(data, "NumMaterials", header->NumMaterials);
	AddInt32(data, "NumDamageStages", header->NumDamageStages);
	if (header->SortLevel)
	{
		AddInt8(data, "SortLevel", (uint8)header->SortLevel);
	}
	else
	{
		AddString(data, "SortLevel", "NONE", "string");
	}
	char c[64];
	if (header->Attributes & W3D_MESH_FLAG_PRELIT_MASK)
	{
		if (header->PrelitVersion)
		{
			sprintf(c, "%u.%hu", header->PrelitVersion >> 16, (uint16)header->PrelitVersion);
		}
		else
		{
			sprintf(c, "UNKNOWN");
		}
	}
	else
	{
		sprintf(c, "N/A");
	}
	AddString(data, "PrelitVersion", c, "string");
	AddInt32Array(data, "FutureCounts", header->FutureCounts, W3D_VERTEX_CHANNEL_LOCATION);
	AddInt32(data, "VertexChannels", header->VertexChannels);
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_LOCATION)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_LOCATION", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_NORMAL)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_NORMAL", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_TEXCOORD)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_TEXCOORD", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_COLOR)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_COLOR", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_BONEID)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_BONEID", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_TANGENT)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_TANGENT", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_BINORMAL)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_BINORMAL", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_SMOOTHSKIN)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_SMOOTHSKIN", "flag");
	}
	if (header->VertexChannels & W3D_VERTEX_CHANNEL_SUPERSMOOTHSKIN)
	{
		AddString(data, "VertexChannels", "W3D_VERTEX_CHANNEL_SUPERSMOOTHSKIN", "flag");
	}
	const uint32 knownVertexChannels = W3D_VERTEX_CHANNEL_LOCATION |
		W3D_VERTEX_CHANNEL_NORMAL |
		W3D_VERTEX_CHANNEL_TEXCOORD |
		W3D_VERTEX_CHANNEL_COLOR |
		W3D_VERTEX_CHANNEL_BONEID |
		W3D_VERTEX_CHANNEL_TANGENT |
		W3D_VERTEX_CHANNEL_BINORMAL |
		W3D_VERTEX_CHANNEL_SMOOTHSKIN |
		W3D_VERTEX_CHANNEL_SUPERSMOOTHSKIN;
	if (header->VertexChannels & ~knownVertexChannels)
	{
		StringClass str;
		str.Format("W3D_CHUNK_MESH_HEADER3 Unknown Vertex Channels %x", header->VertexChannels);
		data->unknowndata.Add(str);
		AddString(data, "VertexChannels", "Unknown", "string");
	}
	AddInt32(data, "FaceChannels", header->FaceChannels);
	if (header->FaceChannels & W3D_FACE_CHANNEL_FACE)
	{
		AddString(data, "FaceChannels", "W3D_FACE_CHANNEL_FACE", "flag");
	}
	if (header->FaceChannels & 0xFFFFFFFE)
	{
		StringClass str;
		str.Format("W3D_CHUNK_MESH_HEADER3 Unknown Face Channels %x", header->FaceChannels);
		data->unknowndata.Add(str);
		AddString(data, "FaceChannels", "Unknown", "string");
	}
	AddVector(data, "Min", &header->Min);
	AddVector(data, "Max", &header->Max);
	AddVector(data, "SphCenter", &header->SphCenter);
	AddFloat(data, "SphRadius", header->SphRadius);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MESH_USER_TEXT)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "UserText", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_NEAR_ATTENUATION)
{
	char *chunkdata = ReadChunkData(cload);
	W3dLightAttenuationStruct *atten = (W3dLightAttenuationStruct *)chunkdata;
	AddFloat(data, "Near Atten Start", atten->Start);
	AddFloat(data, "Near Atten End", atten->End);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_NODE)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHModelNodeStruct *node = (W3dHModelNodeStruct *)chunkdata;
	AddString(data, "RenderObjName", node->RenderObjName, "string");
	AddInt16(data, "PivotIdx", node->PivotIdx);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_NULL_OBJECT)
{
	char *chunkdata = ReadChunkData(cload);
	W3dNullObjectStruct *obj = (W3dNullObjectStruct *)chunkdata;
	AddVersion(data, obj->Version);
	AddInt32(data, "Attributes", obj->Attributes);
	AddString(data, "Name", obj->Name, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PER_FACE_TEXCOORD_IDS)
{
	char *chunkdata = ReadChunkData(cload);
	Vector3i *ids = (Vector3i *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(Vector3i); i++)
	{
		char c[256];
		sprintf(c, "Face[%d] UV Indices", i);
		AddIJK(data, c, &ids[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PER_TRI_MATERIALS)
{
	char *chunkdata = ReadChunkData(cload);
	uint16 *materials = (uint16 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint16); i++)
	{
		char c[256];
		sprintf(c, "Triangle[%d].MaterialIdx", i);
		AddInt16(data, c, materials[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PIVOT_FIXUPS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dPivotFixupStruct *pivots = (W3dPivotFixupStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dPivotFixupStruct); i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			char c[256];
			sprintf(c, "Transform %d, Row[%d]", i, j);
			AddFloatArray(data, c, pivots[i].TM[j], 3);
		}
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PIVOTS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dPivotStruct *pivots = (W3dPivotStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dPivotStruct); i++)
	{
		char c[256];
		sprintf(c, "Pivot[%d].Name", i);
		AddString(data, c, pivots[i].Name, "string");
		sprintf(c, "Pivot[%d].ParentIdx", i);
		AddInt32(data, c, pivots[i].ParentIdx);
		sprintf(c, "Pivot[%d].Translation", i);
		AddVector(data, c, &pivots[i].Translation);
		sprintf(c, "Pivot[%d].EulerAngles", i);
		AddVector(data, c, &pivots[i].EulerAngles);
		sprintf(c, "Pivot[%d].Rotation", i);
		AddQuaternion(data, c, &pivots[i].Rotation);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PLACEHOLDER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dPlaceholderStruct *placeholder = (W3dPlaceholderStruct *)chunkdata;
	AddVersion(data, placeholder->Version);
	AddFloatArray(data, "Transform", placeholder->Transform[0], 3);
	AddFloatArray(data, "Transform", placeholder->Transform[1], 3);
	AddFloatArray(data, "Transform", placeholder->Transform[2], 3);
	AddFloatArray(data, "Transform", placeholder->Transform[3], 3);
	AddString(data, "Name", placeholder->Name, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_POINTS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *points = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Point[%d]", i);
		AddVector(data, c, &points[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_PASS)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_TEXTURE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_PRELIT_UNLIT)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_PRELIT_VERTEX)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_PS2_SHADERS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dPS2ShaderStruct *shaders = (W3dPS2ShaderStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dPS2ShaderStruct); i++)
	{
		char c[256];
		sprintf(c, "shader[%d]", i);
		AddPS2Shader(data, c, &shaders[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SCG)
{
	char *chunkdata = ReadChunkData(cload);
	W3dRGBStruct *colors = (W3dRGBStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dRGBStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d].SCG", i);
		AddRGB(data, c, &colors[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHADER_IDS)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *ids = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Face[%d] Shader Index", i);
		AddInt32(data, c, ids[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHADERS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dShaderStruct *shaders = (W3dShaderStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dShaderStruct); i++)
	{
		char c[256];
		sprintf(c, "shader[%d]", i);
		AddShader(data, c, &shaders[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SKIN_NODE)
{
	char *chunkdata = ReadChunkData(cload);
	W3dHModelNodeStruct *node = (W3dHModelNodeStruct *)chunkdata;
	AddString(data, "SkinMeshName", node->RenderObjName, "string");
	AddInt16(data, "PivotIdx", node->PivotIdx);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SPOT_LIGHT_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dSpotLightStruct *light = (W3dSpotLightStruct *)chunkdata;
	AddVector(data, "SpotDirection", &light->SpotDirection);
	AddFloat(data, "SpotAngle", light->SpotAngle);
	AddFloat(data, "SpotExponent", light->SpotExponent);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SPOT_LIGHT_INFO_5_0)
{
	char *chunkdata = ReadChunkData(cload);
	W3dSpotLightStruct_v5_0 *light = (W3dSpotLightStruct_v5_0*)chunkdata;
	AddFloat(data, "SpotOuterAngle", light->SpotOuterAngle);
	AddFloat(data, "SpotInnerAngle", light->SpotInnerAngle);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_PULSE)
{
	char* chunkdata = ReadChunkData(cload);
	W3dLightPulseStruct* light = (W3dLightPulseStruct*)chunkdata;
	AddFloat(data, "MinIntensity", light->MinIntensity);
	AddFloat(data, "MaxIntensity", light->MaxIntensity);
	AddFloat(data, "IntensityTimeRandom", light->IntensityTimeRandom);
	AddFloat(data, "IntensityAdjust", light->IntensityAdjust);
	AddInt8(data, "IntensityStopsAtMax", light->IntensityStopsAtMax);
	AddInt8(data, "IntensityStopsAtMin", light->IntensityStopsAtMin);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_STAGE_TEXCOORDS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTexCoordStruct *coords = (W3dTexCoordStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dTexCoordStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d].UV", i);
		AddTexCoord(data, c, &coords[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SURRENDER_NORMALS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *normals = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "SRNormal[%d]", i);
		AddVector(data, c, &normals[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TEXCOORDS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTexCoordStruct *texcoords = (W3dTexCoordStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dTexCoordStruct); i++)
	{
		char c[256];
		sprintf(c, "TexCoord[%d]", i);
		AddTexCoord(data, c, &texcoords[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TEXTURE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_TEXTURE_IDS)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *ids = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Face[%d] Texture Index", i);
		AddInt32(data, c, ids[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TEXTURE_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTextureInfoStruct *info = (W3dTextureInfoStruct *)chunkdata;
	AddInt16(data, "Texture.Attributes", info->Attributes);
	if (info->Attributes & W3DTEXTURE_PUBLISH)
	{
		AddString(data, "Attributes", "W3DTEXTURE_PUBLISH", "flag");
	}
	if (info->Attributes & W3DTEXTURE_RESIZE_OBSOLETE)
	{
		AddString(data, "Attributes", "W3DTEXTURE_RESIZE_OBSOLETE", "flag");
	}
	if (info->Attributes & W3DTEXTURE_NO_LOD)
	{
		AddString(data, "Attributes", "W3DTEXTURE_NO_LOD", "flag");
	}
	if (info->Attributes & W3DTEXTURE_CLAMP_U)
	{
		AddString(data, "Attributes", "W3DTEXTURE_CLAMP_U", "flag");
	}
	if (info->Attributes & W3DTEXTURE_CLAMP_V)
	{
		AddString(data, "Attributes", "W3DTEXTURE_CLAMP_V", "flag");
	}
	if (info->Attributes & W3DTEXTURE_ALPHA_BITMAP)
	{
		AddString(data, "Attributes", "W3DTEXTURE_ALPHA_BITMAP", "flag");
	}
	int mip = info->Attributes & W3DTEXTURE_MIP_LEVELS_MASK;
	if (mip == W3DTEXTURE_MIP_LEVELS_ALL)
	{
		AddString(data, "Attributes", "W3DTEXTURE_MIP_LEVELS_ALL", "flag");
	}
	else if (mip == W3DTEXTURE_MIP_LEVELS_2)
	{
		AddString(data, "Attributes", "W3DTEXTURE_MIP_LEVELS_2", "flag");
	}
	else if (mip == W3DTEXTURE_MIP_LEVELS_3)
	{
		AddString(data, "Attributes", "W3DTEXTURE_MIP_LEVELS_3", "flag");
	}
	else if (mip == W3DTEXTURE_MIP_LEVELS_4)
	{
		AddString(data, "Attributes", "W3DTEXTURE_MIP_LEVELS_4", "flag");
	}
	int hint = info->Attributes & 0xF00;
	if (hint == W3DTEXTURE_HINT_BASE)
	{
		AddString(data, "Attributes", "W3DTEXTURE_HINT_BASE", "flag");
	}
	else if (hint == W3DTEXTURE_HINT_EMISSIVE)
	{
		AddString(data, "Attributes", "W3DTEXTURE_HINT_EMISSIVE", "flag");
	}
	else if (hint == W3DTEXTURE_HINT_ENVIRONMENT)
	{
		AddString(data, "Attributes", "W3DTEXTURE_HINT_ENVIRONMENT", "flag");
	}
	else if (hint == W3DTEXTURE_HINT_SHINY_MASK)
	{
		AddString(data, "Attributes", "W3DTEXTURE_HINT_SHINY_MASK", "flag");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_TEXTURE_INFO Unknown Hints %x", hint);
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	if (info->Attributes & W3DTEXTURE_TYPE_MASK)
	{
		AddString(data, "Attributes", "W3DTEXTURE_TYPE_BUMPMAP", "flag");
	}
	else
	{
		AddString(data, "Attributes", "W3DTEXTURE_TYPE_COLORMAP", "flag");
	}
	if (info->Attributes & 0xE000)
	{
		StringClass str;
		str.Format("W3D_CHUNK_TEXTURE_INFO Unknown Flags %x", info->Attributes & 0xE000);
		data->unknowndata.Add(str);
		AddString(data, "Attributes", "Unknown", "string");
	}
	AddInt16(data, "Texture.AnimType", info->AnimType);
	if (info->AnimType == W3DTEXTURE_ANIM_LOOP)
	{
		AddString(data, "AnimType", "W3DTEXTURE_ANIM_LOOP", "flag");
	}
	else if (info->AnimType == W3DTEXTURE_ANIM_PINGPONG)
	{
		AddString(data, "AnimType", "W3DTEXTURE_ANIM_PINGPONG", "flag");
	}
	else if (info->AnimType == W3DTEXTURE_ANIM_ONCE)
	{
		AddString(data, "AnimType", "W3DTEXTURE_ANIM_ONCE", "flag");
	}
	else if (info->AnimType == W3DTEXTURE_ANIM_MANUAL)
	{
		AddString(data, "AnimType", "W3DTEXTURE_ANIM_MANUAL", "flag");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_TEXTURE_INFO Unknown Anim Type %x", info->AnimType);
		data->unknowndata.Add(str);
		AddString(data, "AnimType", "Unknown", "string");
	}
	AddInt32(data, "Texture.FrameCount", info->FrameCount);
	AddFloat(data, "Texture.FrameRate", info->FrameRate);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TEXTURE_NAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Texture Name:", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TEXTURE_REPLACER_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTextureReplacerHeaderStruct *header = (W3dTextureReplacerHeaderStruct *)chunkdata;
	AddInt32(data, "ReplacedTexturesCount", header->ReplacedTexturesCount);
	W3dTextureReplacerStruct *replacer = (W3dTextureReplacerStruct *)(chunkdata + sizeof(W3dTextureReplacerHeaderStruct));
	for (unsigned int i = 0; i < header->ReplacedTexturesCount; i++)
	{
		char c[256];
		for (int j = 0; j < 15; j++)
		{
			sprintf(c, "Replacer[%u].MeshPath[%d]", i, j);
			AddString(data, c, replacer[i].MeshPath[j], "string");
		}
		for (int j = 0; j < 15; j++)
		{
			sprintf(c, "Replacer[%u].BonePath[%d]", i, j);
			AddString(data, c, replacer[i].BonePath[j], "string");
		}
		AddString(data, "OldTextureName", replacer[i].OldTextureName, "string");
		AddString(data, "NewTextureName", replacer[i].NewTextureName, "string");
		AddInt16(data, "TextureParams.Attributes", replacer[i].TextureParams.Attributes);
		AddInt16(data, "TextureParams.AnimType", replacer[i].TextureParams.AnimType);
		AddInt32(data, "TextureParams.FrameCount", replacer[i].TextureParams.FrameCount);
		AddFloat(data, "TextureParams.FrameRate", replacer[i].TextureParams.FrameRate);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TEXTURE_STAGE)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_TEXTURES)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_TRANSFORM_NODE)
{
	char *chunkdata = ReadChunkData(cload);
	W3dPlaceholderStruct *placeholder = (W3dPlaceholderStruct *)chunkdata;
	AddVersion(data, placeholder->Version);
	AddFloatArray(data, "Transform", placeholder->Transform[0], 3);
	AddFloatArray(data, "Transform", placeholder->Transform[1], 3);
	AddFloatArray(data, "Transform", placeholder->Transform[2], 3);
	AddFloatArray(data, "Transform", placeholder->Transform[3], 3);
	AddString(data, "Name", placeholder->Name, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TRIANGLES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTriStruct *triangles = (W3dTriStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dTriStruct); i++)
	{
		char c[256];
		sprintf(c, "Triangle[%d].VertexIndices", i);
		AddInt32Array(data, c, triangles[i].Vindex, 3);
		sprintf(c, "Triangle[%d].Attributes", i);
		AddInt32(data, c, triangles[i].Attributes);
		sprintf(c, "Triangle[%d].Normal", i);
		AddVector(data, c, &triangles[i].Normal);
		sprintf(c, "Triangle[%d].Dist", i);
		AddFloat(data, c, triangles[i].Dist);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_COLORS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dRGBStruct *colors = (W3dRGBStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dRGBStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d].RGB", i);
		AddRGB(data, c, &colors[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_INFLUENCES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVertInfStruct *vertinf = (W3dVertInfStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVertInfStruct); i++)
	{
		char c[256];
		sprintf(c, "VertexInfluence[%d].BoneIdx[0]", i);
		AddInt16(data, c, vertinf[i].BoneIdx[0]);
		sprintf(c, "VertexInfluence[%d].Weight[0]", i);
		AddInt16(data, c, vertinf[i].Weight[0]);
		sprintf(c, "VertexInfluence[%d].BoneIdx[1]", i);
		AddInt16(data, c, vertinf[i].BoneIdx[1]);
		sprintf(c, "VertexInfluence[%d].Weight[1]", i);
		AddInt16(data, c, vertinf[i].Weight[1]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_INFLUENCES_EXTENDED)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVertInf3WStruct *vertinf = (W3dVertInf3WStruct *)chunkdata;
	unsigned int count = cload.Cur_Chunk_Length() / sizeof(W3dVertInf3WStruct);
	for (unsigned int i = 0; i < count; i++)
	{
		char c[256];
		sprintf(c, "VertexInfluence[%d].BoneIdx[0]", i);
		AddInt16(data, c, vertinf[i].BoneIdx[0]);
		sprintf(c, "VertexInfluence[%d].Weight[0]", i);
		AddInt16(data, c, vertinf[i].Weight[0]);
		sprintf(c, "VertexInfluence[%d].BoneIdx[1]", i);
		AddInt16(data, c, vertinf[i].BoneIdx[1]);
		sprintf(c, "VertexInfluence[%d].Weight[1]", i);
		AddInt16(data, c, vertinf[i].Weight[1]);
		sprintf(c, "VertexInfluence[%d].BoneIdx[2]", i);
		AddInt16(data, c, vertinf[i].BoneIdx[2]);
		sprintf(c, "VertexInfluence[%d].Weight[2]", i);
		AddInt16(data, c, vertinf[i].Weight[2]);
		sprintf(c, "VertexInfluence[%d].BoneIdx[3]", i);
		AddInt16(data, c, vertinf[i].BoneIdx[3]);
		uint32 sum = static_cast<uint32>(vertinf[i].Weight[0]) + static_cast<uint32>(vertinf[i].Weight[1]) + static_cast<uint32>(vertinf[i].Weight[2]);
		uint16 derived = 0;
		if (sum < 65535u)
		{
			derived = static_cast<uint16>(65535u - sum);
		}
		sprintf(c, "VertexInfluence[%d].Weight[3]", i);
		AddInt16(data, c, derived);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_MAPPER_ARGS0)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Stage0 Mapper Args:", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_MAPPER_ARGS1)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Stage1 Mapper Args:", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_MATERIAL)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_VERTEX_MATERIAL_IDS)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *ids = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d] Vertex Material Index", i);
		AddInt32(data, c, ids[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_MATERIAL_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVertexMaterialStruct *material = (W3dVertexMaterialStruct *)chunkdata;
	if (material->Attributes & W3DVERTMAT_USE_DEPTH_CUE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_USE_DEPTH_CUE", "flag");
	}
	if (material->Attributes & W3DVERTMAT_ARGB_EMISSIVE_ONLY)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_ARGB_EMISSIVE_ONLY", "flag");
	}
	if (material->Attributes & W3DVERTMAT_COPY_SPECULAR_TO_DIFFUSE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_COPY_SPECULAR_TO_DIFFUSE", "flag");
	}
	if (material->Attributes & W3DVERTMAT_DEPTH_CUE_TO_ALPHA)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_DEPTH_CUE_TO_ALPHA", "flag");
	}
	if (material->Attributes & 0x00000010)
	{
		StringClass str;
		str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown Attribute 0x00000010");
		data->unknowndata.Add(str);
		AddString(data, "Material.Attributes", "Unknown", "string");
	}
	if (material->Attributes & 0x00000020)
	{
		StringClass str;
		str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown Attribute 0x00000020");
		data->unknowndata.Add(str);
		AddString(data, "Material.Attributes", "Unknown", "string");
	}
	if (material->Attributes & 0x00000040)
	{
		StringClass str;
		str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown Attribute 0x00000040");
		data->unknowndata.Add(str);
		AddString(data, "Material.Attributes", "Unknown", "string");
	}
	if (material->Attributes & 0x00000080)
	{
		StringClass str;
		str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown Attribute 0x00000080");
		data->unknowndata.Add(str);
		AddString(data, "Material.Attributes", "Unknown", "string");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_UV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_UV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_CHEAP_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_CHEAP_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SCREEN)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SCREEN", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SILHOUETTE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SILHOUETTE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SCALE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SCALE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_ROTATE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_ROTATE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_SINE_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_SINE_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_STEP_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_STEP_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_ZIGZAG_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_ZIGZAG_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_WS_CLASSIC_ENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_WS_CLASSIC_ENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_WS_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_WS_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID_CLASSIC_ENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID_CLASSIC_ENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_RANDOM)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_RANDOM", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_EDGE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_EDGE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_BUMPENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_BUMPENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID_WS_CLASSIC_ENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID_WS_CLASSIC_ENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) == W3DVERTMAT_STAGE0_MAPPING_GRID_WS_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE0_MAPPING_GRID_WS_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK) > W3DVERTMAT_STAGE0_MAPPING_GRID_WS_ENVIRONMENT)
	{
		StringClass str;
		str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown Stage 0 Mapper %x", material->Attributes & W3DVERTMAT_STAGE0_MAPPING_MASK);
		data->unknowndata.Add(str);
		AddString(data, "Material.Attributes", "Unknown", "string");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_UV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_UV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_CHEAP_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_CHEAP_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SCREEN)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SCREEN", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SILHOUETTE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SILHOUETTE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SCALE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SCALE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_ROTATE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_ROTATE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_SINE_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_SINE_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_STEP_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_STEP_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_ZIGZAG_LINEAR_OFFSET)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_ZIGZAG_LINEAR_OFFSET", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_WS_CLASSIC_ENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_WS_CLASSIC_ENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_WS_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_WS_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID_CLASSIC_ENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID_CLASSIC_ENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_RANDOM)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_RANDOM", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_EDGE)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_EDGE", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_BUMPENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_BUMPENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID_WS_CLASSIC_ENV)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID_WS_CLASSIC_ENV", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) == W3DVERTMAT_STAGE1_MAPPING_GRID_WS_ENVIRONMENT)
	{
		AddString(data, "Material.Attributes", "W3DVERTMAT_STAGE1_MAPPING_GRID_WS_ENVIRONMENT", "flag");
	}
	if ((material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK) > W3DVERTMAT_STAGE1_MAPPING_GRID_WS_ENVIRONMENT)
	{
		StringClass str;
		str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown Stage 1 Mapper %x", material->Attributes & W3DVERTMAT_STAGE1_MAPPING_MASK);
		data->unknowndata.Add(str);
		AddString(data, "Material.Attributes", "Unknown", "string");
	}
	if (material->Attributes & W3DVERTMAT_PSX_MASK)
	{
		if (material->Attributes & W3DVERTMAT_PSX_NO_RT_LIGHTING)
		{
			AddString(data, "Material.Attributes", "W3DVERTMAT_PSX_NO_RT_LIGHTING", "flag");
		}
		else
		{
			if ((material->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_NONE)
				AddString(data, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_NONE", "flag");
			if ((material->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_100)
				AddString(data, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_100", "flag");
			if ((material->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_50)
				AddString(data, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_50", "flag");
			if ((material->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_25)
				AddString(data, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_25", "flag");
			if ((material->Attributes & W3DVERTMAT_PSX_TRANS_MASK) == W3DVERTMAT_PSX_TRANS_MINUS_100)
				AddString(data, "Material.Attributes", "W3DVERTMAT_PSX_TRANS_MINUS_100", "flag");
		}
		if (material->Attributes & 0xF0000000)
		{
			StringClass str;
			str.Format("W3D_CHUNK_VERTEX_MATERIAL_INFO Unknown PSX material flag %x", material->Attributes & 0xF0000000);
			data->unknowndata.Add(str);
			AddString(data, "Material.Attributes", "Unknown", "string");
		}
	}
	AddInt32(data, "Material.Attributes", material->Attributes);
	AddRGB(data, "Material.Ambient", &material->Ambient);
	AddRGB(data, "Material.Diffuse", &material->Diffuse);
	AddRGB(data, "Material.Specular", &material->Specular);
	AddRGB(data, "Material.Emissive", &material->Emissive);
	AddFloat(data, "Material.Shininess", material->Shininess);
	AddFloat(data, "Material.Opacity", material->Opacity);
	AddFloat(data, "Material.Translucency", material->Translucency);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_MATERIAL_NAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Material Name:", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_MATERIALS)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_VERTEX_NORMALS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *normals = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Normal[%d]", i);
		AddVector(data, c, &normals[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTEX_SHADE_INDICES)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *indices = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Index[%d]", i);
		AddInt32(data, c, indices[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_VERTICES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *vertices = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d]", i);
		AddVector(data, c, &vertices[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_LINE_PROPERTIES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterLinePropertiesStruct *props = (W3dEmitterLinePropertiesStruct *)chunkdata;
	AddInt32(data, "Flags", props->Flags);
	if ((props->Flags & W3D_ELINE_MERGE_INTERSECTIONS) == W3D_ELINE_MERGE_INTERSECTIONS)
	{
		AddString(data, "Flags", "W3D_ELINE_MERGE_INTERSECTIONS", "flag");
	}
	if ((props->Flags & W3D_ELINE_FREEZE_RANDOM) == W3D_ELINE_FREEZE_RANDOM)
	{
		AddString(data, "Flags", "W3D_ELINE_FREEZE_RANDOM", "flag");
	}
	if ((props->Flags & W3D_ELINE_DISABLE_SORTING) == W3D_ELINE_DISABLE_SORTING)
	{
		AddString(data, "Flags", "W3D_ELINE_DISABLE_SORTING", "flag");
	}
	if ((props->Flags & W3D_ELINE_END_CAPS) == W3D_ELINE_END_CAPS)
	{
		AddString(data, "Flags", "W3D_ELINE_END_CAPS", "flag");
	}
	if (props->Flags & 0x00FFFFF0)
	{
		StringClass str;
		str.Format("W3D_CHUNK_EMITTER_LINE_PROPERTIES Unknown Emitter Line Properties flags %x", props->Flags & 0x00FFFFF0);
		data->unknowndata.Add(str);
		AddString(data, "Flags", "Unknown", "string");
	}
	int mapmode = props->Flags >> W3D_ELINE_TEXTURE_MAP_MODE_OFFSET;
	switch (mapmode)
	{
	case W3D_ELINE_UNIFORM_WIDTH_TEXTURE_MAP:
		AddString(data, "Flags", "W3D_ELINE_UNIFORM_WIDTH_TEXTURE_MAP", "flag");
		break;
	case W3D_ELINE_UNIFORM_LENGTH_TEXTURE_MAP:
		AddString(data, "Flags", "W3D_ELINE_UNIFORM_LENGTH_TEXTURE_MAP", "flag");
		break;
	case W3D_ELINE_TILED_TEXTURE_MAP:
		AddString(data, "Flags", "W3D_ELINE_TILED_TEXTURE_MAP", "flag");
		break;
	default:
		{
			StringClass str;
			str.Format("W3D_CHUNK_EMITTER_LINE_PROPERTIES Unknown Emitter Mapping Mode %x", mapmode);
			data->unknowndata.Add(str);
			AddString(data, "Flags", "Unknown", "string");
		}
	}
	AddInt32(data, "SubdivisionLevel", props->SubdivisionLevel);
	AddFloat(data, "NoiseAmplitude", props->NoiseAmplitude);
	AddFloat(data, "MergeAbortFactor", props->MergeAbortFactor);
	AddFloat(data, "TextureTileFactor", props->TextureTileFactor);
	AddFloat(data, "UPerSec", props->UPerSec);
	AddFloat(data, "VPerSec", props->VPerSec);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SECONDARY_VERTICES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *vertices = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d]", i);
		AddVector(data, c, &vertices[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SECONDARY_VERTEX_NORMALS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *normals = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Normal[%d]", i);
		AddVector(data, c, &normals[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_TANGENTS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *tangents = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Tangent[%d]", i);
		AddVector(data, c, &tangents[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_BINORMALS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *binormals = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Binormal[%d]", i);
		AddVector(data, c, &binormals[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_COMPRESSED_ANIMATION)
{
	ParseSubchunks(cload, data);
}

int flavor = ANIM_FLAVOR_TIMECODED;
const char *FlavorTypes[] = { "Timecoded", "Adaptive Delta" };
FUNC(W3D_CHUNK_COMPRESSED_ANIMATION_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dCompressedAnimHeaderStruct *header = (W3dCompressedAnimHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddString(data, "HierarchyName", header->HierarchyName, "string");
	AddInt32(data, "NumFrames", header->NumFrames);
	AddInt16(data, "FrameRate", header->FrameRate);
	if (header->Flavor < ANIM_FLAVOR_VALID)
	{
		AddString(data, "Flavor", FlavorTypes[header->Flavor], "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_COMPRESSED_ANIMATION_HEADER Unknown Flavor Type %x", header->Flavor);
		data->unknowndata.Add(str);
		AddString(data, "Flavor", "Unknown", "string");
	}
	flavor = header->Flavor;
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_COMPRESSED_ANIMATION_CHANNEL)
{
	char *chunkdata = ReadChunkData(cload);
	if (flavor == ANIM_FLAVOR_TIMECODED)
	{
		W3dTimeCodedAnimChannelStruct *channel = (W3dTimeCodedAnimChannelStruct *)chunkdata;
		AddInt32(data, "NumTimeCodes", channel->NumTimeCodes);
		AddInt16(data, "Pivot", channel->Pivot);
		AddInt8(data, "VectorLen", channel->VectorLen);
		if (channel->Flags <= ANIM_CHANNEL_VIS)
		{
			AddString(data, "ChannelType", ChannelTypes[channel->Flags], "string");
		}
		else
		{
			StringClass str;
			str.Format("W3D_CHUNK_COMPRESSED_ANIMATION_CHANNEL Unknown Animation Channel Type %x", channel->Flags);
			data->unknowndata.Add(str);
			AddString(data, "ChnanelType", "Unknown", "string");
		}
		int len = cload.Cur_Chunk_Length() - sizeof(W3dTimeCodedAnimChannelStruct);
		int datalen = ((len >> 2) + 1);
		for (int i = 0; i < datalen; i++)
		{
			StringClass str;
			str.Format("Data[%d]", i);
			AddInt32(data, str, channel->Data[i]);
		}
		delete[] chunkdata;
	}
	else
	{
		W3dAdaptiveDeltaAnimChannelStruct *channel = (W3dAdaptiveDeltaAnimChannelStruct *)chunkdata;
		AddInt32(data, "NumFrames", channel->NumFrames);
		AddInt16(data, "Pivot", channel->Pivot);
		AddInt8(data, "VectorLen", channel->VectorLen);
		if (channel->Flags <= ANIM_CHANNEL_VIS)
		{
			AddString(data, "ChannelType", ChannelTypes[channel->Flags], "string");
		}
		else
		{
			StringClass str;
			str.Format("W3D_CHUNK_COMPRESSED_ANIMATION_CHANNEL Unknown Animation Channel Type %x", channel->Flags);
			data->unknowndata.Add(str);
			AddString(data, "ChannelType", "Unknown", "string");
		}
		AddFloat(data, "Scale", channel->Scale);
		int len = cload.Cur_Chunk_Length() - sizeof(W3dAdaptiveDeltaAnimChannelStruct);
		int datalen = ((len >> 2) + 1);
		for (int i = 0; i < datalen; i++)
		{
			StringClass str;
			str.Format("Data[%d]", i);
			AddInt32(data, str, channel->Data[i]);
		}
		delete[] chunkdata;
	}
}
FUNC(W3D_CHUNK_COMPRESSED_BIT_CHANNEL)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTimeCodedBitChannelStruct *channel = (W3dTimeCodedBitChannelStruct *)chunkdata;
	AddInt32(data, "NumTimeCodes", channel->NumTimeCodes);
	AddInt16(data, "Pivot", channel->Pivot);
	if (channel->Flags <= BIT_CHANNEL_TIMECODED_VIS)
	{
		AddString(data, "ChannelType", BitChannelTypes[channel->Flags], "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_COMPRESSED_BIT_CHANNEL Unknown Animation Channel Type %x", channel->Flags);
		data->unknowndata.Add(str);
		AddString(data, "ChannelType", "Unknown", "string");
	}
	AddInt8(data, "Default Value", channel->DefaultVal);
	int datalen = channel->NumTimeCodes;
	for (int i = 0; i < datalen; i++)
	{
		StringClass str;
		str.Format("Data[%d]", i);
		AddInt32(data, str, channel->Data[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MORPH_ANIMATION)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MORPHANIM_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMorphAnimHeaderStruct *header = (W3dMorphAnimHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddString(data, "HierarchyName", header->HierarchyName, "string");
	AddInt32(data, "FrameCount", header->FrameCount);
	AddFloat(data, "FrameRate", header->FrameRate);
	AddInt32(data, "ChannelCount", header->ChannelCount);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MORPHANIM_CHANNEL)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_MORPHANIM_POSENAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Pose Name", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MORPHANIM_KEYDATA)
{
	char *chunkdata = ReadChunkData(cload);
	W3dMorphAnimKeyStruct *key = (W3dMorphAnimKeyStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dMorphAnimKeyStruct); i++)
	{
		char c[256];
		sprintf(c, "MorphKeys[%d].MorphFrame", i);
		AddInt32(data, c, key[i].MorphFrame);
		sprintf(c, "MorphKeys[%d].PoseFrame", i);
		AddInt32(data, c, key[i].PoseFrame);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_MORPHANIM_PIVOTCHANNELDATA)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *pivot = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "PivotChannel[%d]", i);
		AddInt32(data, c, pivot[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SOUNDROBJ)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_SOUNDROBJ_HEADER)
{
	char *chunkdata = ReadChunkData(cload);
	W3dSoundRObjHeaderStruct *header = (W3dSoundRObjHeaderStruct *)chunkdata;
	AddVersion(data, header->Version);
	AddString(data, "Name", header->Name, "string");
	AddInt32(data, "Flags", header->Flags);
	delete[] chunkdata;
}
#define READ_FLOAT(id, name) \
	case id: \
	{ \
		float f; \
		cload.Read(&f,sizeof(f)); \
		AddFloat(data, #name, f); \
		break; \
	}
#define READ_INT(id, name) \
	case id: \
	{ \
		int i; \
		cload.Read(&i,sizeof(i)); \
		AddInt32(data, #name, i); \
		break; \
	}
#define READ_BOOL(id, name) \
	case id: \
	{ \
		bool b; \
		cload.Read(&b,sizeof(b)); \
		AddInt8(data, #name, b); \
		break; \
	}

#define READ_VECTOR(id, name) \
	case id: \
	{ \
		Vector3 v; \
		cload.Read(&v,sizeof(v)); \
		W3dVectorStruct v2; \
		v2.X = v.X; \
		v2.Y = v.Y; \
		v2.Z = v.Z; \
		AddVector(data, #name, &v2); \
		break; \
	}

#define READ_STRING(id, name) \
	case id: \
	{ \
		StringClass str; \
		cload.Read(str.Get_Buffer(cload.Cur_Micro_Chunk_Length()), cload.Cur_Micro_Chunk_Length()); \
		AddString(data, #name, str, "String"); \
		break; \
	}

FUNC(W3D_CHUNK_SOUNDROBJ_DEFINITION)
{
	while (cload.Open_Chunk())
	{
		switch (cload.Cur_Chunk_ID())
		{
		case 0x100:
			while (cload.Open_Micro_Chunk())
			{
				switch (cload.Cur_Micro_Chunk_ID())
				{
					READ_FLOAT(3, m_Priority);
					READ_FLOAT(4, m_Volume);
					READ_FLOAT(5, m_Pan);
					READ_INT(6, m_LoopCount);
					READ_FLOAT(7, m_DropoffRadius);
					READ_FLOAT(8, m_MaxVolRadius);
					READ_INT(9, m_Type);
					READ_BOOL(10, m_Is3DSound);
					READ_STRING(11, m_Filename);
					READ_STRING(12, m_DisplayText);
					READ_FLOAT(18, m_StartOffset);
					READ_FLOAT(19, m_PitchFactor);
					READ_FLOAT(20, m_PitchFactorRandomizer);
					READ_FLOAT(21, m_VolumeRandomizer);
					READ_INT(22, m_VirtualChannel);
					READ_INT(13, m_LogicalType);
					READ_FLOAT(14, m_LogicalNotifDelay);
					READ_BOOL(15, m_CreateLogicalSound);
					READ_FLOAT(16, m_LogicalDropoffRadius);
					READ_VECTOR(17, m_SphereColor);
					READ_FLOAT(23, m_Doppler);
				}
				cload.Close_Micro_Chunk();
			}
			break;
		case 0x200:
			while (cload.Open_Chunk())
			{
				if (cload.Cur_Chunk_ID() == 0x100)
				{
					while (cload.Open_Micro_Chunk())
					{
						switch (cload.Cur_Micro_Chunk_ID())
						{
							READ_INT(1, m_ID);
							READ_STRING(3, m_Name);
						}
						cload.Close_Micro_Chunk();
					}
				}
				cload.Close_Chunk();
			}
			break;
		}
		cload.Close_Chunk();
	}
}

void DoVector3Channel(ChunkLoadClass &cload, ChunkData *data, const char *name)
{
	int i = 0;
	for (; cload.Open_Chunk(); cload.Close_Chunk())
	{
		if (cload.Cur_Chunk_ID() == 51709961)
		{
			for (; cload.Open_Micro_Chunk(); cload.Close_Micro_Chunk())
			{
				if (cload.Cur_Micro_Chunk_ID() == 1)
				{
					Vector3 value;
					float time;
					cload.Read(&value, sizeof(value));
					cload.Read(&time, sizeof(time));
					StringClass str;
					str.Format("%s[%d].Value.X", name, i);
					AddFloat(data, str, value.X);
					str.Format("%s[%d].Value.Y", name, i);
					AddFloat(data, str, value.Y);
					str.Format("%s[%d].Value.Z", name, i);
					AddFloat(data, str, value.Z);
					str.Format("%s[%d].time", name, i);
					AddFloat(data, str, time);
					i++;
				}
			}
		}
	}
}

void DoVector2Channel(ChunkLoadClass &cload, ChunkData *data, const char *name)
{
	int i = 0;
	for (; cload.Open_Chunk(); cload.Close_Chunk())
	{
		if (cload.Cur_Chunk_ID() == 51709961)
		{
			for (; cload.Open_Micro_Chunk(); cload.Close_Micro_Chunk())
			{
				if (cload.Cur_Micro_Chunk_ID() == 1)
				{
					Vector2 value;
					float time;
					cload.Read(&value, sizeof(value));
					cload.Read(&time, sizeof(time));
					StringClass str;
					str.Format("%s[%d].Value.X", name, i);
					AddFloat(data, str, value.X);
					str.Format("%s[%d].Value.Y", name, i);
					AddFloat(data, str, value.Y);
					str.Format("%s[%d].time", name, i);
					AddFloat(data, str, time);
					i++;
				}
			}
		}
	}
}

void DofloatChannel(ChunkLoadClass &cload, ChunkData *data, const char *name)
{
	int i = 0;
	for (; cload.Open_Chunk(); cload.Close_Chunk())
	{
		if (cload.Cur_Chunk_ID() == 51709961)
		{
			for (; cload.Open_Micro_Chunk(); cload.Close_Micro_Chunk())
			{
				if (cload.Cur_Micro_Chunk_ID() == 1)
				{
					float value;
					float time;
					cload.Read(&value, sizeof(value));
					cload.Read(&time, sizeof(time));
					StringClass str;
					str.Format("%s[%d].Value", name, i);
					AddFloat(data, str, value);
					str.Format("%s[%d].time", name, i);
					AddFloat(data, str, time);
					i++;
				}
			}
		}
	}
}

void DoAlphaVectorStructChannel(ChunkLoadClass &cload, ChunkData *data, const char *name)
{
	int i = 0;
	for (; cload.Open_Chunk(); cload.Close_Chunk())
	{
		if (cload.Cur_Chunk_ID() == 51709961)
		{
			for (; cload.Open_Micro_Chunk(); cload.Close_Micro_Chunk())
			{
				if (cload.Cur_Micro_Chunk_ID() == 1)
				{
					AlphaVectorStruct value;
					float time;
					cload.Read(&value, sizeof(value));
					cload.Read(&time, sizeof(time));
					StringClass str;
					str.Format("%s[%d].Value.Quat.X", name, i);
					AddFloat(data, str, value.Quat.X);
					str.Format("%s[%d].Value.Quat.Y", name, i);
					AddFloat(data, str, value.Quat.Y);
					str.Format("%s[%d].Value.Quat.Z", name, i);
					AddFloat(data, str, value.Quat.Z);
					str.Format("%s[%d].Value.Quat.W", name, i);
					AddFloat(data, str, value.Quat.W);
					str.Format("%s[%d].Value.Magnitude", name, i);
					AddFloat(data, str, value.Magnitude);
					str.Format("%s[%d].time", name, i);
					AddFloat(data, str, time);
					i++;
				}
			}
		}
	}
}

FUNC(W3D_CHUNK_RING)
{
	while (cload.Open_Chunk())
	{
		switch (cload.Cur_Chunk_ID())
		{
		case 1:
			{
				W3dRingStruct RingStruct;
				cload.Read(&RingStruct, sizeof(RingStruct));
				AddInt32(data, "unk0", RingStruct.unk0);
				AddInt32(data, "Flags", RingStruct.Flags);
				if (RingStruct.Flags & RING_CAMERA_ALIGNED)
				{
					AddString(data, "Flags", "RING_CAMERA_ALIGNED", "flag");
				}
				if (RingStruct.Flags & RING_LOOPING)
				{
					AddString(data, "Flags", "RING_LOOPING", "flag");
				}
				if (RingStruct.Flags & 0xFFFFFFFC)
				{
					StringClass str;
					str.Format("W3D_CHUNK_RING Unknown Ring Flags %x", RingStruct.Flags & 0xFFFFFFFC);
					data->unknowndata.Add(str);
					AddString(data, "Flags", "Unknown", "string");
				}
				AddString(data, "Name", RingStruct.Name, "string");
				AddVector(data, "Center", &RingStruct.Center);
				AddVector(data, "Extent", &RingStruct.Extent);
				AddFloat(data, "AnimationDuration", RingStruct.AnimationDuration);
				AddVector(data, "Color", &RingStruct.Color);
				AddFloat(data, "Alpha", RingStruct.Alpha);
				AddFloat(data, "InnerScale.X", RingStruct.InnerScale.X);
				AddFloat(data, "InnerScale.Y", RingStruct.InnerScale.Y);
				AddFloat(data, "OuterScale.X", RingStruct.OuterScale.X);
				AddFloat(data, "OuterScale.Y", RingStruct.OuterScale.Y);
				AddFloat(data, "InnerExtent.X", RingStruct.InnerExtent.X);
				AddFloat(data, "InnerExtent.Y", RingStruct.InnerExtent.Y);
				AddFloat(data, "OuterExtent.X", RingStruct.OuterExtent.X);
				AddFloat(data, "OuterExtent.Y", RingStruct.OuterExtent.Y);
				AddString(data, "TextureName", RingStruct.TextureName, "string");
				AddShader(data, "Shader", &RingStruct.Shader);
				AddInt32(data, "TextureTiling", RingStruct.TextureTiling);
			}
			break;
		case 2:
			DoVector3Channel(cload, data, "ColorChannel");
			break;
		case 3:
			DofloatChannel(cload, data, "AlphaChannel");
			break;
		case 4:
			DoVector2Channel(cload, data, "InnerScaleChannel");
			break;
		case 5:
			DoVector2Channel(cload, data, "OuterScaleChannel");
			break;
		}
		cload.Close_Chunk();
	}
}
FUNC(W3D_CHUNK_SPHERE)
{
	while (cload.Open_Chunk())
	{
		switch (cload.Cur_Chunk_ID())
		{
		case 1:
			{
				W3dSphereStruct SphereStruct;
				cload.Read(&SphereStruct, sizeof(SphereStruct));
				AddInt32(data, "unk0", SphereStruct.unk0);
				AddInt32(data, "Flags", SphereStruct.Flags);
				if (SphereStruct.Flags & SPHERE_ALPHA_VECTOR)
				{
					AddString(data, "Flags", "SPHERE_ALPHA_VECTOR", "flag");
				}
				if (SphereStruct.Flags & SPHERE_CAMERA_ALIGNED)
				{
					AddString(data, "Flags", "SPHERE_CAMERA_ALIGNED", "flag");
				}
				if (SphereStruct.Flags & SPHERE_INVERT_EFFECT)
				{
					AddString(data, "Flags", "SPHERE_INVERT_EFFECT", "flag");
				}
				if (SphereStruct.Flags & SPHERE_LOOPING)
				{
					AddString(data, "Flags", "SPHERE_LOOPING", "flag");
				}
				if (SphereStruct.Flags & 0xFFFFFFF0)
				{
					StringClass str;
					str.Format("W3D_CHUNK_SPHERE Unknown Sphere Flags %x", SphereStruct.Flags & 0xFFFFFFFC);
					data->unknowndata.Add(str);
					AddString(data, "Flags", "Unknown", "string");
				}
				AddString(data, "Name", SphereStruct.Name, "string");
				AddVector(data, "Center", &SphereStruct.Center);
				AddVector(data, "Extent", &SphereStruct.Extent);
				AddFloat(data, "AnimationDuration", SphereStruct.AnimationDuration);
				AddVector(data, "Color", &SphereStruct.Color);
				AddFloat(data, "Alpha", SphereStruct.Alpha);
				AddVector(data, "Scale", &SphereStruct.Scale);
				AddFloat(data, "Vector.Quat.X", SphereStruct.Vector.Quat.X);
				AddFloat(data, "Vector.Quat.Y", SphereStruct.Vector.Quat.Y);
				AddFloat(data, "Vector.Quat.Z", SphereStruct.Vector.Quat.Z);
				AddFloat(data, "Vector.Quat.W", SphereStruct.Vector.Quat.W);
				AddFloat(data, "Vector.Magnutide", SphereStruct.Vector.Magnitude);
				AddString(data, "TextureName", SphereStruct.TextureName, "string");
				AddShader(data, "Shader", &SphereStruct.Shader);
			}
			break;
		case 2:
			DoVector3Channel(cload, data, "ColorChannel");
			break;
		case 3:
			DofloatChannel(cload, data, "AlphaChannel");
			break;
		case 4:
			DoVector3Channel(cload, data, "ScaleChannel");
			break;
		case 5:
			DoAlphaVectorStructChannel(cload, data, "VectorChannel");
			break;
		}
		cload.Close_Chunk();
	}
}
FUNC(W3D_CHUNK_SHDMESH)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_SHDMESH_NAME)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "Name", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_SHDSUBMESH_SHADER)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_SHDSUBMESH_SHADER_TYPE)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *type = (uint32 *)chunkdata;
	AddInt32(data, "Shader Type", *type);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_VERTICES)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *vertices = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Vertex[%d]", i);
		AddVector(data, c, &vertices[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_VERTEX_NORMALS)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *normals = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Normal[%d]", i);
		AddVector(data, c, &normals[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_TRIANGLES)
{
	char *chunkdata = ReadChunkData(cload);
	Vector3i16 *ids = (Vector3i16 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(Vector3i16); i++)
	{
		char c[256];
		sprintf(c, "Triangle[%d]", i);
		AddIJK16(data, c, &ids[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_VERTEX_SHADE_INDICES)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *indices = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Index[%d]", i);
		AddInt32(data, c, indices[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_UV0)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTexCoordStruct *coords = (W3dTexCoordStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dTexCoordStruct); i++)
	{
		char c[256];
		sprintf(c, "UV0[%d]", i);
		AddTexCoord(data, c, &coords[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_UV1)
{
	char *chunkdata = ReadChunkData(cload);
	W3dTexCoordStruct *coords = (W3dTexCoordStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dTexCoordStruct); i++)
	{
		char c[256];
		sprintf(c, "UV1[%d]", i);
		AddTexCoord(data, c, &coords[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_TANGENT_BASIS_S)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *tangents = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Tangent Basis S[%d]", i);
		AddVector(data, c, &tangents[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_TANGENT_BASIS_T)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *tangents = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Tangent Basis T[%d]", i);
		AddVector(data, c, &tangents[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDSUBMESH_TANGENT_BASIS_SXT)
{
	char *chunkdata = ReadChunkData(cload);
	W3dVectorStruct *tangents = (W3dVectorStruct *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(W3dVectorStruct); i++)
	{
		char c[256];
		sprintf(c, "Tangent Basis SXT[%d]", i);
		AddVector(data, c, &tangents[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_EMITTER_EXTRA_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	W3dEmitterExtraInfoStruct *info = (W3dEmitterExtraInfoStruct *)chunkdata;
	AddFloat(data, "FutureStartTime", info->FutureStartTime);
	AddInt8(data, "unk1", info->unk1);
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_SHDMESH_USER_TEXT)
{
	char *chunkdata = ReadChunkData(cload);
	AddString(data, "UserText", chunkdata, "string");
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_FXSHADER_IDS)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 *ids = (uint32 *)chunkdata;
	for (unsigned int i = 0; i < cload.Cur_Chunk_Length() / sizeof(uint32); i++)
	{
		char c[256];
		sprintf(c, "Face[%d] FX Shader Index", i);
		AddInt32(data, c, ids[i]);
	}
	delete[] chunkdata;
}
FUNC(W3D_CHUNK_FX_SHADERS)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_FX_SHADER)
{
	ParseSubchunks(cload, data);
}
FUNC(W3D_CHUNK_FX_SHADER_INFO)
{
	char *chunkdata = ReadChunkData(cload);
	uint8 *version = (uint8 *)chunkdata;
	AddInt8(data, "Version", *version);
	W3dFXShaderStruct *shader = (W3dFXShaderStruct *)(chunkdata + 1);
	AddString(data, "ShaderName", shader->shadername, "string");
	AddInt8(data, "Technique", shader->technique);
	delete[] chunkdata;
}
const char *Types[] = { "Texture", "Float", "Vector2" ,"Vector3", "Vector4", "Int", "Bool" };
FUNC(W3D_CHUNK_FX_SHADER_CONSTANT)
{
	char *chunkdata = ReadChunkData(cload);
	uint32 type = *(uint32 *)chunkdata;
	uint32 strlen = *(uint32 *)(chunkdata + 4);
	char *constantname = (char *)(chunkdata + 4 + 4);
	AddString(data, "Type", Types[type - 1], "String");
	AddString(data, "Constant Name", constantname, "String");
	if (type == CONSTANT_TYPE_TEXTURE)
	{
		char *texture = (char *)(chunkdata + 4 + 4 + strlen + 4);
		AddString(data, "Texture", texture, "String");
	}
	else if (type >= CONSTANT_TYPE_FLOAT1 && type <= CONSTANT_TYPE_FLOAT4)
	{
		int count = type - 1;
		float *floats = (float *)(chunkdata + 4 + 4 + strlen);
		AddFloatArray(data, "Floats", floats, count);
	}
	else if (type == CONSTANT_TYPE_INT)
	{
		uint32 u = *(uint32 *)(chunkdata + 4 + 4 + strlen);
		AddInt32(data, "Int", u);
	}
	else if (type == CONSTANT_TYPE_BOOL)
	{
		uint8 u = *(uint8 *)(chunkdata + 4 + 4 + strlen);
		AddInt32(data, "Bool", u);
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_FX_SHADER_CONSTANT Unknown Constant Type %x", type);
		data->unknowndata.Add(str);
		AddString(data, "Unknown", "Unknown", "string");
	}
	delete[] chunkdata;
}
const char *NewFlavorTypes[] = { "Timecoded", "Adaptive Delta 4", "Adaptive Delta 8" };
FUNC(W3D_CHUNK_COMPRESSED_ANIMATION_MOTION_CHANNEL)
{
	char *chunkdata = ReadChunkData(cload);
	W3dCompressedMotionChannelStruct *channel = (W3dCompressedMotionChannelStruct *)chunkdata;
	if (channel->Flavor < ANIM_FLAVOR_NEW_VALID)
	{
		AddString(data, "Flavor", NewFlavorTypes[channel->Flavor], "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_COMPRESSED_ANIMATION_MOTION_CHANNEL Unknown Flavor Type %x", channel->Flavor);
		data->unknowndata.Add(str);
		AddString(data, "Flavor", "Unknown", "string");
	}
	AddInt8(data, "VectorLen", channel->VectorLen);
	if (channel->Flags <= ANIM_CHANNEL_VIS)
	{
		AddString(data, "ChannelType", ChannelTypes[channel->Flags], "string");
	}
	else
	{
		StringClass str;
		str.Format("W3D_CHUNK_COMPRESSED_ANIMATION_MOTION_CHANNEL Unknown Animation Channel Type %x", channel->Flags);
		data->unknowndata.Add(str);
		AddString(data, "ChnanelType", "Unknown", "string");
	}
	AddInt32(data, "NumTimeCodes", channel->NumTimeCodes);
	AddInt16(data, "Pivot", channel->Pivot);
	if (channel->Flavor == ANIM_FLAVOR_NEW_TIMECODED)
	{
		uint16 *keyframes = (uint16 *)(chunkdata + sizeof(W3dCompressedMotionChannelStruct));
		for (int i = 0; i < channel->NumTimeCodes; i++)
		{
			StringClass str;
			str.Format("KeyFrames[%d]", i);
			AddInt32(data, str, keyframes[i]);
		}
		int datalen = channel->VectorLen * channel->NumTimeCodes;
		int pos = sizeof(W3dCompressedMotionChannelStruct);
		pos += channel->NumTimeCodes * 2;
		if (channel->NumTimeCodes & 1)
		{
			pos += 2;
		}
		uint32 *values = (uint32 *)(chunkdata + pos);
		for (int i = 0; i < datalen; i++)
		{
			StringClass str;
			str.Format("Data[%d]", i);
			AddInt32(data, str, values[i]);
		}
	}
	else
	{
		float *scale = (float *)(chunkdata + sizeof(W3dCompressedMotionChannelStruct));
		AddFloat(data, "Scale", *scale);
		float *initial = (float *)(chunkdata + sizeof(W3dCompressedMotionChannelStruct) + 4);
		for (int i = 0; i < channel->VectorLen; i++)
		{
			StringClass str;
			str.Format("Initial[%d]", i);
			AddFloat(data, str, initial[i]);
		}
		int count = (cload.Cur_Chunk_Length() - sizeof(W3dCompressedMotionChannelStruct) - 4 - 4 * channel->VectorLen) / 4;
		uint32 *values = (uint32 *)(chunkdata + sizeof(W3dCompressedMotionChannelStruct) + 4 + 4 * channel->VectorLen);
		for (int i = 0; i < count; i++)
		{
			StringClass str;
			str.Format("Data[%d]", i);
			AddInt32(data, str, values[i]);
		}
	}
	delete[] chunkdata;
}

#define CHUNK(id) \
{ \
	ChunkDumper c; \
	c.name = #id; \
	c.function = dump##id; \
	chunks[id] = c; \
}

void initmap()
{
	CHUNK(O_W3D_CHUNK_MATERIALS);
	CHUNK(O_W3D_CHUNK_MATERIALS2);
	CHUNK(O_W3D_CHUNK_POV_QUADRANGLES);
	CHUNK(O_W3D_CHUNK_POV_TRIANGLES);
	CHUNK(O_W3D_CHUNK_QUADRANGLES);
	CHUNK(O_W3D_CHUNK_SURRENDER_TRIANGLES);
	CHUNK(O_W3D_CHUNK_TRIANGLES);
	CHUNK(OBSOLETE_W3D_CHUNK_EMITTER_COLOR_KEYFRAME);
	CHUNK(OBSOLETE_W3D_CHUNK_EMITTER_OPACITY_KEYFRAME);
	CHUNK(OBSOLETE_W3D_CHUNK_EMITTER_SIZE_KEYFRAME);
	CHUNK(OBSOLETE_W3D_CHUNK_SHADOW_NODE);
	CHUNK(W3D_CHUNK_AABTREE);
	CHUNK(W3D_CHUNK_AABTREE_HEADER);
	CHUNK(W3D_CHUNK_AABTREE_NODES);
	CHUNK(W3D_CHUNK_AABTREE_POLYINDICES);
	CHUNK(W3D_CHUNK_AGGREGATE);
	CHUNK(W3D_CHUNK_AGGREGATE_CLASS_INFO);
	CHUNK(W3D_CHUNK_AGGREGATE_HEADER);
	CHUNK(W3D_CHUNK_AGGREGATE_INFO);
	CHUNK(W3D_CHUNK_ANIMATION);
	CHUNK(W3D_CHUNK_ANIMATION_CHANNEL);
	CHUNK(W3D_CHUNK_ANIMATION_HEADER);
	CHUNK(W3D_CHUNK_BIT_CHANNEL);
	CHUNK(W3D_CHUNK_BOX);
	CHUNK(W3D_CHUNK_COLLECTION);
	CHUNK(W3D_CHUNK_COLLECTION_HEADER);
	CHUNK(W3D_CHUNK_COLLECTION_OBJ_NAME);
	CHUNK(W3D_CHUNK_COLLISION_NODE);
	CHUNK(W3D_CHUNK_DAMAGE);
	CHUNK(W3D_CHUNK_DAMAGE_COLORS);
	CHUNK(W3D_CHUNK_DAMAGE_HEADER);
	CHUNK(W3D_CHUNK_DAMAGE_VERTICES);
	CHUNK(W3D_CHUNK_DAZZLE);
	CHUNK(W3D_CHUNK_DAZZLE_NAME);
	CHUNK(W3D_CHUNK_DAZZLE_TYPENAME);
	CHUNK(W3D_CHUNK_DCG);
	CHUNK(W3D_CHUNK_DIG);
	CHUNK(W3D_CHUNK_EMITTER);
	CHUNK(W3D_CHUNK_EMITTER_BLUR_TIME_KEYFRAMES);
	CHUNK(W3D_CHUNK_EMITTER_FRAME_KEYFRAMES);
	CHUNK(W3D_CHUNK_EMITTER_HEADER);
	CHUNK(W3D_CHUNK_EMITTER_INFO);
	CHUNK(W3D_CHUNK_EMITTER_INFOV2);
	CHUNK(W3D_CHUNK_EMITTER_PROPS);
	CHUNK(W3D_CHUNK_EMITTER_ROTATION_KEYFRAMES);
	CHUNK(W3D_CHUNK_EMITTER_USER_DATA);
	CHUNK(W3D_CHUNK_FAR_ATTENUATION);
	CHUNK(W3D_CHUNK_HIERARCHY);
	CHUNK(W3D_CHUNK_HIERARCHY_HEADER);
	CHUNK(W3D_CHUNK_HLOD);
	CHUNK(W3D_CHUNK_HLOD_AGGREGATE_ARRAY);
	CHUNK(W3D_CHUNK_HLOD_HEADER);
	CHUNK(W3D_CHUNK_HLOD_LOD_ARRAY);
	CHUNK(W3D_CHUNK_HLOD_SUB_OBJECT_ARRAY_HEADER);
	CHUNK(W3D_CHUNK_HLOD_PROXY_ARRAY);
	CHUNK(W3D_CHUNK_HLOD_LIGHT_ARRAY);
	CHUNK(W3D_CHUNK_HLOD_SUB_OBJECT);
	CHUNK(W3D_CHUNK_HMODEL);
	CHUNK(OBSOLETE_W3D_CHUNK_HMODEL_AUX_DATA);
	CHUNK(W3D_CHUNK_HMODEL_HEADER);
	CHUNK(W3D_CHUNK_LIGHT);
	CHUNK(W3D_CHUNK_LIGHT_INFO);
	CHUNK(W3D_CHUNK_LIGHT_TRANSFORM);
	CHUNK(W3D_CHUNK_LIGHTSCAPE);
	CHUNK(W3D_CHUNK_LIGHTSCAPE_LIGHT);
	CHUNK(W3D_CHUNK_LOD);
	CHUNK(W3D_CHUNK_LODMODEL);
	CHUNK(W3D_CHUNK_LODMODEL_HEADER);
	CHUNK(W3D_CHUNK_MAP3_FILENAME);
	CHUNK(W3D_CHUNK_MAP3_INFO);
	CHUNK(W3D_CHUNK_MATERIAL_INFO);
	CHUNK(W3D_CHUNK_MATERIAL_PASS);
	CHUNK(W3D_CHUNK_MATERIAL3);
	CHUNK(W3D_CHUNK_MATERIAL3_DC_MAP);
	CHUNK(W3D_CHUNK_MATERIAL3_DI_MAP);
	CHUNK(W3D_CHUNK_MATERIAL3_INFO);
	CHUNK(W3D_CHUNK_MATERIAL3_NAME);
	CHUNK(W3D_CHUNK_MATERIAL3_SC_MAP);
	CHUNK(W3D_CHUNK_MATERIAL3_SI_MAP);
	CHUNK(W3D_CHUNK_MATERIALS3);
	CHUNK(W3D_CHUNK_MESH);
	CHUNK(W3D_CHUNK_MESH_HEADER);
	CHUNK(W3D_CHUNK_MESH_HEADER3);
	CHUNK(W3D_CHUNK_MESH_USER_TEXT);
	CHUNK(W3D_CHUNK_NEAR_ATTENUATION);
	CHUNK(W3D_CHUNK_NODE);
	CHUNK(W3D_CHUNK_NULL_OBJECT);
	CHUNK(W3D_CHUNK_PER_FACE_TEXCOORD_IDS);
	CHUNK(W3D_CHUNK_PER_TRI_MATERIALS);
	CHUNK(W3D_CHUNK_PIVOT_FIXUPS);
	CHUNK(W3D_CHUNK_PIVOTS);
	CHUNK(W3D_CHUNK_PLACEHOLDER);
	CHUNK(W3D_CHUNK_POINTS);
	CHUNK(W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_PASS);
	CHUNK(W3D_CHUNK_PRELIT_LIGHTMAP_MULTI_TEXTURE);
	CHUNK(W3D_CHUNK_PRELIT_UNLIT);
	CHUNK(W3D_CHUNK_PRELIT_VERTEX);
	CHUNK(W3D_CHUNK_PS2_SHADERS);
	CHUNK(W3D_CHUNK_SCG);
	CHUNK(W3D_CHUNK_SHADER_IDS);
	CHUNK(W3D_CHUNK_SHADERS);
	CHUNK(W3D_CHUNK_SKIN_NODE);
	CHUNK(W3D_CHUNK_SPOT_LIGHT_INFO);
	CHUNK(W3D_CHUNK_SPOT_LIGHT_INFO_5_0);
	CHUNK(W3D_CHUNK_PULSE);
	CHUNK(W3D_CHUNK_STAGE_TEXCOORDS);
	CHUNK(W3D_CHUNK_SURRENDER_NORMALS);
	CHUNK(W3D_CHUNK_TEXCOORDS);
	CHUNK(W3D_CHUNK_TEXTURE);
	CHUNK(W3D_CHUNK_TEXTURE_IDS);
	CHUNK(W3D_CHUNK_TEXTURE_INFO);
	CHUNK(W3D_CHUNK_TEXTURE_NAME);
	CHUNK(W3D_CHUNK_TEXTURE_REPLACER_INFO);
	CHUNK(W3D_CHUNK_TEXTURE_STAGE);
	CHUNK(W3D_CHUNK_TEXTURES);
	CHUNK(W3D_CHUNK_TRANSFORM_NODE);
	CHUNK(W3D_CHUNK_TRIANGLES);
	CHUNK(W3D_CHUNK_VERTEX_COLORS);
	CHUNK(W3D_CHUNK_VERTEX_INFLUENCES);
	CHUNK(W3D_CHUNK_VERTEX_INFLUENCES_EXTENDED);
	CHUNK(W3D_CHUNK_VERTEX_MAPPER_ARGS0);
	CHUNK(W3D_CHUNK_VERTEX_MAPPER_ARGS1);
	CHUNK(W3D_CHUNK_VERTEX_MATERIAL);
	CHUNK(W3D_CHUNK_VERTEX_MATERIAL_IDS);
	CHUNK(W3D_CHUNK_VERTEX_MATERIAL_INFO);
	CHUNK(W3D_CHUNK_VERTEX_MATERIAL_NAME);
	CHUNK(W3D_CHUNK_VERTEX_MATERIALS);
	CHUNK(W3D_CHUNK_VERTEX_NORMALS);
	CHUNK(W3D_CHUNK_VERTEX_SHADE_INDICES);
	CHUNK(W3D_CHUNK_VERTICES);
	CHUNK(W3D_CHUNK_EMITTER_LINE_PROPERTIES);
	CHUNK(W3D_CHUNK_SECONDARY_VERTICES)
	CHUNK(W3D_CHUNK_SECONDARY_VERTEX_NORMALS)
	CHUNK(W3D_CHUNK_TANGENTS)
	CHUNK(W3D_CHUNK_BINORMALS)
	CHUNK(W3D_CHUNK_COMPRESSED_ANIMATION)
	CHUNK(W3D_CHUNK_COMPRESSED_ANIMATION_HEADER)
	CHUNK(W3D_CHUNK_COMPRESSED_ANIMATION_CHANNEL)
	CHUNK(W3D_CHUNK_COMPRESSED_BIT_CHANNEL)
	CHUNK(W3D_CHUNK_MORPH_ANIMATION)
	CHUNK(W3D_CHUNK_MORPHANIM_HEADER)
	CHUNK(W3D_CHUNK_MORPHANIM_CHANNEL)
	CHUNK(W3D_CHUNK_MORPHANIM_POSENAME)
	CHUNK(W3D_CHUNK_MORPHANIM_KEYDATA)
	CHUNK(W3D_CHUNK_MORPHANIM_PIVOTCHANNELDATA)
	CHUNK(W3D_CHUNK_SOUNDROBJ)
	CHUNK(W3D_CHUNK_SOUNDROBJ_HEADER)
	CHUNK(W3D_CHUNK_SOUNDROBJ_DEFINITION)
	CHUNK(W3D_CHUNK_RING)
	CHUNK(W3D_CHUNK_SPHERE)
	CHUNK(W3D_CHUNK_SHDMESH)
	CHUNK(W3D_CHUNK_SHDMESH_NAME)
	CHUNK(W3D_CHUNK_SHDSUBMESH)
	CHUNK(W3D_CHUNK_SHDSUBMESH_SHADER)
	CHUNK(W3D_CHUNK_SHDSUBMESH_SHADER_TYPE)
	CHUNK(W3D_CHUNK_SHDSUBMESH_VERTICES)
	CHUNK(W3D_CHUNK_SHDSUBMESH_VERTEX_NORMALS)
	CHUNK(W3D_CHUNK_SHDSUBMESH_TRIANGLES)
	CHUNK(W3D_CHUNK_SHDSUBMESH_VERTEX_SHADE_INDICES)
	CHUNK(W3D_CHUNK_SHDSUBMESH_UV0)
	CHUNK(W3D_CHUNK_SHDSUBMESH_UV1)
	CHUNK(W3D_CHUNK_SHDSUBMESH_TANGENT_BASIS_S)
	CHUNK(W3D_CHUNK_SHDSUBMESH_TANGENT_BASIS_T)
	CHUNK(W3D_CHUNK_SHDSUBMESH_TANGENT_BASIS_SXT)
	CHUNK(W3D_CHUNK_EMITTER_EXTRA_INFO)
	CHUNK(W3D_CHUNK_SHDMESH_USER_TEXT)
	CHUNK(W3D_CHUNK_FXSHADER_IDS)
	CHUNK(W3D_CHUNK_FX_SHADERS)
	CHUNK(W3D_CHUNK_FX_SHADER)
	CHUNK(W3D_CHUNK_FX_SHADER_INFO)
	CHUNK(W3D_CHUNK_FX_SHADER_CONSTANT)
	CHUNK(W3D_CHUNK_COMPRESSED_ANIMATION_MOTION_CHANNEL)
}

// Beautify helper: reserved fields whose all-zero state is noise we suppress.
static bool IsReservedFieldName(const char *name)
{
	return strcmp(name, "pad") == 0
		|| strcmp(name, "FutureCounts") == 0
		|| strcmp(name, "FutureUse") == 0;
}

// Beautify helper: returns true when value contains only zero digits, spaces,
// and commas - i.e. something like "0", "0 0 0 0 0", or "0, 0, 0".
static bool IsAllZeroValue(const char *value)
{
	if (!value || !*value) return false;
	for (const char *p = value; *p; ++p)
	{
		if (*p != '0' && *p != ' ' && *p != ',' && *p != '\t') return false;
	}
	return true;
}

static void HtmlEscape(FILE *out, const char *s)
{
	for (; *s; ++s)
	{
		switch (*s)
		{
		case '&':  fputs("&amp;",  out); break;
		case '<':  fputs("&lt;",   out); break;
		case '>':  fputs("&gt;",   out); break;
		case '"':  fputs("&quot;", out); break;
		default:   fputc(*s, out);       break;
		}
	}
}

static void DumpDataHtml(FILE *out, FILE *unknown, const ChunkData *data, int depth)
{
	// Field table for this chunk (only when there are fields).
	if (data->data.Count() > 0)
	{
		fputs("<table><thead><tr><th>Name</th><th>Type</th><th>Value</th></tr></thead><tbody>\n", out);
		for (int i = 0; i < data->data.Count(); i++)
		{
			const ChunkInfo *e = data->data[i];
			fputs("<tr><td>", out); HtmlEscape(out, e->name.Peek_Buffer());
			fputs("</td><td>", out); HtmlEscape(out, e->type.Peek_Buffer());
			fputs("</td><td>", out); HtmlEscape(out, e->value.Peek_Buffer());
			fputs("</td></tr>\n", out);
		}
		fputs("</tbody></table>\n", out);
	}

	// Subchunks as nested <details>.
	for (int i = 0; i < data->subchunks.Count(); i++)
	{
		const ChunkData *sub = data->subchunks[i];
		fputs("<details><summary>", out);
		HtmlEscape(out, sub->name.Peek_Buffer());
		fputs("</summary>\n", out);
		DumpDataHtml(out, unknown, sub, depth + 1);
		fputs("</details>\n", out);
	}

	for (int i = 0; i < data->unknowndata.Count(); i++)
		fprintf(unknown, "%s\n", data->unknowndata[i].Peek_Buffer());
}

void DumpData(FILE *out, FILE *unknown, ChunkData *data, StringClass tabs, bool beautify)
{
	StringClass str = tabs;
	str += '\t';

	if (beautify && data->data.Count() > 0)
	{
		// Compute column widths for this chunk's fields.
		int nameW = 0, typeW = 0;
		for (int i = 0; i < data->data.Count(); i++)
		{
			int n = (int)strlen(data->data[i]->name.Peek_Buffer());
			int t = (int)strlen(data->data[i]->type.Peek_Buffer());
			if (n > nameW) nameW = n;
			if (t > typeW) typeW = t;
		}
		for (int i = 0; i < data->data.Count(); i++)
		{
			const ChunkInfo *entry = data->data[i];
			fprintf(out, "%s  %-*s  =  %-*s  %s\n",
				tabs.Peek_Buffer(),
				nameW, entry->name.Peek_Buffer(),
				typeW, entry->type.Peek_Buffer(),
				entry->value.Peek_Buffer());
		}
	}
	else if (!beautify)
	{
		for (int i = 0; i < data->data.Count(); i++)
		{
			fprintf(out, "%s%s %s\n", tabs.Peek_Buffer(),
				data->data[i]->name.Peek_Buffer(),
				data->data[i]->value.Peek_Buffer());
		}
	}

	for (int i = 0; i < data->subchunks.Count(); i++)
	{
		if (beautify)
			fprintf(out, "%s  [%s]\n", tabs.Peek_Buffer(), data->subchunks[i]->name.Peek_Buffer());
		else
			fprintf(out, "%s%s\n", tabs.Peek_Buffer(), data->subchunks[i]->name.Peek_Buffer());
		DumpData(out, unknown, data->subchunks[i], str, beautify);
	}
	for (int i = 0; i < data->unknowndata.Count(); i++)
	{
		fprintf(unknown, "%s\n", data->unknowndata[i].Peek_Buffer());
	}
}

HTREEITEM TreeViewInsertItem(HWND tree, const wchar_t *text, HTREEITEM parent, HTREEITEM insertafter)
{
	TVINSERTSTRUCT str;
	str.hParent = parent;
	str.hInsertAfter = insertafter;
	str.item.mask = TVIF_TEXT;
	str.item.pszText = (LPWSTR)text;
	return TreeView_InsertItem(tree, &str);
}

// Single-message insert with both text and lParam (saves one TVM_SETITEM
// round-trip per node — meaningful on trees with tens of thousands of nodes).
static HTREEITEM TreeViewInsertItemP(HWND tree, const wchar_t *text, HTREEITEM parent, HTREEITEM insertafter, LPARAM param)
{
	TVINSERTSTRUCT str;
	str.hParent = parent;
	str.hInsertAfter = insertafter;
	str.item.mask = TVIF_TEXT | TVIF_PARAM;
	str.item.pszText = (LPWSTR)text;
	str.item.lParam = param;
	return TreeView_InsertItem(tree, &str);
}

void TreeViewSetItem(HWND tree, HTREEITEM item, LPARAM param)
{
	TVITEM tv;
	tv.mask = TVIF_PARAM;
	tv.hItem = item;
	tv.lParam = param;
	TreeView_SetItem(tree, &tv);
}

LPARAM TreeViewGetItem(HWND tree, HTREEITEM item)
{
	TVITEM tv;
	tv.mask = TVIF_PARAM;
	tv.hItem = item;
	TreeView_GetItem(tree, &tv);
	return tv.lParam;
}

// -1 = no sort (original insertion order); 0–2 = column index.
static int  g_sortCol = -1;
static int  g_sortDir = 0;   // +1 ascending, -1 descending, 0 = original order

// Active cell tracked by NM_CLICK; -1 = none. Used for cell-level copy and
// the focus-rect drawn by NM_CUSTOMDRAW.
static int  g_cellRow = -1;
static int  g_cellCol = -1;

// Cached Explorer ListView theme used to paint the active cell highlight.
// Opened lazily; re-opened on WM_THEMECHANGED.
static HTHEME g_listTheme = nullptr;

static int CALLBACK ListViewSortProc(LPARAM idx1, LPARAM idx2, LPARAM lp)
{
	const int dir = (int)lp;
	if (dir == 0)
	{
		LVITEM li1 = {}, li2 = {};
		li1.mask = li2.mask = LVIF_PARAM;
		li1.iItem = (int)idx1; li2.iItem = (int)idx2;
		ListView_GetItem(listwnd, &li1);
		ListView_GetItem(listwnd, &li2);
		return (int)(li1.lParam - li2.lParam);
	}
	wchar_t t1[1024], t2[1024];
	ListView_GetItemText(listwnd, (int)idx1, g_sortCol, t1, 1024);
	ListView_GetItemText(listwnd, (int)idx2, g_sortCol, t2, 1024);
	return dir * _wcsicmp(t1, t2);
}

static void ListViewSetSortArrow(int col, int dir)
{
	HWND hdr = ListView_GetHeader(listwnd);
	if (!hdr) return;
	int n = Header_GetItemCount(hdr);
	for (int i = 0; i < n; i++)
	{
		HDITEM hi = {};
		hi.mask = HDI_FORMAT;
		Header_GetItem(hdr, i, &hi);
		hi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
		if (i == col && dir != 0)
			hi.fmt |= (dir > 0) ? HDF_SORTUP : HDF_SORTDOWN;
		Header_SetItem(hdr, i, &hi);
	}
}

static void ListViewApplySort()
{
	ListView_SortItemsEx(listwnd, ListViewSortProc, (LPARAM)g_sortDir);
	ListViewSetSortArrow(g_sortCol, g_sortDir);
}

static void CopyListViewToClipboard()
{
	int total = ListView_GetItemCount(listwnd);
	if (!total) return;

	// Single active cell → copy just that cell's text.
	if (g_cellRow >= 0 && g_cellCol >= 0 &&
		ListView_GetSelectedCount(listwnd) <= 1)
	{
		wchar_t buf[1024];
		ListView_GetItemText(listwnd, g_cellRow, g_cellCol, buf, 1024);
		size_t bytes = (wcslen(buf) + 1) * sizeof(wchar_t);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
		if (!hMem) return;
		wchar_t *p = (wchar_t *)GlobalLock(hMem);
		if (!p) { GlobalFree(hMem); return; }
		memcpy(p, buf, bytes);
		GlobalUnlock(hMem);
		if (OpenClipboard(mainwnd))
		{
			EmptyClipboard();
			SetClipboardData(CF_UNICODETEXT, hMem);
			CloseClipboard();
		}
		else GlobalFree(hMem);
		return;
	}

	bool hasSelection = ListView_GetNextItem(listwnd, -1, LVNI_SELECTED) >= 0;
	int  flags        = hasSelection ? LVNI_SELECTED : 0;

	std::wstring out;
	out.reserve(total * 96);
	wchar_t buf[1024];
	int idx = -1;
	while ((idx = ListView_GetNextItem(listwnd, idx, flags)) >= 0)
	{
		ListView_GetItemText(listwnd, idx, 0, buf, 1024); out += buf; out += L'\t';
		ListView_GetItemText(listwnd, idx, 1, buf, 1024); out += buf; out += L'\t';
		ListView_GetItemText(listwnd, idx, 2, buf, 1024); out += buf; out += L"\r\n";
	}
	if (out.empty()) return;

	size_t bytes = (out.size() + 1) * sizeof(wchar_t);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!hMem) return;
	wchar_t *p = (wchar_t *)GlobalLock(hMem);
	if (!p) { GlobalFree(hMem); return; }
	memcpy(p, out.c_str(), bytes);
	GlobalUnlock(hMem);

	if (OpenClipboard(mainwnd))
	{
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
		CloseClipboard();
	}
	else GlobalFree(hMem);
}

void ListViewInsertColumn(HWND list, int col, wchar_t *name, int width)
{
	LVCOLUMN column;
	column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	column.pszText = name;
	column.fmt = LVCFMT_LEFT;
	column.cx = width;
	ListView_InsertColumn(list, col, &column);
}

int ListViewInsertItem(HWND list, int i, wchar_t *name)
{
	LVITEM item;
	memset(&item, 0, sizeof(item));
	item.mask = LVIF_TEXT;
	item.iItem = i;
	item.pszText = name;
	return ListView_InsertItem(list, &item);
}

void ListViewSetItemText(HWND list, int item, int subitem, wchar_t *str)
{
	ListView_SetItemText(list, item, subitem, str);
}

// Translates a double-null-terminated GetOpenFileName-style filter
// ("Label\0*.ext\0Label2\0*.ext2\0\0") into the COMDLG_FILTERSPEC array
// IFileDialog wants. The returned vectors own the wide strings; the spec
// vector references them.
static void BuildFilterSpec(const wchar_t *legacyFilter,
	std::vector<std::wstring> &storage,
	std::vector<COMDLG_FILTERSPEC> &spec)
{
	const wchar_t *p = legacyFilter;
	while (p && *p)
	{
		std::wstring label = p;
		p += label.size() + 1;
		if (!*p) break;
		std::wstring pattern = p;
		p += pattern.size() + 1;
		storage.push_back(std::move(label));
		storage.push_back(std::move(pattern));
	}
	for (size_t i = 0; i + 1 < storage.size(); i += 2)
	{
		COMDLG_FILTERSPEC s;
		s.pszName = storage[i].c_str();
		s.pszSpec = storage[i + 1].c_str();
		spec.push_back(s);
	}
}

static bool ShowFileDialog(REFCLSID clsid, const wchar_t *legacyFilter,
	const wchar_t *defExt, HWND parent, const wchar_t *title,
	FILEOPENDIALOGOPTIONS extraFlags, char *outBuf)
{
	IFileDialog *dlg = nullptr;
	if (FAILED(CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&dlg)))) return false;

	std::vector<std::wstring> storage;
	std::vector<COMDLG_FILTERSPEC> spec;
	BuildFilterSpec(legacyFilter, storage, spec);
	if (!spec.empty()) dlg->SetFileTypes((UINT)spec.size(), spec.data());
	if (defExt && *defExt) dlg->SetDefaultExtension(defExt);
	if (title && *title) dlg->SetTitle(title);

	// If the caller pre-filled outBuf with a suggested filename, seed the
	// dialog's edit box with just the leaf name (paths break IFileDialog).
	if (outBuf && outBuf[0])
	{
		const char *slash = strrchr(outBuf, '\\');
		const char *fwd = strrchr(outBuf, '/');
		if (fwd > slash) slash = fwd;
		const char *leaf = slash ? slash + 1 : outBuf;
		WideStringClass wleaf = leaf;
		dlg->SetFileName(wleaf);
	}

	FILEOPENDIALOGOPTIONS opts = 0;
	dlg->GetOptions(&opts);
	dlg->SetOptions(opts | FOS_FORCEFILESYSTEM | extraFlags);

	HRESULT hr = dlg->Show(parent);
	if (FAILED(hr)) { dlg->Release(); return false; }

	IShellItem *item = nullptr;
	if (FAILED(dlg->GetResult(&item))) { dlg->Release(); return false; }

	PWSTR path = nullptr;
	bool ok = false;
	if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path)
	{
		_snprintf(outBuf, MAX_PATH, "%ls", path);
		CoTaskMemFree(path);
		ok = true;
	}
	item->Release();
	dlg->Release();
	return ok;
}

bool GetOpenFile(char *buf, const wchar_t *filter, const wchar_t *dir, HWND parent, const wchar_t *title)
{
	(void)dir; // IFileOpenDialog uses MRU; legacy initial-dir argument is ignored.
	return ShowFileDialog(CLSID_FileOpenDialog, filter, nullptr, parent, title,
		FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST, buf);
}

bool GetSaveFile(char *buf, const wchar_t *filter, const wchar_t *defExt, HWND parent, const wchar_t *title)
{
	return ShowFileDialog(CLSID_FileSaveDialog, filter, defExt, parent, title,
		FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST, buf);
}

// Top-level chunk-name predicate used by the "Animation dump" menu item.
// Matches every top-level chunk that carries information needed to make sense
// of an animation: the hierarchy (pivot list, needed to interpret pivot IDs)
// and every animation flavour the format supports.
static bool IsAnimationTopLevelChunk(const StringClass &name)
{
	const char *n = name.Peek_Buffer();
	if (!n) return false;
	return strcmp(n, "W3D_CHUNK_HIERARCHY") == 0
		|| strcmp(n, "W3D_CHUNK_ANIMATION") == 0
		|| strcmp(n, "W3D_CHUNK_COMPRESSED_ANIMATION") == 0
		|| strcmp(n, "W3D_CHUNK_MORPH_ANIMATION") == 0;
}

extern ChunkData *master;
extern char currentFilePath[MAX_PATH];

// Forward decls of the lookup helpers (defined below near AddItems).
static const char *FindInfoValue(const ChunkData *data, const char *name);
static const ChunkData *FindMeshHeader(const ChunkData *meshChunk);

// Returns the first direct subchunk named exactly `name`, or nullptr.
static const ChunkData *FindSubchunk(const ChunkData *parent, const char *name)
{
	if (!parent) return nullptr;
	for (int i = 0; i < parent->subchunks.Count(); i++)
	{
		if (parent->subchunks[i]->name == name) return parent->subchunks[i];
	}
	return nullptr;
}

// Resolves the ChunkData attached to the currently selected treeview item,
// then walks up the visible tree to find an enclosing W3D_CHUNK_MESH. Returns
// nullptr if no mesh is selected (selection is on the root, on a synthetic
// container group, or selection is missing entirely).
static const ChunkData *FindSelectedMesh()
{
	HTREEITEM sel = TreeView_GetSelection(treewnd);
	while (sel)
	{
		ChunkData *cd = (ChunkData *)TreeViewGetItem(treewnd, sel);
		if (cd && cd->name == "W3D_CHUNK_MESH") return cd;
		sel = TreeView_GetParent(treewnd, sel);
	}
	return nullptr;
}

// Pulls "Texture Name:" out of a W3D_CHUNK_TEXTURE_NAME data block (the
// parser stores the texture name under that key in W3D_CHUNK_TEXTURE_NAME).
static const char *FindTextureName(const ChunkData *texChunk)
{
	if (!texChunk) return nullptr;
	const ChunkData *nameChunk = FindSubchunk(texChunk, "W3D_CHUNK_TEXTURE_NAME");
	if (!nameChunk) return nullptr;
	for (int i = 0; i < nameChunk->data.Count(); i++)
	{
		const ChunkInfo *e = nameChunk->data[i];
		if (e->name == "Texture Name:") return e->value.Peek_Buffer();
	}
	return nullptr;
}

// Plain-text texture dump for a single mesh. meshChunk is non-null. Each
// texture entry writes its name plus any W3D_CHUNK_TEXTURE_INFO fields.
static void DumpMeshTexturesText(FILE *out, const ChunkData *meshChunk)
{
	const ChunkData *header = FindMeshHeader(meshChunk);
	const char *meshName  = header ? FindInfoValue(header, "MeshName")      : nullptr;
	const char *container = header ? FindInfoValue(header, "ContainerName") : nullptr;
	fprintf(out, "Mesh: %s%s%s%s\n",
		(container && *container) ? container : "(legacy)",
		(meshName && *meshName) ? "." : "",
		(meshName && *meshName) ? meshName : "",
		(!meshName || !*meshName) ? "" : "");

	const ChunkData *textures = FindSubchunk(meshChunk, "W3D_CHUNK_TEXTURES");
	if (!textures)
	{
		fputs("\t(no W3D_CHUNK_TEXTURES)\n\n", out);
		return;
	}
	int count = 0;
	for (int i = 0; i < textures->subchunks.Count(); i++)
	{
		const ChunkData *tex = textures->subchunks[i];
		if (tex->name != "W3D_CHUNK_TEXTURE") continue;
		const char *texName = FindTextureName(tex);
		fprintf(out, "\t[%d] %s\n", count++, texName ? texName : "(unnamed)");
		const ChunkData *info = FindSubchunk(tex, "W3D_CHUNK_TEXTURE_INFO");
		if (info)
		{
			for (int k = 0; k < info->data.Count(); k++)
			{
				const ChunkInfo *e = info->data[k];
				fprintf(out, "\t\t%s = %s\n",
					e->name.Peek_Buffer(), e->value.Peek_Buffer());
			}
		}
	}
	if (count == 0) fputs("\t(no textures)\n", out);
	fputc('\n', out);
}

// Beautified (HTML) texture entry block for a single mesh.
static void DumpMeshTexturesHtml(FILE *out, const ChunkData *meshChunk)
{
	const ChunkData *header = FindMeshHeader(meshChunk);
	const char *meshName  = header ? FindInfoValue(header, "MeshName")      : nullptr;
	const char *container = header ? FindInfoValue(header, "ContainerName") : nullptr;

	fputs("<details open><summary>", out);
	HtmlEscape(out, (container && *container) ? container : "(legacy)");
	if (meshName && *meshName) { fputs(".", out); HtmlEscape(out, meshName); }
	fputs("</summary>\n", out);

	const ChunkData *textures = FindSubchunk(meshChunk, "W3D_CHUNK_TEXTURES");
	if (!textures)
	{
		fputs("<p><em>No W3D_CHUNK_TEXTURES.</em></p>\n</details>\n", out);
		return;
	}

	fputs("<table><thead><tr><th>#</th><th>Texture</th><th>Attributes</th>"
		"<th>AnimType</th><th>FrameCount</th><th>FrameRate</th></tr></thead><tbody>\n", out);

	int count = 0;
	for (int i = 0; i < textures->subchunks.Count(); i++)
	{
		const ChunkData *tex = textures->subchunks[i];
		if (tex->name != "W3D_CHUNK_TEXTURE") continue;
		const char *texName = FindTextureName(tex);
		const ChunkData *info = FindSubchunk(tex, "W3D_CHUNK_TEXTURE_INFO");

		StringClass attrs;
		const char *animType   = nullptr;
		const char *frameCount = nullptr;
		const char *frameRate  = nullptr;
		if (info)
		{
			for (int k = 0; k < info->data.Count(); k++)
			{
				const ChunkInfo *e = info->data[k];
				if (e->name == "Attributes" && e->type == "flag")
				{
					if (attrs.Get_Length()) attrs += " | ";
					attrs += e->value;
				}
				else if (e->name == "Texture.AnimType")   animType   = e->value.Peek_Buffer();
				else if (e->name == "Texture.FrameCount") frameCount = e->value.Peek_Buffer();
				else if (e->name == "Texture.FrameRate")  frameRate  = e->value.Peek_Buffer();
				else if (e->name == "AnimType" && e->type == "string") animType = e->value.Peek_Buffer();
			}
		}

		fprintf(out, "<tr><td>%d</td><td>", count++);
		HtmlEscape(out, texName ? texName : "(unnamed)");
		fputs("</td><td>", out); HtmlEscape(out, attrs.Get_Length() ? attrs.Peek_Buffer() : "-");
		fputs("</td><td>", out); HtmlEscape(out, animType   ? animType   : "-");
		fputs("</td><td>", out); HtmlEscape(out, frameCount ? frameCount : "-");
		fputs("</td><td>", out); HtmlEscape(out, frameRate  ? frameRate  : "-");
		fputs("</td></tr>\n", out);
	}
	if (count == 0)
		fputs("<tr><td colspan=\"6\"><em>No textures.</em></td></tr>\n", out);
	fputs("</tbody></table>\n</details>\n", out);
}

// Writes a texture-only dump. When meshChunk is non-null, only that mesh's
// W3D_CHUNK_TEXTURES is dumped; otherwise every W3D_CHUNK_MESH at top level
// is included. Mirrors DumpMasterToPath: plain-text or HTML based on beautify.
static bool DumpTexturesToPath(const char *path, const ChunkData *meshChunk, bool beautify)
{
	if (!master) return false;
	FILE *out = fopen(path, "wt");
	if (!out) return false;

	if (beautify)
	{
		fputs("<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n", out);
		fprintf(out, "<title>Textures - "); HtmlEscape(out, currentFilePath); fputs("</title>\n", out);
		fputs("<style>\n"
			"body{font-family:Consolas,'Courier New',monospace;font-size:13px;"
			"margin:1em 2em;background:#1e1e1e;color:#d4d4d4;}\n"
			"h1{font-size:1.1em;color:#9cdcfe;margin-bottom:.5em;}\n"
			"details{margin:.2em 0 .2em 1em;}\n"
			"summary{cursor:pointer;font-weight:bold;color:#4ec9b0;"
			"list-style:disclosure-closed;padding:.1em .2em;}\n"
			"details[open]>summary{list-style:disclosure-open;color:#ce9178;}\n"
			"table{border-collapse:collapse;margin:.3em 0 .3em 1.2em;font-size:12px;}\n"
			"th{text-align:left;padding:2px 10px;background:#2d2d2d;color:#9cdcfe;"
			"border-bottom:1px solid #444;}\n"
			"td{padding:1px 10px;border-bottom:1px solid #2a2a2a;}\n"
			"td:first-child{color:#dcdcaa;}\n"
			"td:nth-child(2){color:#ce9178;}\n"
			"tr:hover td{background:#2a2a2a;}\n"
			"</style>\n</head>\n<body>\n", out);
		fprintf(out, "<h1>Textures - "); HtmlEscape(out, currentFilePath); fputs("</h1>\n", out);

		if (meshChunk)
		{
			DumpMeshTexturesHtml(out, meshChunk);
		}
		else
		{
			for (int i = 0; i < master->subchunks.Count(); i++)
			{
				if (master->subchunks[i]->name == "W3D_CHUNK_MESH")
					DumpMeshTexturesHtml(out, master->subchunks[i]);
			}
		}
		fputs("<p style=\"color:#f44;margin-top:1em;\">Generated by CABAL.</p>\n"
			"</body>\n</html>\n", out);
	}
	else
	{
		fprintf(out, "Textures dump: %s\n\n", currentFilePath);
		if (meshChunk)
		{
			DumpMeshTexturesText(out, meshChunk);
		}
		else
		{
			for (int i = 0; i < master->subchunks.Count(); i++)
			{
				if (master->subchunks[i]->name == "W3D_CHUNK_MESH")
					DumpMeshTexturesText(out, master->subchunks[i]);
			}
		}
		fputs("Generated by CABAL.\n", out);
	}

	fclose(out);
	return true;
}

// Counts how many direct subchunks of `parent` are named `name`.
static int CountSubchunks(const ChunkData *parent, const char *name)
{
	if (!parent) return 0;
	int n = 0;
	for (int i = 0; i < parent->subchunks.Count(); i++)
	{
		if (parent->subchunks[i]->name == name) ++n;
	}
	return n;
}

// Writes the animation summary as HTML. Called only in beautify mode.
static void WriteAnimationSummary(FILE *out)
{
	for (int i = 0; i < master->subchunks.Count(); i++)
	{
		const ChunkData *sub = master->subchunks[i];
		if (sub->name == "W3D_CHUNK_HIERARCHY")
		{
			const ChunkData *header = FindSubchunk(sub, "W3D_CHUNK_HIERARCHY_HEADER");
			const char *name = header ? FindInfoValue(header, "Name") : nullptr;
			const char *numPivots = header ? FindInfoValue(header, "NumPivots") : nullptr;
			fputs("<details><summary>Hierarchy: ", out); HtmlEscape(out, name ? name : "(unknown)");
			fprintf(out, " &mdash; %s pivots</summary>\n", numPivots ? numPivots : "?");

			const ChunkData *pivots = FindSubchunk(sub, "W3D_CHUNK_PIVOTS");
			if (pivots)
			{
				fputs("<table><thead><tr><th>#</th><th>Name</th><th>Parent</th></tr></thead><tbody>\n", out);
				int total = numPivots ? atoi(numPivots) : 0;
				for (int p = 0; p < total; p++)
				{
					char k1[64], k2[64];
					sprintf(k1, "Pivot[%d].Name", p);
					sprintf(k2, "Pivot[%d].ParentIdx", p);
					const char *pname  = FindInfoValue(pivots, k1);
					const char *parent = FindInfoValue(pivots, k2);
					fprintf(out, "<tr><td>%d</td><td>", p);
					HtmlEscape(out, pname ? pname : "?");
					fputs("</td><td>", out);
					HtmlEscape(out, parent ? parent : "?");
					fputs("</td></tr>\n", out);
				}
				fputs("</tbody></table>\n", out);
			}
			fputs("</details>\n", out);
		}
		else if (sub->name == "W3D_CHUNK_ANIMATION" || sub->name == "W3D_CHUNK_COMPRESSED_ANIMATION")
		{
			bool compressed = (sub->name == "W3D_CHUNK_COMPRESSED_ANIMATION");
			const ChunkData *header = FindSubchunk(sub,
				compressed ? "W3D_CHUNK_COMPRESSED_ANIMATION_HEADER" : "W3D_CHUNK_ANIMATION_HEADER");
			const char *name   = header ? FindInfoValue(header, "Name")      : nullptr;
			const char *frames = header ? FindInfoValue(header, "NumFrames") : nullptr;
			const char *rate   = header ? FindInfoValue(header, "FrameRate") : nullptr;
			const char *flav   = compressed ? (header ? FindInfoValue(header, "Flavor") : nullptr) : nullptr;

			const char *motionChunk = compressed ? "W3D_CHUNK_COMPRESSED_ANIMATION_CHANNEL" : "W3D_CHUNK_ANIMATION_CHANNEL";
			const char *bitChunk    = compressed ? "W3D_CHUNK_COMPRESSED_BIT_CHANNEL"        : "W3D_CHUNK_BIT_CHANNEL";
			int motionCount = CountSubchunks(sub, motionChunk);
			int bitCount    = CountSubchunks(sub, bitChunk);

			fputs("<details><summary>", out);
			fputs(compressed ? "Compressed Animation: " : "Animation: ", out);
			HtmlEscape(out, name ? name : "(unknown)");
			fprintf(out, " &mdash; %s frames @ %s fps",
				frames ? frames : "?", rate ? rate : "?");
			if (flav) { fputs(", flavor=", out); HtmlEscape(out, flav); }
			fprintf(out, " &mdash; %d motion, %d bit channels</summary>\n", motionCount, bitCount);

			if (compressed)
				fputs("<table><thead><tr><th>Pivot</th><th>Kind</th><th>Channel Type</th><th>Codes</th></tr></thead><tbody>\n", out);
			else
				fputs("<table><thead><tr><th>Pivot</th><th>Kind</th><th>Channel Type</th><th>First Frame</th><th>Last Frame</th></tr></thead><tbody>\n", out);

			for (int c = 0; c < sub->subchunks.Count(); c++)
			{
				const ChunkData *ch = sub->subchunks[c];
				bool isMotion = (ch->name == motionChunk);
				bool isBit    = (ch->name == bitChunk);
				if (!isMotion && !isBit) continue;
				const char *pivot = FindInfoValue(ch, "Pivot");
				const char *type  = FindInfoValue(ch, "ChannelType");
				fputs("<tr><td>", out); HtmlEscape(out, pivot ? pivot : "?");
				fputs("</td><td>", out); fputs(isMotion ? "motion" : "bit", out);
				fputs("</td><td>", out); HtmlEscape(out, type ? type : "?");
				if (compressed)
				{
					const char *codes = FindInfoValue(ch, "NumTimeCodes");
					if (!codes) codes = FindInfoValue(ch, "NumFrames");
					fputs("</td><td>", out); HtmlEscape(out, codes ? codes : "?");
				}
				else
				{
					const char *first = FindInfoValue(ch, "FirstFrame");
					const char *last  = FindInfoValue(ch, "LastFrame");
					fputs("</td><td>", out); HtmlEscape(out, first ? first : "?");
					fputs("</td><td>", out); HtmlEscape(out, last  ? last  : "?");
				}
				fputs("</td></tr>\n", out);
			}
			fputs("</tbody></table>\n</details>\n", out);
		}
	}
}

// Writes the parsed chunk tree to <path>.txt (and unknown bytes to <path>.unk
// when any are present). When animationOnly is true, only the top-level
// subchunks matched by IsAnimationTopLevelChunk are emitted. When beautify
// is true, applies the cosmetic clean-up rules in DumpData and (for
// animation dumps) prepends a one-page summary table.
static bool DumpMasterToPath(const char *path, bool animationOnly, bool beautify)
{
	if (!master) return false;

	StringClass outPath = path;
	StringClass unkPath = path;
	unkPath += ".unk";

	FILE *out = fopen(outPath, "wt");
	if (!out) return false;
	FILE *unk = fopen(unkPath, "wt");
	if (!unk) { fclose(out); return false; }

	if (beautify)
	{
		fputs("<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\">\n", out);
		fputs("<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n", out);
		fprintf(out, "<title>"); HtmlEscape(out, currentFilePath); fputs("</title>\n", out);
		fputs("<style>\n"
			/* ── shared structural styles ── */
			"*{box-sizing:border-box;}\n"
			"body{font-family:Consolas,'Courier New',monospace;font-size:13px;margin:1em 2em;}\n"
			"h1{font-size:1.1em;margin-bottom:.5em;}\n"
			"h2{font-size:.9em;margin:1em 0 .3em;}\n"
			"details{margin:.15em 0 .15em 1em;}\n"
			"summary{cursor:pointer;font-weight:bold;padding:.1em .2em;border-radius:3px;"
			"list-style:disclosure-closed;}\n"
			"details[open]>summary{list-style:disclosure-open;}\n"
			"table{border-collapse:collapse;margin:.3em 0 .3em 1.2em;font-size:12px;}\n"
			"th{text-align:left;padding:2px 10px;}\n"
			"td{padding:1px 10px;}\n"
			"#theme-bar{position:fixed;top:.6em;right:1em;font-family:sans-serif;font-size:12px;}\n"
			"#theme-bar select{font-size:12px;padding:2px 4px;cursor:pointer;}\n"
			/* ── Dark (default) ── */
			"body.t-dark{background:#1e1e1e;color:#d4d4d4;}\n"
			"body.t-dark h1,body.t-dark h2{color:#9cdcfe;}\n"
			"body.t-dark summary{color:#4ec9b0;}\n"
			"body.t-dark details[open]>summary{color:#ce9178;}\n"
			"body.t-dark th{background:#2d2d2d;color:#9cdcfe;border-bottom:1px solid #444;}\n"
			"body.t-dark td{border-bottom:1px solid #2a2a2a;color:#d4d4d4;}\n"
			"body.t-dark td:first-child{color:#dcdcaa;}\n"
			"body.t-dark td:nth-child(2){color:#569cd6;}\n"
			"body.t-dark tr:hover td{background:#2a2a2a;}\n"
			"body.t-dark #theme-bar select{background:#2d2d2d;color:#d4d4d4;border:1px solid #555;}\n"
			/* ── Light ── */
			"body.t-light{background:#ffffff;color:#1e1e1e;}\n"
			"body.t-light h1,body.t-light h2{color:#0050a0;}\n"
			"body.t-light summary{color:#007070;}\n"
			"body.t-light details[open]>summary{color:#a03000;}\n"
			"body.t-light th{background:#e8e8e8;color:#0050a0;border-bottom:1px solid #bbb;}\n"
			"body.t-light td{border-bottom:1px solid #e0e0e0;color:#1e1e1e;}\n"
			"body.t-light td:first-child{color:#7d5c00;}\n"
			"body.t-light td:nth-child(2){color:#0050a0;}\n"
			"body.t-light tr:hover td{background:#f0f0f0;}\n"
			"body.t-light #theme-bar select{background:#f5f5f5;color:#1e1e1e;border:1px solid #bbb;}\n"
			/* ── Monokai ── */
			"body.t-monokai{background:#272822;color:#f8f8f2;}\n"
			"body.t-monokai h1,body.t-monokai h2{color:#a6e22e;}\n"
			"body.t-monokai summary{color:#66d9e8;}\n"
			"body.t-monokai details[open]>summary{color:#fd971f;}\n"
			"body.t-monokai th{background:#3e3d32;color:#a6e22e;border-bottom:1px solid #555;}\n"
			"body.t-monokai td{border-bottom:1px solid #3e3d32;color:#f8f8f2;}\n"
			"body.t-monokai td:first-child{color:#e6db74;}\n"
			"body.t-monokai td:nth-child(2){color:#66d9e8;}\n"
			"body.t-monokai tr:hover td{background:#3e3d32;}\n"
			"body.t-monokai #theme-bar select{background:#3e3d32;color:#f8f8f2;border:1px solid #555;}\n"
			/* ── Solarized Dark ── */
			"body.t-solarized{background:#002b36;color:#839496;}\n"
			"body.t-solarized h1,body.t-solarized h2{color:#268bd2;}\n"
			"body.t-solarized summary{color:#2aa198;}\n"
			"body.t-solarized details[open]>summary{color:#cb4b16;}\n"
			"body.t-solarized th{background:#073642;color:#268bd2;border-bottom:1px solid #586e75;}\n"
			"body.t-solarized td{border-bottom:1px solid #073642;color:#839496;}\n"
			"body.t-solarized td:first-child{color:#b58900;}\n"
			"body.t-solarized td:nth-child(2){color:#268bd2;}\n"
			"body.t-solarized tr:hover td{background:#073642;}\n"
			"body.t-solarized #theme-bar select{background:#073642;color:#839496;border:1px solid #586e75;}\n"
			/* ── High Contrast ── */
			"body.t-hc{background:#000000;color:#ffffff;}\n"
			"body.t-hc h1,body.t-hc h2{color:#ffff00;}\n"
			"body.t-hc summary{color:#00ff00;}\n"
			"body.t-hc details[open]>summary{color:#ff9900;}\n"
			"body.t-hc th{background:#1a1a1a;color:#ffff00;border-bottom:1px solid #fff;}\n"
			"body.t-hc td{border-bottom:1px solid #333;color:#ffffff;}\n"
			"body.t-hc td:first-child{color:#ffff00;}\n"
			"body.t-hc td:nth-child(2){color:#00cfff;}\n"
			"body.t-hc tr:hover td{background:#1a1a1a;}\n"
			"body.t-hc #theme-bar select{background:#1a1a1a;color:#ffffff;border:1px solid #fff;}\n"
			/* ── Dracula ── */
			"body.t-dracula{background:#282a36;color:#f8f8f2;}\n"
			"body.t-dracula h1,body.t-dracula h2{color:#bd93f9;}\n"
			"body.t-dracula summary{color:#8be9fd;}\n"
			"body.t-dracula details[open]>summary{color:#ff79c6;}\n"
			"body.t-dracula th{background:#44475a;color:#bd93f9;border-bottom:1px solid #6272a4;}\n"
			"body.t-dracula td{border-bottom:1px solid #44475a;color:#f8f8f2;}\n"
			"body.t-dracula td:first-child{color:#f1fa8c;}\n"
			"body.t-dracula td:nth-child(2){color:#8be9fd;}\n"
			"body.t-dracula tr:hover td{background:#44475a;}\n"
			"body.t-dracula #theme-bar select{background:#44475a;color:#f8f8f2;border:1px solid #6272a4;}\n"
			/* ── Nord ── */
			"body.t-nord{background:#2e3440;color:#d8dee9;}\n"
			"body.t-nord h1,body.t-nord h2{color:#88c0d0;}\n"
			"body.t-nord summary{color:#8fbcbb;}\n"
			"body.t-nord details[open]>summary{color:#d08770;}\n"
			"body.t-nord th{background:#3b4252;color:#88c0d0;border-bottom:1px solid #4c566a;}\n"
			"body.t-nord td{border-bottom:1px solid #3b4252;color:#d8dee9;}\n"
			"body.t-nord td:first-child{color:#ebcb8b;}\n"
			"body.t-nord td:nth-child(2){color:#81a1c1;}\n"
			"body.t-nord tr:hover td{background:#3b4252;}\n"
			"body.t-nord #theme-bar select{background:#3b4252;color:#d8dee9;border:1px solid #4c566a;}\n"
			/* ── Tokyo Night ── */
			"body.t-tokyo{background:#1a1b2e;color:#a9b1d6;}\n"
			"body.t-tokyo h1,body.t-tokyo h2{color:#7aa2f7;}\n"
			"body.t-tokyo summary{color:#2ac3de;}\n"
			"body.t-tokyo details[open]>summary{color:#ff9e64;}\n"
			"body.t-tokyo th{background:#24283b;color:#7aa2f7;border-bottom:1px solid #414868;}\n"
			"body.t-tokyo td{border-bottom:1px solid #24283b;color:#a9b1d6;}\n"
			"body.t-tokyo td:first-child{color:#e0af68;}\n"
			"body.t-tokyo td:nth-child(2){color:#2ac3de;}\n"
			"body.t-tokyo tr:hover td{background:#24283b;}\n"
			"body.t-tokyo #theme-bar select{background:#24283b;color:#a9b1d6;border:1px solid #414868;}\n"
			/* ── Gruvbox Dark ── */
			"body.t-gruvbox{background:#282828;color:#ebdbb2;}\n"
			"body.t-gruvbox h1,body.t-gruvbox h2{color:#83a598;}\n"
			"body.t-gruvbox summary{color:#8ec07c;}\n"
			"body.t-gruvbox details[open]>summary{color:#fe8019;}\n"
			"body.t-gruvbox th{background:#3c3836;color:#83a598;border-bottom:1px solid #504945;}\n"
			"body.t-gruvbox td{border-bottom:1px solid #3c3836;color:#ebdbb2;}\n"
			"body.t-gruvbox td:first-child{color:#fabd2f;}\n"
			"body.t-gruvbox td:nth-child(2){color:#83a598;}\n"
			"body.t-gruvbox tr:hover td{background:#3c3836;}\n"
			"body.t-gruvbox #theme-bar select{background:#3c3836;color:#ebdbb2;border:1px solid #504945;}\n"
			/* ── One Dark ── */
			"body.t-onedark{background:#282c34;color:#abb2bf;}\n"
			"body.t-onedark h1,body.t-onedark h2{color:#61afef;}\n"
			"body.t-onedark summary{color:#56b6c2;}\n"
			"body.t-onedark details[open]>summary{color:#e5c07b;}\n"
			"body.t-onedark th{background:#2c313c;color:#61afef;border-bottom:1px solid #3e4451;}\n"
			"body.t-onedark td{border-bottom:1px solid #2c313c;color:#abb2bf;}\n"
			"body.t-onedark td:first-child{color:#e5c07b;}\n"
			"body.t-onedark td:nth-child(2){color:#56b6c2;}\n"
			"body.t-onedark tr:hover td{background:#2c313c;}\n"
			"body.t-onedark #theme-bar select{background:#2c313c;color:#abb2bf;border:1px solid #3e4451;}\n"
			/* ── Gruvbox Light ── */
			"body.t-gruvbox-light{background:#fbf1c7;color:#3c3836;}\n"
			"body.t-gruvbox-light h1,body.t-gruvbox-light h2{color:#076678;}\n"
			"body.t-gruvbox-light summary{color:#427b58;}\n"
			"body.t-gruvbox-light details[open]>summary{color:#af3a03;}\n"
			"body.t-gruvbox-light th{background:#ebdbb2;color:#076678;border-bottom:1px solid #bdae93;}\n"
			"body.t-gruvbox-light td{border-bottom:1px solid #ebdbb2;color:#3c3836;}\n"
			"body.t-gruvbox-light td:first-child{color:#b57614;}\n"
			"body.t-gruvbox-light td:nth-child(2){color:#076678;}\n"
			"body.t-gruvbox-light tr:hover td{background:#ebdbb2;}\n"
			"body.t-gruvbox-light #theme-bar select{background:#ebdbb2;color:#3c3836;border:1px solid #bdae93;}\n"
			/* ── GitHub Light ── */
			"body.t-github{background:#ffffff;color:#24292e;}\n"
			"body.t-github h1,body.t-github h2{color:#0366d6;}\n"
			"body.t-github summary{color:#22863a;}\n"
			"body.t-github details[open]>summary{color:#e36209;}\n"
			"body.t-github th{background:#f6f8fa;color:#0366d6;border-bottom:1px solid #dfe2e5;}\n"
			"body.t-github td{border-bottom:1px solid #eaecef;color:#24292e;}\n"
			"body.t-github td:first-child{color:#6f42c1;}\n"
			"body.t-github td:nth-child(2){color:#0366d6;}\n"
			"body.t-github tr:hover td{background:#f6f8fa;}\n"
			"body.t-github #theme-bar select{background:#f6f8fa;color:#24292e;border:1px solid #dfe2e5;}\n"
			/* ── Solarized Light ── */
			"body.t-solarized-light{background:#fdf6e3;color:#657b83;}\n"
			"body.t-solarized-light h1,body.t-solarized-light h2{color:#268bd2;}\n"
			"body.t-solarized-light summary{color:#2aa198;}\n"
			"body.t-solarized-light details[open]>summary{color:#cb4b16;}\n"
			"body.t-solarized-light th{background:#eee8d5;color:#268bd2;border-bottom:1px solid #93a1a1;}\n"
			"body.t-solarized-light td{border-bottom:1px solid #eee8d5;color:#657b83;}\n"
			"body.t-solarized-light td:first-child{color:#b58900;}\n"
			"body.t-solarized-light td:nth-child(2){color:#268bd2;}\n"
			"body.t-solarized-light tr:hover td{background:#eee8d5;}\n"
			"body.t-solarized-light #theme-bar select{background:#eee8d5;color:#657b83;border:1px solid #93a1a1;}\n"
			/* ── Retro/CRT ── */
			"body.t-retro{background:#0a0a0a;color:#00ff00;}\n"
			"body.t-retro h1,body.t-retro h2{color:#00ff00;text-shadow:0 0 6px #00ff00;}\n"
			"body.t-retro summary{color:#00cc00;}\n"
			"body.t-retro details[open]>summary{color:#00ff00;text-shadow:0 0 4px #00ff00;}\n"
			"body.t-retro th{background:#001a00;color:#00ff00;border-bottom:1px solid #00ff00;}\n"
			"body.t-retro td{border-bottom:1px solid #002200;color:#00ee00;}\n"
			"body.t-retro td:first-child{color:#00ff00;}\n"
			"body.t-retro td:nth-child(2){color:#00cc00;}\n"
			"body.t-retro tr:hover td{background:#001a00;}\n"
			"body.t-retro #theme-bar select{background:#001a00;color:#00ff00;border:1px solid #00ff00;}\n"
			/* ── Cyberpunk ── */
			"body.t-cyberpunk{background:#0d0d1a;color:#e0e0ff;}\n"
			"body.t-cyberpunk h1,body.t-cyberpunk h2{color:#ff2d78;text-shadow:0 0 6px #ff2d78;}\n"
			"body.t-cyberpunk summary{color:#00fff9;}\n"
			"body.t-cyberpunk details[open]>summary{color:#ff2d78;}\n"
			"body.t-cyberpunk th{background:#1a0a2e;color:#00fff9;border-bottom:1px solid #ff2d78;}\n"
			"body.t-cyberpunk td{border-bottom:1px solid #1a0a2e;color:#e0e0ff;}\n"
			"body.t-cyberpunk td:first-child{color:#ffe600;}\n"
			"body.t-cyberpunk td:nth-child(2){color:#00fff9;}\n"
			"body.t-cyberpunk tr:hover td{background:#1a0a2e;}\n"
			"body.t-cyberpunk #theme-bar select{background:#1a0a2e;color:#e0e0ff;border:1px solid #ff2d78;}\n"
			"</style>\n"
			"<script>\n"
			"function setTheme(t){"
			"document.body.className='t-'+t;"
			"localStorage.setItem('wdump-theme',t);}\n"
			"window.onload=function(){"
			"var s=localStorage.getItem('wdump-theme')||'dark';"
			"setTheme(s);"
			"document.getElementById('theme-sel').value=s;};\n"
			"</script>\n"
			"</head>\n<body class=\"t-dark\">\n", out);

		fputs("<div id=\"theme-bar\">Theme: "
			"<select id=\"theme-sel\" onchange=\"setTheme(this.value)\">"
			"<option value=\"dark\">Dark</option>"
			"<option value=\"light\">Light</option>"
			"<option value=\"monokai\">Monokai</option>"
			"<option value=\"solarized\">Solarized Dark</option>"
			"<option value=\"hc\">High Contrast</option>"
			"<optgroup label=\"── Dark Variants ──\">"
			"<option value=\"dracula\">Dracula</option>"
			"<option value=\"nord\">Nord</option>"
			"<option value=\"tokyo\">Tokyo Night</option>"
			"<option value=\"gruvbox\">Gruvbox Dark</option>"
			"<option value=\"onedark\">One Dark</option>"
			"</optgroup>"
			"<optgroup label=\"── Light Variants ──\">"
			"<option value=\"gruvbox-light\">Gruvbox Light</option>"
			"<option value=\"github\">GitHub Light</option>"
			"<option value=\"solarized-light\">Solarized Light</option>"
			"</optgroup>"
			"<optgroup label=\"── Specialty ──\">"
			"<option value=\"retro\">Retro / CRT</option>"
			"<option value=\"cyberpunk\">Cyberpunk</option>"
			"</optgroup>"
			"</select></div>\n", out);

		fprintf(out, "<h1>File Loaded: "); HtmlEscape(out, currentFilePath); fputs("</h1>\n", out);

		// Animation summary block (only when relevant chunks are present).
		bool hasAnimContent = false;
		for (int i = 0; i < master->subchunks.Count(); i++)
		{
			if (IsAnimationTopLevelChunk(master->subchunks[i]->name))
			{
				hasAnimContent = true;
				break;
			}
		}
		if (hasAnimContent)
			WriteAnimationSummary(out);

		// Group W3D_CHUNK_MESH chunks under their ContainerName, mirroring the
		// treeview layout. One open container <details> at a time; close it
		// when the container changes or a non-mesh chunk is encountered.
		std::string openContainer;
		for (int i = 0; i < master->subchunks.Count(); i++)
		{
			ChunkData *sub = master->subchunks[i];
			if (animationOnly && !IsAnimationTopLevelChunk(sub->name)) continue;

			if (sub->name == "W3D_CHUNK_MESH")
			{
				const ChunkData *header   = FindMeshHeader(sub);
				const char *meshName      = header ? FindInfoValue(header, "MeshName")      : nullptr;
				const char *containerName = header ? FindInfoValue(header, "ContainerName") : nullptr;
				std::string key = (containerName && *containerName) ? containerName : "(legacy)";

				if (key != openContainer)
				{
					if (!openContainer.empty())
						fputs("</details>\n", out);
					fputs("<details open><summary>", out);
					HtmlEscape(out, key.c_str());
					fputs("</summary>\n", out);
					openContainer = key;
				}

				fputs("<details><summary>", out);
				HtmlEscape(out, (meshName && *meshName) ? meshName : sub->name.Peek_Buffer());
				fputs("</summary>\n", out);
				DumpDataHtml(out, unk, sub, 0);
				fputs("</details>\n", out);
			}
			else
			{
				if (!openContainer.empty())
				{
					fputs("</details>\n", out);
					openContainer.clear();
				}
				fputs("<details open><summary>", out);
				HtmlEscape(out, sub->name.Peek_Buffer());
				fputs("</summary>\n", out);
				DumpDataHtml(out, unk, sub, 0);
				fputs("</details>\n", out);
			}
		}
		if (!openContainer.empty())
			fputs("</details>\n", out);

		fputs("<p style=\"color:#f44;margin-top:1em;\">Generated by CABAL.</p>\n"
			"</body>\n</html>\n", out);
	}
	else
	{
		fprintf(out, "File Loaded: %s\n\n", currentFilePath);
		for (int i = 0; i < master->subchunks.Count(); i++)
		{
			ChunkData *sub = master->subchunks[i];
			if (animationOnly && !IsAnimationTopLevelChunk(sub->name)) continue;
			fprintf(out, "%s\n", sub->name.Peek_Buffer());
			DumpData(out, unk, sub, "\t", false);
		}
		fputs("Generated by CABAL.\n", out);
	}

	fclose(unk);
	fclose(out);

	// Mirror the CLI behaviour: drop the .unk file when nothing unknown was logged.
	HANDLE h = CreateFileA(unkPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h != INVALID_HANDLE_VALUE)
	{
		DWORD size = GetFileSize(h, nullptr);
		CloseHandle(h);
		if (!size) DeleteFileA(unkPath);
	}
	return true;
}

// Looks up a named ChunkInfo on a ChunkData (linear scan; parsed mesh
// headers contain at most a few dozen entries).
static const char *FindInfoValue(const ChunkData *data, const char *name)
{
	if (!data) return nullptr;
	for (int i = 0; i < data->data.Count(); i++)
	{
		if (data->data[i]->name == name)
		{
			return data->data[i]->value.Peek_Buffer();
		}
	}
	return nullptr;
}

// Locates the W3D_CHUNK_MESH_HEADER or W3D_CHUNK_MESH_HEADER3 child of a
// W3D_CHUNK_MESH and returns it; nullptr if neither is present.
static const ChunkData *FindMeshHeader(const ChunkData *meshChunk)
{
	for (int i = 0; i < meshChunk->subchunks.Count(); i++)
	{
		const ChunkData *sc = meshChunk->subchunks[i];
		if (sc->name == "W3D_CHUNK_MESH_HEADER" ||
			sc->name == "W3D_CHUNK_MESH_HEADER3")
		{
			return sc;
		}
	}
	return nullptr;
}

// Lowercased copy for case-insensitive filter compare.
static std::wstring ToLowerW(const wchar_t *s)
{
	std::wstring out = s ? s : L"";
	for (size_t i = 0; i < out.size(); i++)
		out[i] = (wchar_t)towlower(out[i]);
	return out;
}

// ASCII-lowercased filter, kept in sync with g_filterText. Chunk names and
// MeshName/ContainerName values are ASCII, so we can avoid per-node UTF
// conversion during the filter walk.
static std::string g_filterTextA;

// Per-rebuild memo of "this chunk or a descendant matches the filter".
// Without this the recursive AddItems would re-walk every subtree from each
// ancestor, blowing rebuild time up to O(N * depth) on deep trees.
static std::unordered_map<const ChunkData *, bool> g_filterCache;

static const char *MeshDisplayName(ChunkData *meshChunk);

static bool ContainsCI(const char *hay, const char *needle)
{
	if (!*needle) return true;
	for (; *hay; hay++)
	{
		const char *h = hay; const char *n = needle;
		while (*n && *h && (char)tolower((unsigned char)*h) == *n) { h++; n++; }
		if (!*n) return true;
	}
	return false;
}

// Does this chunk's own name (or, for meshes, its display MeshName) match
// the active filter? No recursion into children.
static bool ChunkMatchesSelf(const ChunkData *data)
{
	if (g_filterTextA.empty()) return true;
	if (ContainsCI(data->name.Peek_Buffer(), g_filterTextA.c_str())) return true;
	if (data->name == "W3D_CHUNK_MESH")
	{
		const char *display = MeshDisplayName(const_cast<ChunkData *>(data));
		if (display && ContainsCI(display, g_filterTextA.c_str())) return true;
	}
	return false;
}

// Should this chunk appear in the filtered tree at all? True if the chunk
// itself matches OR any descendant matches (so we keep ancestors as
// breadcrumbs to the match). Memoized per rebuild via g_filterCache.
static bool ChunkMatchesFilter(const ChunkData *data)
{
	if (g_filterTextA.empty()) return true;
	auto it = g_filterCache.find(data);
	if (it != g_filterCache.end()) return it->second;
	bool match = ChunkMatchesSelf(data);
	if (!match)
	{
		for (int i = 0; i < data->subchunks.Count(); i++)
			if (ChunkMatchesFilter(data->subchunks[i])) { match = true; break; }
	}
	g_filterCache.emplace(data, match);
	return match;
}

static const char *MeshDisplayName(ChunkData *meshChunk)
{
	const ChunkData *header = FindMeshHeader(meshChunk);
	const char *meshName = header ? FindInfoValue(header, "MeshName") : nullptr;
	return (meshName && *meshName) ? meshName : meshChunk->name.Peek_Buffer();
}

// UTF-8/ASCII to UTF-16 into a caller-provided buffer (avoids the
// WideStringClass allocation per insert). Chunk names fit comfortably in
// 256 wchar_t; oversize names get truncated rather than allocating.
static const wchar_t *WidenInto(wchar_t *buf, size_t cap, const char *s)
{
	if (!s) { buf[0] = 0; return buf; }
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, buf, (int)cap);
	if (n <= 0) { buf[0] = 0; }
	else if ((size_t)n > cap) { buf[cap - 1] = 0; }
	return buf;
}

static void SortByDisplayName(std::vector<ChunkData *> &v)
{
	std::sort(v.begin(), v.end(), [](ChunkData *a, ChunkData *b) {
		const char *na = (a->name == "W3D_CHUNK_MESH") ? MeshDisplayName(a) : a->name.Peek_Buffer();
		const char *nb = (b->name == "W3D_CHUNK_MESH") ? MeshDisplayName(b) : b->name.Peek_Buffer();
		return _stricmp(na, nb) < 0;
	});
}

// `forceAll` means: ancestor already matched the filter, so include this
// node and its entire subtree unconditionally. Without it, the filter
// would hide the matched node's children — defeating the point of
// searching for e.g. a mesh by name.
void AddItems(ChunkData *data, HTREEITEM item, bool forceAll)
{
	if (!forceAll && !ChunkMatchesFilter(data)) return;
	bool descendForceAll = forceAll || ChunkMatchesSelf(data);
	wchar_t wbuf[256];
	HTREEITEM newitem = TreeViewInsertItemP(treewnd,
		WidenInto(wbuf, 256, data->name.Peek_Buffer()), item, TVI_LAST, (LPARAM)data);
	if (g_viewMode == VIEW_ALPHABETICAL)
	{
		std::vector<ChunkData *> kids;
		kids.reserve(data->subchunks.Count());
		for (int i = 0; i < data->subchunks.Count(); i++)
			kids.push_back(data->subchunks[i]);
		SortByDisplayName(kids);
		for (size_t i = 0; i < kids.size(); i++)
			AddItems(kids[i], newitem, descendForceAll);
	}
	else
	{
		for (int i = 0; i < data->subchunks.Count(); i++)
			AddItems(data->subchunks[i], newitem, descendForceAll);
	}
}

// Inserts a single W3D_CHUNK_MESH under a parent treeview node, displaying
// the MeshName from its header (falls back to the raw chunk name).
static void AddMeshItem(ChunkData *meshChunk, HTREEITEM parent, bool forceAll)
{
	if (!forceAll && !ChunkMatchesFilter(meshChunk)) return;
	bool descendForceAll = forceAll || ChunkMatchesSelf(meshChunk);
	wchar_t wbuf[256];
	HTREEITEM mesh = TreeViewInsertItemP(treewnd,
		WidenInto(wbuf, 256, MeshDisplayName(meshChunk)), parent, TVI_LAST, (LPARAM)meshChunk);
	if (g_viewMode == VIEW_ALPHABETICAL)
	{
		std::vector<ChunkData *> kids;
		kids.reserve(meshChunk->subchunks.Count());
		for (int i = 0; i < meshChunk->subchunks.Count(); i++)
			kids.push_back(meshChunk->subchunks[i]);
		SortByDisplayName(kids);
		for (size_t i = 0; i < kids.size(); i++)
			AddItems(kids[i], mesh, descendForceAll);
	}
	else
	{
		for (int i = 0; i < meshChunk->subchunks.Count(); i++)
			AddItems(meshChunk->subchunks[i], mesh, descendForceAll);
	}
}

// Original-order layout: top-level meshes are grouped under synthetic
// ContainerName nodes; non-mesh chunks in original file order. This is the
// default view; alphabetical mode uses the same shape but sorts siblings.
static void AddTopLevelGrouped(ChunkData *root)
{
	std::unordered_map<std::string, HTREEITEM> containerNodes;
	std::vector<ChunkData *> children;
	children.reserve(root->subchunks.Count());
	for (int i = 0; i < root->subchunks.Count(); i++)
		children.push_back(root->subchunks[i]);
	if (g_viewMode == VIEW_ALPHABETICAL)
		SortByDisplayName(children);
	wchar_t wbuf[256];
	for (size_t i = 0; i < children.size(); i++)
	{
		ChunkData *child = children[i];
		if (child->name != "W3D_CHUNK_MESH")
		{
			AddItems(child, TVI_ROOT, false);
			continue;
		}
		if (!ChunkMatchesFilter(child)) continue;
		const ChunkData *header = FindMeshHeader(child);
		const char *container = header ? FindInfoValue(header, "ContainerName") : nullptr;
		std::string key = (container && *container) ? container : "(legacy)";
		auto it = containerNodes.find(key);
		HTREEITEM group;
		if (it == containerNodes.end())
		{
			group = TreeViewInsertItemP(treewnd,
				WidenInto(wbuf, 256, key.c_str()), TVI_ROOT, TVI_LAST, (LPARAM)0);
			containerNodes.emplace(std::move(key), group);
		}
		else
		{
			group = it->second;
		}
		AddMeshItem(child, group, false);
	}
}

// Flat layout: skip the synthetic ContainerName grouping, show meshes at
// root in (optionally sorted) order alongside other chunks.
static void AddTopLevelFlat(ChunkData *root)
{
	std::vector<ChunkData *> children;
	children.reserve(root->subchunks.Count());
	for (int i = 0; i < root->subchunks.Count(); i++)
		children.push_back(root->subchunks[i]);
	if (g_viewMode == VIEW_ALPHABETICAL)
		SortByDisplayName(children);
	for (size_t i = 0; i < children.size(); i++)
	{
		ChunkData *child = children[i];
		if (!ChunkMatchesFilter(child)) continue;
		if (child->name == "W3D_CHUNK_MESH")
			AddMeshItem(child, TVI_ROOT, false);
		else
			AddItems(child, TVI_ROOT, false);
	}
}

// By-type layout: bucket every top-level chunk under a synthetic node named
// after its chunk type. Useful for navigating large files where you want
// "all animations" or "all hierarchies" in one place.
static void AddTopLevelByType(ChunkData *root)
{
	std::unordered_map<std::string, HTREEITEM> typeNodes;
	std::vector<ChunkData *> children;
	children.reserve(root->subchunks.Count());
	for (int i = 0; i < root->subchunks.Count(); i++)
		children.push_back(root->subchunks[i]);
	if (g_viewMode == VIEW_ALPHABETICAL)
		SortByDisplayName(children);
	wchar_t wbuf[256];
	for (size_t i = 0; i < children.size(); i++)
	{
		ChunkData *child = children[i];
		if (!ChunkMatchesFilter(child)) continue;
		std::string key = child->name.Peek_Buffer();
		auto it = typeNodes.find(key);
		HTREEITEM group;
		if (it == typeNodes.end())
		{
			group = TreeViewInsertItemP(treewnd,
				WidenInto(wbuf, 256, key.c_str()), TVI_ROOT, TVI_LAST, (LPARAM)0);
			typeNodes.emplace(std::move(key), group);
		}
		else
		{
			group = it->second;
		}
		if (child->name == "W3D_CHUNK_MESH")
			AddMeshItem(child, group, false);
		else
			AddItems(child, group, false);
	}
}

static void AddTopLevelItems(ChunkData *root)
{
	switch (g_viewMode)
	{
	case VIEW_BY_TYPE: AddTopLevelByType(root); break;
	case VIEW_FLAT:    AddTopLevelFlat(root); break;
	case VIEW_ORIGINAL:
	case VIEW_ALPHABETICAL:
	default:           AddTopLevelGrouped(root); break;
	}
}

// Forward decls for view-mode plumbing used from WM_COMMAND.
static void RebuildTree();
static void UpdateViewMenuChecks();

ChunkData *master = nullptr;
char currentFilePath[MAX_PATH] = "";
bool beautifyDump = true;

// Returns the path stem (everything before the final dot, after the final
// directory separator) of the currently loaded file. Used to prefill dump
// save dialogs with "<basename>_FULL.txt" / "<basename>_Ani.txt".
static StringClass CurrentFileStem()
{
	const char *slash = strrchr(currentFilePath, '\\');
	const char *fwd = strrchr(currentFilePath, '/');
	if (fwd > slash) slash = fwd;
	const char *base = slash ? slash + 1 : currentFilePath;
	const char *dot = strrchr(base, '.');
	char buf[MAX_PATH];
	if (dot)
	{
		size_t len = (size_t)(dot - base);
		if (len >= sizeof(buf)) len = sizeof(buf) - 1;
		memcpy(buf, base, len);
		buf[len] = '\0';
	}
	else
	{
		strncpy(buf, base, sizeof(buf) - 1);
		buf[sizeof(buf) - 1] = '\0';
	}
	return StringClass(buf);
}

// Loads a w3d-style file by absolute path: tears down the previous tree,
// parses chunks into a fresh master, and repopulates the treeview. Shared
// by the File -> Open menu and the WM_DROPFILES drag-and-drop handler.
static void LoadFile(const char *path)
{
	TreeView_SetItemState(treewnd, TreeView_GetSelection(treewnd), 0, TVIS_SELECTED);
	TreeView_DeleteAllItems(treewnd);
	BufferedFileClass file(path);
	file.Open(1);
	ChunkLoadClass cload(&file);
	if (master)
	{
		delete master;
	}
	master = new ChunkData;
	ParseSubchunks(cload, master);
	RebuildTree();
	strncpy(currentFilePath, path, sizeof(currentFilePath) - 1);
	currentFilePath[sizeof(currentFilePath) - 1] = '\0';

	char title[MAX_PATH + 32];
	_snprintf(title, sizeof(title), "wdump - %s", currentFilePath);
	title[sizeof(title) - 1] = '\0';
	SetWindowTextA(mainwnd, title);
}

static void ApplySplitter()
{
	int contentH = max(0, mainheight - statusHeight);
	splitterX = max(50, min(splitterX, mainwidth - 50 - SPLITTER_WIDTH));
	if (filterwnd)
		SetWindowPos(filterwnd, nullptr, 0, 0, splitterX, FILTER_HEIGHT, SWP_NOZORDER);
	SetWindowPos(treewnd, nullptr, 0, FILTER_HEIGHT, splitterX, max(0, contentH - FILTER_HEIGHT), SWP_NOZORDER);
	SetWindowPos(listwnd, nullptr, splitterX + SPLITTER_WIDTH, 0, mainwidth - splitterX - SPLITTER_WIDTH, contentH, SWP_NOZORDER);
	if (progresswnd)
	{
		// Place progress bar in the right portion of the status bar (last
		// part). Status bar handles its own positioning via WM_SIZE.
		RECT pr;
		SendMessage(statuswnd, SB_GETRECT, 1, (LPARAM)&pr);
		MapWindowPoints(statuswnd, mainwnd, (POINT *)&pr, 2);
		SetWindowPos(progresswnd, nullptr, pr.left, pr.top, pr.right - pr.left, pr.bottom - pr.top, SWP_NOZORDER);
	}
}

static int CountChunks(const ChunkData *data)
{
	int n = 1;
	for (int i = 0; i < data->subchunks.Count(); i++)
		n += CountChunks(data->subchunks[i]);
	return n;
}

static int CountChunksFiltered(const ChunkData *data)
{
	int n = ChunkMatchesFilter(data) ? 1 : 0;
	for (int i = 0; i < data->subchunks.Count(); i++)
		n += CountChunksFiltered(data->subchunks[i]);
	return n;
}

static void SetStatusText(const wchar_t *text)
{
	if (statuswnd) SendMessageW(statuswnd, SB_SETTEXTW, 0, (LPARAM)text);
}

static void ShowProgressMarquee(bool on)
{
	if (!progresswnd) return;
	ShowWindow(progresswnd, on ? SW_SHOW : SW_HIDE);
	SendMessage(progresswnd, PBM_SETMARQUEE, on ? TRUE : FALSE, 30);
}

// Tear down and repopulate the tree from `master` using the current view
// mode and filter text. Disables redraw and fade-expandos during the
// rebuild — the latter halves insert cost on large trees because the tree
// otherwise schedules a fade animation per node with children.
static void RebuildTree()
{
	if (!master) return;
	SetStatusText(L"Rebuilding tree...");
	ShowProgressMarquee(true);
	UpdateWindow(statuswnd);
	UpdateWindow(progresswnd);

	SendMessage(treewnd, WM_SETREDRAW, FALSE, 0);
	TreeView_SetExtendedStyle(treewnd, 0, TVS_EX_FADEINOUTEXPANDOS);
	TreeView_SetItemState(treewnd, TreeView_GetSelection(treewnd), 0, TVIS_SELECTED);
	TreeView_DeleteAllItems(treewnd);
	g_filterCache.clear();
	AddTopLevelItems(master);

	int total = 0, shown = 0;
	for (int i = 0; i < master->subchunks.Count(); i++)
	{
		total += CountChunks(master->subchunks[i]);
		shown += CountChunksFiltered(master->subchunks[i]);
	}
	// Filter cache holds pointers into `master`; safe across rebuilds, but
	// we drop it to keep memory steady for very large files.
	g_filterCache.clear();
	TreeView_SetExtendedStyle(treewnd, TVS_EX_FADEINOUTEXPANDOS, TVS_EX_FADEINOUTEXPANDOS);
	SendMessage(treewnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(treewnd, nullptr, TRUE);

	ShowProgressMarquee(false);
	wchar_t status[128];
	if (g_filterTextA.empty())
		swprintf(status, 128, L"%d chunks", total);
	else
		swprintf(status, 128, L"Filtered: %d / %d chunks", shown, total);
	SetStatusText(status);
}

static WNDPROC g_origFilterProc = nullptr;

// Filter edit subclass: rebuild on Enter only. Swallow Enter so the edit
// doesn't beep, and pass everything else through. Escape clears the filter
// and rebuilds (small affordance — easier than selecting all + delete).
static LRESULT CALLBACK FilterEditProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN && wParam == VK_RETURN)
	{
		wchar_t buf[256];
		GetWindowTextW(hWnd, buf, 256);
		g_filterText = ToLowerW(buf);
		char abuf[256];
		int n = WideCharToMultiByte(CP_UTF8, 0, g_filterText.c_str(), -1, abuf, 256, nullptr, nullptr);
		g_filterTextA.assign(abuf, n > 0 ? n - 1 : 0);
		RebuildTree();
		return 0;
	}
	if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE)
	{
		SetWindowTextW(hWnd, L"");
		g_filterText.clear();
		g_filterTextA.clear();
		RebuildTree();
		return 0;
	}
	if (uMsg == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE))
		return 0; // suppress MessageBeep from default EDIT behavior
	return CallWindowProc(g_origFilterProc, hWnd, uMsg, wParam, lParam);
}

static void UpdateViewMenuChecks()
{
	UINT ids[4] = { ID_VIEW_ORIGINAL, ID_VIEW_ALPHABETICAL, ID_VIEW_BY_TYPE, ID_VIEW_FLAT };
	for (int i = 0; i < 4; i++)
		CheckMenuItem(menu, ids[i],
			MF_BYCOMMAND | (((int)g_viewMode == i) ? MF_CHECKED : MF_UNCHECKED));
}

// Append `count` copies of `c` to `out` — small helper to keep the
// serializer below readable.
static void AppendIndent(std::string &out, int count, char c = ' ')
{
	out.append((size_t)count, c);
}

// Serialize one chunk and (filtered) descendants into `out`. Mirrors what
// the user sees in the tree: nodes hidden by the filter are skipped, but
// once a node self-matches its full subtree is included (same rule as
// AddItems' forceAll).
static void SerializeChunkSubtree(const ChunkData *data, std::string &out, int depth, bool forceAll)
{
	if (!forceAll && !ChunkMatchesFilter(data)) return;
	bool descendForceAll = forceAll || ChunkMatchesSelf(data);

	const char *displayName = data->name.Peek_Buffer();
	if (data->name == "W3D_CHUNK_MESH")
		displayName = MeshDisplayName(const_cast<ChunkData *>(data));

	AppendIndent(out, depth * 2);
	out.append("[");
	out.append(displayName);
	out.append("]\r\n");

	for (int i = 0; i < data->data.Count(); i++)
	{
		const ChunkInfo *ci = data->data[i];
		AppendIndent(out, depth * 2 + 2);
		out.append(ci->name.Peek_Buffer());
		out.append("\t");
		out.append(ci->type.Peek_Buffer());
		out.append("\t");
		out.append(ci->value.Peek_Buffer());
		out.append("\r\n");
	}

	for (int i = 0; i < data->subchunks.Count(); i++)
		SerializeChunkSubtree(data->subchunks[i], out, depth + 1, descendForceAll);
}

// Copy the indented-text serialization of the selected subtree to the
// clipboard as CF_UNICODETEXT (consistent with the listview copy path).
static void CopyTreeSubtreeToClipboard()
{
	HTREEITEM sel = TreeView_GetSelection(treewnd);
	if (!sel) return;
	ChunkData *cd = (ChunkData *)TreeViewGetItem(treewnd, sel);
	if (!cd) return; // synthetic group node (ContainerName / by-type bucket)

	std::string out;
	out.reserve(4096);
	SerializeChunkSubtree(cd, out, 0, false);
	if (out.empty()) return;

	int wlen = MultiByteToWideChar(CP_UTF8, 0, out.c_str(), (int)out.size(), nullptr, 0);
	if (wlen <= 0) return;
	size_t bytes = ((size_t)wlen + 1) * sizeof(wchar_t);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
	if (!hMem) return;
	wchar_t *p = (wchar_t *)GlobalLock(hMem);
	if (!p) { GlobalFree(hMem); return; }
	MultiByteToWideChar(CP_UTF8, 0, out.c_str(), (int)out.size(), p, wlen);
	p[wlen] = 0;
	GlobalUnlock(hMem);

	if (OpenClipboard(mainwnd))
	{
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
		CloseClipboard();
	}
	else GlobalFree(hMem);
}

// RFC-4180 CSV field: quote if it contains comma, quote, CR or LF; double
// internal quotes. Excel handles .csv natively (no import wizard) and
// honors the quoting rules, unlike .tsv which it treats as a single column.
static void AppendCSVField(std::string &out, const char *s)
{
	if (!s) { return; }
	bool needsQuote = false;
	for (const char *p = s; *p; p++)
	{
		if (*p == ',' || *p == '"' || *p == '\r' || *p == '\n') { needsQuote = true; break; }
	}
	if (!needsQuote) { out.append(s); return; }
	out.push_back('"');
	for (; *s; s++)
	{
		if (*s == '"') out.append("\"\"");
		else out.push_back(*s);
	}
	out.push_back('"');
}

// Flatten the chunk subtree into CSV rows. Each data field becomes its own
// row; chunks with no data still emit one row (so empty container chunks
// remain visible in Excel). Path column is "/"-joined chunk names from the
// dump root, which makes it easy to filter/group in Excel.
static void SerializeChunkSubtreeCSV(const ChunkData *data, std::string &out, std::string &path)
{
	const char *displayName = data->name.Peek_Buffer();
	if (data->name == "W3D_CHUNK_MESH")
		displayName = MeshDisplayName(const_cast<ChunkData *>(data));

	size_t pathBefore = path.size();
	if (!path.empty()) path.push_back('/');
	path.append(displayName);

	if (data->data.Count() == 0)
	{
		AppendCSVField(out, path.c_str()); out.push_back(',');
		AppendCSVField(out, displayName);  out.push_back(',');
		out.append(",,\r\n"); // empty Name/Type/Value
	}
	else
	{
		for (int i = 0; i < data->data.Count(); i++)
		{
			const ChunkInfo *ci = data->data[i];
			AppendCSVField(out, path.c_str());           out.push_back(',');
			AppendCSVField(out, displayName);            out.push_back(',');
			AppendCSVField(out, ci->name.Peek_Buffer()); out.push_back(',');
			AppendCSVField(out, ci->type.Peek_Buffer()); out.push_back(',');
			AppendCSVField(out, ci->value.Peek_Buffer());
			out.append("\r\n");
		}
	}

	for (int i = 0; i < data->subchunks.Count(); i++)
		SerializeChunkSubtreeCSV(data->subchunks[i], out, path);

	path.resize(pathBefore);
}

// Same payload as Copy subtree but written to a file. Plain text only —
// the indented serializer doesn't produce HTML, and "Dump selected" is
// meant for grep/diff/editor consumption rather than presentation.
static void DumpSelectedSubtree()
{
	HTREEITEM sel = TreeView_GetSelection(treewnd);
	if (!sel)
	{
		MessageBox(mainwnd, L"Select a tree node first.", L"wdump", MB_OK | MB_ICONINFORMATION);
		return;
	}
	ChunkData *cd = (ChunkData *)TreeViewGetItem(treewnd, sel);
	if (!cd)
	{
		MessageBox(mainwnd, L"Selected node has no chunk data (it's a synthetic group).",
			L"wdump", MB_OK | MB_ICONINFORMATION);
		return;
	}

	// Best-effort label for the default filename: mesh display name for
	// meshes, raw chunk name otherwise. Strip the W3D_CHUNK_ prefix to
	// keep the suggested filename short.
	const char *label = (cd->name == "W3D_CHUNK_MESH")
		? MeshDisplayName(cd) : cd->name.Peek_Buffer();
	const char *trimmed = label;
	if (strncmp(trimmed, "W3D_CHUNK_", 10) == 0) trimmed += 10;

	char fname[MAX_PATH];
	StringClass stem = CurrentFileStem();
	_snprintf(fname, MAX_PATH, "%s_SEL_%s.txt",
		stem.Peek_Buffer(), (trimmed && *trimmed) ? trimmed : "node");
	if (!GetSaveFile(fname,
		L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0",
		L"txt", mainwnd, L"Dump selected subtree"))
		return;

	// Dump ignores the active filter — the user navigated to this chunk
	// specifically, so they want the complete subtree, not the filtered
	// projection of it.
	std::string out;
	out.reserve(65536);
	SerializeChunkSubtree(cd, out, 0, true);

	FILE *f = fopen(fname, "wb");
	if (!f)
	{
		MessageBox(mainwnd, L"Failed to open file for writing.", L"wdump", MB_OK | MB_ICONERROR);
		return;
	}
	fwrite(out.data(), 1, out.size(), f);
	fclose(f);
}

// Excel-friendly variant of DumpSelectedSubtree. Same selection rules, but
// the output is a flat CSV with header row: Path, Chunk, Name, Type, Value.
// CSV (not TSV) because Excel treats .tsv as a single column when opened
// directly — only .csv is parsed without the import wizard.
static void DumpSelectedSubtreeTSV()
{
	HTREEITEM sel = TreeView_GetSelection(treewnd);
	if (!sel)
	{
		MessageBox(mainwnd, L"Select a tree node first.", L"wdump", MB_OK | MB_ICONINFORMATION);
		return;
	}
	ChunkData *cd = (ChunkData *)TreeViewGetItem(treewnd, sel);
	if (!cd)
	{
		MessageBox(mainwnd, L"Selected node has no chunk data (it's a synthetic group).",
			L"wdump", MB_OK | MB_ICONINFORMATION);
		return;
	}

	const char *label = (cd->name == "W3D_CHUNK_MESH")
		? MeshDisplayName(cd) : cd->name.Peek_Buffer();
	const char *trimmed = label;
	if (strncmp(trimmed, "W3D_CHUNK_", 10) == 0) trimmed += 10;

	char fname[MAX_PATH];
	StringClass stem = CurrentFileStem();
	_snprintf(fname, MAX_PATH, "%s_SEL_%s.csv",
		stem.Peek_Buffer(), (trimmed && *trimmed) ? trimmed : "node");
	if (!GetSaveFile(fname,
		L"CSV Files (*.csv)\0*.csv\0All Files (*.*)\0*.*\0\0",
		L"csv", mainwnd, L"Dump selected subtree as CSV"))
		return;

	std::string out;
	out.reserve(65536);
	out.append("Path,Chunk,Name,Type,Value\r\n");
	std::string path;
	SerializeChunkSubtreeCSV(cd, out, path);

	FILE *f = fopen(fname, "wb");
	if (!f)
	{
		MessageBox(mainwnd, L"Failed to open file for writing.", L"wdump", MB_OK | MB_ICONERROR);
		return;
	}
	// UTF-8 BOM so Excel detects encoding correctly on direct double-click.
	const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
	fwrite(bom, 1, 3, f);
	fwrite(out.data(), 1, out.size(), f);
	fclose(f);
}

LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		break;
	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			mainwidth = LOWORD(lParam);
			mainheight = HIWORD(lParam);
			if (statuswnd)
			{
				SendMessage(statuswnd, WM_SIZE, 0, 0);
				int parts[2] = { mainwidth - 200, -1 };
				SendMessage(statuswnd, SB_SETPARTS, 2, (LPARAM)parts);
			}
			ApplySplitter();
		}
		break;
	case WM_SETCURSOR:
		if ((HWND)wParam == hWnd)
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			if (pt.x >= splitterX && pt.x < splitterX + SPLITTER_WIDTH)
			{
				SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
				return TRUE;
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam);
			if (x >= splitterX && x < splitterX + SPLITTER_WIDTH)
			{
				splitterDragging = true;
				SetCapture(hWnd);
				SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
			}
		}
		break;
	case WM_MOUSEMOVE:
		if (splitterDragging)
		{
			splitterX = GET_X_LPARAM(lParam);
			ApplySplitter();
			SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
		}
		break;
	case WM_LBUTTONUP:
		if (splitterDragging)
		{
			splitterDragging = false;
			ReleaseCapture();
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_OPEN:
			{
				char fname[MAX_PATH];
				fname[0] = 0;
				if (GetOpenFile(fname, L"W3D Files (*.w3d)\0*.w3d\0WLT Files (*.wlt)\0*.wlt\0WHT Files (*.wht)\0*.wht\0WHA Files (*.wha)\0*.wha\0WTM Files (*.wtm)\0*.wtm\0All Files (*.*)\0*.*\0\0", nullptr, mainwnd, L"Open File"))
				{
					LoadFile(fname);
				}
			}
			break;
		case ID_EXIT:
			SendMessage(mainwnd, WM_CLOSE, 0, 0);
			break;
		case ID_DUMP_ANIMATION:
		case ID_DUMP_ALL:
			{
				if (!master)
				{
					MessageBox(mainwnd, L"Open a W3D file first.", L"wdump", MB_OK | MB_ICONINFORMATION);
					break;
				}
				bool animationOnly = (LOWORD(wParam) == ID_DUMP_ANIMATION);
				const wchar_t *title = animationOnly ? L"Save animation dump" : L"Save full dump";
				char fname[MAX_PATH];
				StringClass stem = CurrentFileStem();
				const char *ext = beautifyDump ? "html" : "txt";
				_snprintf(fname, MAX_PATH, "%s%s.%s", stem.Peek_Buffer(),
					animationOnly ? "_Ani" : "_FULL", ext);
				const wchar_t *filter = beautifyDump
					? L"HTML Files (*.html)\0*.html\0All Files (*.*)\0*.*\0\0"
					: L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
				if (GetSaveFile(fname, filter, beautifyDump ? L"html" : L"txt", mainwnd, title))
				{
					if (!DumpMasterToPath(fname, animationOnly, beautifyDump))
					{
						MessageBox(mainwnd, L"Failed to write dump file.", L"wdump", MB_OK | MB_ICONERROR);
					}
				}
			}
			break;
		case ID_LIST_SELECTALL:
			ListView_SetItemState(listwnd, -1, LVIS_SELECTED, LVIS_SELECTED);
			SetFocus(listwnd);
			break;
		case ID_LIST_COPY:
			CopyListViewToClipboard();
			break;
		case ID_DUMP_TEXTURES:
			{
				if (!master)
				{
					MessageBox(mainwnd, L"Open a W3D file first.", L"wdump", MB_OK | MB_ICONINFORMATION);
					break;
				}
				const ChunkData *meshChunk = FindSelectedMesh();
				char fname[MAX_PATH];
				StringClass stem = CurrentFileStem();
				const char *ext = beautifyDump ? "html" : "txt";
				if (meshChunk)
				{
					const ChunkData *header = FindMeshHeader(meshChunk);
					const char *meshName = header ? FindInfoValue(header, "MeshName") : nullptr;
					_snprintf(fname, MAX_PATH, "%s_TEX_%s.%s",
						stem.Peek_Buffer(),
						(meshName && *meshName) ? meshName : "mesh", ext);
				}
				else
				{
					_snprintf(fname, MAX_PATH, "%s_TEX.%s", stem.Peek_Buffer(), ext);
				}
				const wchar_t *filter = beautifyDump
					? L"HTML Files (*.html)\0*.html\0All Files (*.*)\0*.*\0\0"
					: L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
				const wchar_t *title = meshChunk
					? L"Save mesh texture dump"
					: L"Save textures dump (all meshes)";
				if (GetSaveFile(fname, filter, beautifyDump ? L"html" : L"txt", mainwnd, title))
				{
					if (!DumpTexturesToPath(fname, meshChunk, beautifyDump))
					{
						MessageBox(mainwnd, L"Failed to write dump file.", L"wdump", MB_OK | MB_ICONERROR);
					}
				}
			}
			break;
		case ID_DUMP_BEAUTIFY:
			beautifyDump = !beautifyDump;
			CheckMenuItem(menu, ID_DUMP_BEAUTIFY,
				MF_BYCOMMAND | (beautifyDump ? MF_CHECKED : MF_UNCHECKED));
			break;
		case ID_VIEW_ORIGINAL:
		case ID_VIEW_ALPHABETICAL:
		case ID_VIEW_BY_TYPE:
		case ID_VIEW_FLAT:
			g_viewMode = (ViewMode)(LOWORD(wParam) - ID_VIEW_ORIGINAL);
			UpdateViewMenuChecks();
			RebuildTree();
			break;
		case ID_VIEW_FILTER:
			SetFocus(filterwnd);
			SendMessage(filterwnd, EM_SETSEL, 0, -1);
			break;
		case ID_TREE_COPY_SUBTREE:
			CopyTreeSubtreeToClipboard();
			break;
		case ID_DUMP_SELECTED:
			DumpSelectedSubtree();
			break;
		case ID_DUMP_SELECTED_TSV:
			DumpSelectedSubtreeTSV();
			break;
		default:
			return FALSE;
		}
		return TRUE;
	case WM_NOTIFY:
		{
			LPNMHDR hdr = (LPNMHDR)lParam;
			if (hdr->hwndFrom == treewnd && hdr->code == TVN_SELCHANGED)
			{
				LPNMTREEVIEW nm = (LPNMTREEVIEW)lParam;
				ChunkData *cd = (ChunkData *)TreeViewGetItem(treewnd, nm->itemNew.hItem);
				ListView_DeleteAllItems(listwnd);
				g_sortCol = -1; g_sortDir = 0; g_cellRow = -1; g_cellCol = -1;
				ListViewSetSortArrow(-1, 0);
				if (!cd) return TRUE; // synthetic group node has no chunk payload
				for (int i = 0; i < cd->subchunks.Count(); i++)
				{
					WideStringClass str = cd->subchunks[i]->name;
					int item = ListViewInsertItem(listwnd, 0xFFFF, str.Peek_Buffer());
					ListViewSetItemText(listwnd, item, 1, L"chunk");
					LVITEM li = {}; li.mask = LVIF_PARAM; li.iItem = item; li.lParam = item;
					ListView_SetItem(listwnd, &li);
				}
				for (int i = 0; i < cd->data.Count(); i++)
				{
					WideStringClass str = cd->data[i]->name;
					int item = ListViewInsertItem(listwnd, 0xFFFF, str.Peek_Buffer());
					str = cd->data[i]->type;
					ListViewSetItemText(listwnd, item, 1, str.Peek_Buffer());
					str = cd->data[i]->value;
					ListViewSetItemText(listwnd, item, 2, str.Peek_Buffer());
					LVITEM li = {}; li.mask = LVIF_PARAM; li.iItem = item; li.lParam = item;
					ListView_SetItem(listwnd, &li);
				}
				return TRUE;
			}
			if (hdr->hwndFrom == listwnd && hdr->code == NM_CLICK)
			{
				LVHITTESTINFO hti = {};
				GetCursorPos(&hti.pt);
				ScreenToClient(listwnd, &hti.pt);
				ListView_SubItemHitTest(listwnd, &hti);
				if (hti.iItem >= 0)
				{
					g_cellRow = hti.iItem;
					g_cellCol = hti.iSubItem;
				}
				else
				{
					g_cellRow = g_cellCol = -1;
				}
				ListView_RedrawItems(listwnd, 0, ListView_GetItemCount(listwnd) - 1);
				UpdateWindow(listwnd);
				return TRUE;
			}
			if (hdr->hwndFrom == listwnd && hdr->code == NM_CUSTOMDRAW)
			{
				LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
				switch (cd->nmcd.dwDrawStage)
				{
				case CDDS_PREPAINT:
					return CDRF_NOTIFYITEMDRAW;
				case CDDS_ITEMPREPAINT:
					// Suppress default selection paint; we draw the active cell ourselves.
					cd->nmcd.uItemState &= ~(CDIS_SELECTED | CDIS_FOCUS | CDIS_HOT);
					cd->clrTextBk = GetSysColor(COLOR_WINDOW);
					cd->clrText   = GetSysColor(COLOR_WINDOWTEXT);
					if ((int)cd->nmcd.dwItemSpec == g_cellRow)
						return CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					return CDRF_NEWFONT;
				case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
					if ((int)cd->nmcd.dwItemSpec == g_cellRow && cd->iSubItem == g_cellCol)
					{
						if (!g_listTheme)
							g_listTheme = OpenThemeData(listwnd, L"Explorer::ListView");

						RECT rc = {};
						// LVIR_BOUNDS on subitem 0 returns the entire row — use LVIR_LABEL there.
						ListView_GetSubItemRect(listwnd, g_cellRow, g_cellCol,
							g_cellCol == 0 ? LVIR_LABEL : LVIR_BOUNDS, &rc);

						int stateId = (GetFocus() == listwnd) ? LISS_SELECTED : LISS_SELECTEDNOTFOCUS;

						if (g_listTheme)
							DrawThemeBackground(g_listTheme, cd->nmcd.hdc,
								LVP_LISTITEM, stateId, &rc, nullptr);
						else
							FillRect(cd->nmcd.hdc, &rc, GetSysColorBrush(COLOR_HIGHLIGHT));

						wchar_t text[1024] = {};
						ListView_GetItemText(listwnd, g_cellRow, g_cellCol, text, 1024);
						RECT rcText = rc;
						rcText.left  += 4;
						rcText.right -= 4;
						int      oldBk  = SetBkMode(cd->nmcd.hdc, TRANSPARENT);
						COLORREF oldClr = SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_WINDOWTEXT));
						DrawText(cd->nmcd.hdc, text, -1, &rcText,
							DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
						SetTextColor(cd->nmcd.hdc, oldClr);
						SetBkMode(cd->nmcd.hdc, oldBk);
						return CDRF_SKIPDEFAULT;
					}
					return CDRF_DODEFAULT;
				}
				return CDRF_DODEFAULT;
			}
			if (hdr->hwndFrom == listwnd && hdr->code == LVN_COLUMNCLICK)
			{
				LPNMLISTVIEW nmlv = (LPNMLISTVIEW)lParam;
				int col = nmlv->iSubItem;
				if (g_sortCol == col)
				{
					if      (g_sortDir ==  1) g_sortDir = -1;
					else if (g_sortDir == -1) { g_sortDir = 0; g_sortCol = -1; }
					else                       g_sortDir =  1;
				}
				else
				{
					g_sortCol = col;
					g_sortDir = 1;
				}
				ListViewApplySort();
				return TRUE;
			}
			if (hdr->hwndFrom == treewnd && hdr->code == TVN_DELETEITEM)
			{
				LPNMTREEVIEW tvn = (LPNMTREEVIEW)lParam;
				TVITEM tv;
				tv.mask = TVIF_PARAM;
				tv.hItem = tvn->itemOld.hItem;
				tv.lParam = (LPARAM)0;
				TreeView_SetItem(treewnd, &tv);
				return TRUE;
			}
	}
		break;
	case WM_CONTEXTMENU:
		if ((HWND)wParam == listwnd)
		{
			HMENU ctx = CreatePopupMenu();
			AppendMenu(ctx, MF_STRING, ID_LIST_SELECTALL, L"Select All\tCtrl+A");
			AppendMenu(ctx, MF_STRING, ID_LIST_COPY,      L"Copy\tCtrl+C");
			int sel = ListView_GetNextItem(listwnd, -1, LVNI_SELECTED);
			if (sel < 0 && g_cellRow < 0) EnableMenuItem(ctx, ID_LIST_COPY, MF_BYCOMMAND | MF_GRAYED);
			TrackPopupMenu(ctx, TPM_RIGHTBUTTON,
				GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, mainwnd, nullptr);
			DestroyMenu(ctx);
			return 0;
		}
		if ((HWND)wParam == treewnd)
		{
			// Keyboard-initiated context menu reports (-1,-1); anchor under
			// the selected item in that case so the popup isn't off-screen.
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			HTREEITEM target = nullptr;
			if (x == -1 && y == -1)
			{
				target = TreeView_GetSelection(treewnd);
				RECT r;
				if (target && TreeView_GetItemRect(treewnd, target, &r, TRUE))
				{
					POINT pt = { r.left, r.bottom };
					ClientToScreen(treewnd, &pt);
					x = pt.x; y = pt.y;
				}
				else
				{
					POINT pt = { 0, 0 };
					ClientToScreen(treewnd, &pt);
					x = pt.x; y = pt.y;
				}
			}
			else
			{
				POINT pt = { x, y };
				ScreenToClient(treewnd, &pt);
				TVHITTESTINFO ht = {};
				ht.pt = pt;
				target = TreeView_HitTest(treewnd, &ht);
				if (target) TreeView_SelectItem(treewnd, target);
				else target = TreeView_GetSelection(treewnd);
			}

			HMENU ctx = CreatePopupMenu();
			AppendMenu(ctx, MF_STRING, ID_TREE_COPY_SUBTREE,  L"Copy subtree\tCtrl+Shift+C");
			AppendMenu(ctx, MF_STRING, ID_DUMP_SELECTED,      L"Dump selected...");
			AppendMenu(ctx, MF_STRING, ID_DUMP_SELECTED_TSV,  L"Dump selected as CSV (Excel)...");
			ChunkData *cd = target ? (ChunkData *)TreeViewGetItem(treewnd, target) : nullptr;
			if (!cd)
			{
				EnableMenuItem(ctx, ID_TREE_COPY_SUBTREE,  MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ctx, ID_DUMP_SELECTED,      MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ctx, ID_DUMP_SELECTED_TSV,  MF_BYCOMMAND | MF_GRAYED);
			}
			TrackPopupMenu(ctx, TPM_RIGHTBUTTON, x, y, 0, mainwnd, nullptr);
			DestroyMenu(ctx);
			return 0;
		}
		break;
	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;
			wchar_t wpath[MAX_PATH] = L"";
			if (DragQueryFileW(hDrop, 0, wpath, MAX_PATH))
			{
				char path[MAX_PATH];
				_snprintf(path, MAX_PATH, "%ls", wpath);
				LoadFile(path);
			}
			DragFinish(hDrop);
		}
		return 0;
	case WM_THEMECHANGED:
		if (g_listTheme) { CloseThemeData(g_listTheme); g_listTheme = nullptr; }
		return 0;
	case WM_CLOSE:
		if (g_listTheme) { CloseThemeData(g_listTheme); g_listTheme = nullptr; }
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#pragma warning(suppress: 28251) //warning C28251: Inconsistent annotation for function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	initmap();
	if (__argc > 1)
	{
		BufferedFileClass file(__argv[1]);
		file.Open(1);
		ChunkLoadClass cload(&file);
		master = new ChunkData;
		ParseSubchunks(cload, master);
		StringClass of = __argv[1];
		of += ".txt";
		StringClass uf = __argv[1];
		uf += ".unk";
		FILE *out = fopen(of, "wt");
		FILE *unknown = fopen(uf, "wt");
		DumpData(out, unknown, master, "", false);
		fclose(unknown);
		fclose(out);
		HANDLE h = CreateFileA(uf, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		DWORD size = GetFileSize(h, nullptr);
		CloseHandle(h);
		if (!size)
		{
			DeleteFileA(uf);
		}
		delete master;
		return 0;
	}
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	INITCOMMONCONTROLSEX ex;
	ex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ex.dwICC = ICC_WIN95_CLASSES | ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&ex);
	WNDCLASSEXW wcls = {};
	wcls.lpszClassName = CLASS_NAME;
	wcls.cbSize = sizeof(wcls);
	wcls.style = 0;
	wcls.lpfnWndProc = MainWindowProc;
	wcls.hIcon = LoadIcon(hInstance, (LPCWSTR)IDI_MAIN);
	wcls.hIconSm = LoadIcon(hInstance, (LPCWSTR)IDI_MAIN);
	wcls.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcls.cbClsExtra = 0;
	wcls.cbWndExtra = 0;
	wcls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wcls.hInstance = hInstance;
	RegisterClassEx(&wcls);

	DWORD window_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS;
	RECT window_rect = { 0, 0, 1024, 768 };
	AdjustWindowRect(&window_rect, window_style, false);

	menu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAINMENU));
	CheckMenuItem(menu, ID_DUMP_BEAUTIFY, MF_BYCOMMAND | MF_CHECKED);
	CheckMenuItem(menu, ID_VIEW_ORIGINAL, MF_BYCOMMAND | MF_CHECKED);
	mainwidth = window_rect.right - window_rect.left;
	mainheight = window_rect.bottom - window_rect.top;

	mainwnd = CreateWindowEx(0, CLASS_NAME, WND_TITLE, window_style, CW_USEDEFAULT, CW_USEDEFAULT, mainwidth, mainheight, nullptr, menu, hInstance, nullptr);
	DragAcceptFiles(mainwnd, TRUE);
	filterwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		0, 0, splitterX, FILTER_HEIGHT, mainwnd, (HMENU)(INT_PTR)IDC_FILTER_EDIT, hInstance, nullptr);
	HFONT guiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(filterwnd, WM_SETFONT, (WPARAM)guiFont, TRUE);
	SendMessageW(filterwnd, EM_SETCUEBANNER, TRUE, (LPARAM)L"Filter (Ctrl+F, Enter to apply)");
	g_origFilterProc = (WNDPROC)SetWindowLongPtrW(filterwnd, GWLP_WNDPROC, (LONG_PTR)FilterEditProc);
	treewnd = CreateWindow(WC_TREEVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS, 0, FILTER_HEIGHT, splitterX, mainheight - FILTER_HEIGHT, mainwnd, (HMENU)101, hInstance, nullptr);
	listwnd = CreateWindow(WC_LISTVIEW, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT, splitterX + SPLITTER_WIDTH, 0, mainwidth - splitterX - SPLITTER_WIDTH, mainheight, mainwnd, (HMENU)101, hInstance, nullptr);
	// Modern explorer theme + double-buffered drawing on tree/list (matches W3DView).
	SetWindowTheme(treewnd, L"Explorer", nullptr);
	SetWindowTheme(listwnd, L"Explorer", nullptr);
	SetWindowTheme(ListView_GetHeader(listwnd), L"Explorer", nullptr);
	TreeView_SetExtendedStyle(treewnd, TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS,
		TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS);
	ListView_SetExtendedListViewStyle(listwnd,
		LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);
	ListViewInsertColumn(listwnd, 0, L"Name", 230);
	ListViewInsertColumn(listwnd, 1, L"Type", 70);
	ListViewInsertColumn(listwnd, 2, L"Value", 0xFFFF);

	// Status bar with two parts: status text on the left, progress bar
	// embedded in the right part. Status bar resizes itself on WM_SIZE
	// (auto-handled by SBARS_SIZEGRIP); we only push our part widths.
	statuswnd = CreateWindowExW(0, STATUSCLASSNAMEW, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0, mainwnd, (HMENU)(INT_PTR)102, hInstance, nullptr);
	int parts[2] = { -1, -1 };
	// Right part 200px wide for the progress bar; left part takes the rest.
	{
		RECT cr; GetClientRect(mainwnd, &cr);
		parts[0] = cr.right - 200;
		parts[1] = -1;
	}
	SendMessage(statuswnd, SB_SETPARTS, 2, (LPARAM)parts);
	progresswnd = CreateWindowExW(0, PROGRESS_CLASSW, L"",
		WS_CHILD | PBS_MARQUEE | PBS_SMOOTH,
		0, 0, 100, 16, mainwnd, (HMENU)(INT_PTR)103, hInstance, nullptr);
	{
		SendMessage(statuswnd, WM_SIZE, 0, 0);
		RECT sr; GetWindowRect(statuswnd, &sr);
		statusHeight = sr.bottom - sr.top;
		RECT cr; GetClientRect(mainwnd, &cr);
		mainwidth = cr.right; mainheight = cr.bottom;
		ApplySplitter();
	}

	accel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));
	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(mainwnd, accel, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	if (master)
	{
		delete master;
	}
	CoUninitialize();
	return 0;
}
