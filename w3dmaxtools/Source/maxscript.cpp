#include <stdmat.h>
#include <iInstanceMgr.h>
#include <triobj.h>
#include <meshnormalspec.h>
#include <maxscript\maxscript.h>
#include <maxscript\maxwrapper\mxsobjects.h>
#include <maxscript\maxwrapper\mxsmaterial.h>
#include <maxscript\foundation\numbers.h>
#include <maxscript\foundation\colors.h>
#include <maxscript\macros\define_instantiation_functions.h>
#include "w3d.h"
#include "w3dmaterial.h"
#ifndef W3X
#include "w3dutilities.h"
#else
#include "w3xutilities.h"
#endif
using namespace W3D::MaxTools;
def_visible_primitive(wwSetType, "wwSetType");
Value *wwSetType_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetType, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Type", arg_list[1], class_tag(Integer));
	}
	INode *node = arg_list[0]->to_node();
	int type = arg_list[1]->to_int();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryType = (W3DGeometryType)type;
	}
	return &ok;
}

#ifndef W3X
def_visible_primitive(wwSetDazzle, "wwSetDazzle");
Value *wwSetDazzle_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDazzle, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_string(arg_list[1]))
	{
		throw TypeError(L"Type", arg_list[1], class_tag(String));
	}
	INode *node = arg_list[0]->to_node();
	CStr str = CStr::FromMSTR(arg_list[1]->to_string());
	W3DUtilities::SetDazzleTypeInAppData(node, str);
	return &ok;
}

def_visible_primitive(wwSetFlags, "wwSetFlags");
Value *wwSetFlags_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetFlags, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Flags", arg_list[1], class_tag(Integer));
	}
	INode *node = arg_list[0]->to_node();
	int flags = arg_list[1]->to_int();
	W3DAppDataChunk &str = W3DUtilities::GetOrCreateW3DAppDataChunk(*node);
	if (flags & W3D_MESH_FLAG_HIDDEN)
	{
		str.GeometryFlags |= W3DGeometryFlags::Hide;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::Hide;
	}
	if (flags & W3D_MESH_FLAG_TWO_SIDED)
	{
		str.GeometryFlags |= W3DGeometryFlags::TwoSided;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::TwoSided;
	}

	if (flags & W3D_MESH_FLAG_CAST_SHADOW)
	{
		str.GeometryFlags |= W3DGeometryFlags::Shadow;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::Shadow;
	}

	if (flags & W3D_MESH_FLAG_SHATTERABLE)
	{
		str.GeometryFlags |= W3DGeometryFlags::Shatter;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::Shatter;
	}

	if (flags & W3D_MESH_FLAG_NPATCHABLE)
	{
		str.GeometryFlags |= W3DGeometryFlags::Tangents;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::Tangents;
	}

	if (flags & W3D_MESH_FLAG_PRELIT)
	{
		str.GeometryFlags |= W3DGeometryFlags::Prelit;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::Prelit;
	}

	if (flags & W3D_MESH_FLAG_ALWAYSDYNLIGHT)
	{
		str.GeometryFlags |= W3DGeometryFlags::AlwaysDynLight;
	}
	else
	{
		str.GeometryFlags &= ~W3DGeometryFlags::AlwaysDynLight;
	}
	
	int gt = flags & W3D_MESH_FLAG_GEOMETRY_TYPE_MASK;
	switch (gt)
	{
	case W3D_MESH_FLAG_GEOMETRY_TYPE_NORMAL:
		str.GeometryType = W3DGeometryType::Normal;
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ALIGNED:
		str.GeometryType = W3DGeometryType::CamParal;
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_ORIENTED:
		str.GeometryType = W3DGeometryType::CamOrient;
		break;
	case W3D_MESH_FLAG_GEOMETRY_TYPE_CAMERA_Z_ORIENTED:
		str.GeometryType = W3DGeometryType::CamZOrient;
		break;
	}

	if (flags & W3D_BOX_ATTRIBUTE_ORIENTED)
	{
		str.GeometryType = W3DGeometryType::OBBox;
		str.GeometryFlags |= W3DGeometryFlags::Hide;
	}
	if (flags & W3D_BOX_ATTRIBUTE_ALIGNED)
	{
		str.GeometryType = W3DGeometryType::AABox;
	}
	int ct = flags & W3D_MESH_FLAG_COLLISION_TYPE_MASK;
	if (ct & W3D_MESH_FLAG_COLLISION_TYPE_PHYSICAL)
	{
		str.CollisionFlags |= W3DCollisionFlags::Physical;
	}
	else
	{
		str.CollisionFlags &= ~W3DCollisionFlags::Physical;
	}

	if (ct & W3D_MESH_FLAG_COLLISION_TYPE_PROJECTILE)
	{
		str.CollisionFlags |= W3DCollisionFlags::Projectile;
	}
	else
	{
		str.CollisionFlags &= ~W3DCollisionFlags::Projectile;
	}

	if (ct & W3D_MESH_FLAG_COLLISION_TYPE_VIS)
	{
		str.CollisionFlags |= W3DCollisionFlags::Vis;
	}
	else
	{
		str.CollisionFlags &= ~W3DCollisionFlags::Vis;
	}

	if (ct & W3D_MESH_FLAG_COLLISION_TYPE_CAMERA)
	{
		str.CollisionFlags |= W3DCollisionFlags::Camera;
	}
	else
	{
		str.CollisionFlags &= ~W3DCollisionFlags::Camera;
	}

	if (ct & W3D_MESH_FLAG_COLLISION_TYPE_VEHICLE)
	{
		str.CollisionFlags |= W3DCollisionFlags::Vehicle;
	}
	else
	{
		str.CollisionFlags &= ~W3DCollisionFlags::Vehicle;
	}
	return &ok;
}
#endif

def_visible_primitive(wwSetSorting, "wwSetSorting");
Value *wwSetSorting_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSorting, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Sorting", arg_list[1], class_tag(Integer));
	}
	INode *node = arg_list[0]->to_node();
	int type = arg_list[1]->to_int();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).StaticSortLevel = type;
	}
	return &ok;
}

def_visible_primitive(wwSetExportGeoFlag, "wwSetExportGeoFlag");
Value *wwSetExportGeoFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetExportGeoFlag, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"export bone value", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).ExportFlags |= W3DExportFlags::ExportGeometry;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).ExportFlags &= ~W3DExportFlags::ExportGeometry;
		}
	}
	return &ok;
}

def_visible_primitive(wwSetExportBoneFlag, "wwSetExportBoneFlag");
Value *wwSetExportBoneFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetExportBoneFlag, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"export bone value", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).ExportFlags |= W3DExportFlags::ExportTransform;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).ExportFlags &= ~W3DExportFlags::ExportTransform;
		}
	}
	return &ok;
}

def_visible_primitive(wwSetHide, "wwSetHide");
Value *wwSetHide_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetHide, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::Hide;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::Hide;
		}
	}
	return &ok;
}

def_visible_primitive(wwSet2Side, "wwSet2Side");
Value *wwSet2Side_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSet2Side, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::TwoSided;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::TwoSided;
		}
	}
	return &ok;
}

def_visible_primitive(wwSetShadow, "wwSetShadow");
Value *wwSetShadow_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetShadow, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::Shadow;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::Shadow;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetVAlpha, "wwGetVAlpha");
Value* wwGetVAlpha_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetVAlpha, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::VAlpha) == W3DGeometryFlags::VAlpha)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetVAlpha, "wwSetVAlpha");
Value *wwSetVAlpha_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetVAlpha, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::VAlpha;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::VAlpha;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetZNormal, "wwGetZNormal");
Value* wwGetZNormal_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetZNormal, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::ZNormal) == W3DGeometryFlags::ZNormal)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetZNormal, "wwSetZNormal");
Value *wwSetZNormal_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetZNormal, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::ZNormal;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::ZNormal;
		}
	}
	return &ok;
}

#ifndef W3X
def_visible_primitive(wwGetShatter, "wwGetShatter");
Value* wwGetShatter_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetShatter, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::Shatter) == W3DGeometryFlags::Shatter)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetShatter, "wwSetShatter");
Value *wwSetShatter_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetShatter, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::Shatter;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::Shatter;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetNPatch, "wwGetNPatch");
Value* wwGetNPatch_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetNPatch, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::Tangents) == W3DGeometryFlags::Tangents)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetNPatch, "wwSetNPatch");
Value *wwSetNPatch_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetNPatch, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::Tangents;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::Tangents;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetPrelit, "wwGetPrelit");
Value* wwGetPrelit_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetPrelit, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::Prelit) == W3DGeometryFlags::Prelit)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetPrelit, "wwSetPrelit");
Value *wwSetPrelit_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetPrelit, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::Prelit;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::Prelit;
		}
	}
	return &ok;
}
#else
def_visible_primitive(wwGetJoypadPick, "wwGetJoypadPick");
Value* wwGetJoypadPick_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetJoypadPick, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::JoypadPick) == W3DGeometryFlags::JoypadPick)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetJoypadPick, "wwSetJoypadPick");
Value* wwSetJoypadPick_cf(Value** arg_list, int count)
{
	check_arg_count(wwSetJoypadPick, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode* node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::JoypadPick;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::JoypadPick;
		}
	}
	return &ok;
}
#endif

def_visible_primitive(wwGetKeepNormal, "wwGetKeepNormal");
Value* wwGetKeepNormal_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetKeepNormal, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags & W3DGeometryFlags::KeepNml) == W3DGeometryFlags::KeepNml)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetKeepNormal, "wwSetKeepNormal");
Value *wwSetKeepNormal_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetKeepNormal, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags |= W3DGeometryFlags::KeepNml;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).GeometryFlags &= ~W3DGeometryFlags::KeepNml;
		}
	}
	return &ok;
}

#ifndef W3X
def_visible_primitive(wwGetCollidePhysical, "wwGetCollidePhysical");
Value* wwGetCollidePhysical_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetCollidePhysical, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags & W3DCollisionFlags::Physical) == W3DCollisionFlags::Physical)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetCollidePhysical, "wwSetCollidePhysical");
Value *wwSetCollidePhysical_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetCollidePhysical, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags |= W3DCollisionFlags::Physical;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags &= ~W3DCollisionFlags::Physical;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetCollideProjectile, "wwGetCollideProjectile");
Value* wwGetCollideProjectile_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetCollideProjectile, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags & W3DCollisionFlags::Projectile) == W3DCollisionFlags::Projectile)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetCollideProjectile, "wwSetCollideProjectile");
Value *wwSetCollideProjectile_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetCollideProjectile, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags |= W3DCollisionFlags::Projectile;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags &= ~W3DCollisionFlags::Projectile;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetCollideVis, "wwGetCollideVis");
Value* wwGetCollideVis_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetCollideVis, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags & W3DCollisionFlags::Vis) == W3DCollisionFlags::Vis)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetCollideVis, "wwSetCollideVis");
Value *wwSetCollideVis_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetCollideVis, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags |= W3DCollisionFlags::Vis;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags &= ~W3DCollisionFlags::Vis;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetCollideCamera, "wwGetCollideCamera");
Value* wwGetCollideCamera_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetCollideCamera, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags & W3DCollisionFlags::Camera) == W3DCollisionFlags::Camera)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetCollideCamera, "wwSetCollideCamera");
Value *wwSetCollideCamera_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetCollideCamera, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags |= W3DCollisionFlags::Camera;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags &= ~W3DCollisionFlags::Camera;
		}
	}
	return &ok;
}

def_visible_primitive(wwGetCollideVehicle, "wwGetCollideVehicle");
Value* wwGetCollideVehicle_cf(Value** arg_list, int count)
{
	check_arg_count(wwGetCollideVehicle, 1, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	INode* node = arg_list[0]->to_node();
	INodeTab nodes;
	IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
	for (int i = 0; i < nodes.Count(); ++i)
	{
		if ((W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags & W3DCollisionFlags::Vehicle) == W3DCollisionFlags::Vehicle)
		{
			return &true_value;
		}
	}
	return &false_value;
}

def_visible_primitive(wwSetCollideVehicle, "wwSetCollideVehicle");
Value *wwSetCollideVehicle_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetCollideVehicle, 2, count);
	if (!is_node(arg_list[0]))
	{
		throw TypeError(L"Max INode", arg_list[0], class_tag(MAXNode));
	}
	if (!is_bool(arg_list[1]))
	{
		throw TypeError(L"Boolean", arg_list[1], class_tag(Boolean));
	}
	INode *node = arg_list[0]->to_node();
	BOOL hide = arg_list[1]->to_bool();
	if (hide)
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags |= W3DCollisionFlags::Vehicle;
		}
	}
	else
	{
		INodeTab nodes;
		IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
		for (int i = 0; i < nodes.Count(); ++i)
		{
			W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]).CollisionFlags &= ~W3DCollisionFlags::Vehicle;
		}
	}
	return &ok;
}
#endif

#define is_material(mat) mat->is_kind_of(class_tag(MAXMaterial))
def_visible_primitive(wwSetSortLevel, "wwSetSortLevel");
Value *wwSetSortLevel_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSortLevel, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Value", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int value = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		((IParamBlock2 *)gm->GetReference(enum_to_value(W3DMaterialRefID::SurfaceTypeBlock)))->SetValue(enum_to_value(W3DMaterialParamID::StaticSortLevel), 0, value);
	}
	return &ok;
}

def_visible_primitive(wwGetSortLevel, "wwGetSortLevel");
Value *wwGetSortLevel_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetSortLevel, 1, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		return Integer::intern(((IParamBlock2 *)gm->GetReference(enum_to_value(W3DMaterialRefID::SurfaceTypeBlock)))->GetInt(enum_to_value(W3DMaterialParamID::StaticSortLevel), 0));
	}
	return &undefined;
}

def_visible_primitive(wwSetSurfaceType, "wwSetSurfaceType");
Value *wwSetSurfaceType_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSurfaceType, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Value", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int value = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		((IParamBlock2 *)gm->GetReference(enum_to_value(W3DMaterialRefID::SurfaceTypeBlock)))->SetValue(enum_to_value(W3DMaterialParamID::SurfaceType), 0, value);

	}
	return &ok;
}

def_visible_primitive(wwGetSurfaceType, "wwGetSurfaceType");
Value *wwGetSurfaceType_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetSurfaceType, 1, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		return Integer::intern(((IParamBlock2 *)gm->GetReference(enum_to_value(W3DMaterialRefID::SurfaceTypeBlock)))->GetInt(enum_to_value(W3DMaterialParamID::SurfaceType), 0));
	}
	return &undefined;
}

def_visible_primitive(wwSetPassCount, "wwSetPassCount");
Value *wwSetPassCount_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetPassCount, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Value", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int value = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		((IParamBlock2 *)gm->GetReference(enum_to_value(W3DMaterialRefID::PassCountBlock)))->SetValue(enum_to_value(W3DMaterialParamID::PassCount), 0, value);
		gm->RefreshPasses();
	}
	return &ok;
}

def_visible_primitive(wwGetPassCount, "wwGetPassCount");
Value *wwGetPassCount_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetPassCount, 1, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		return Integer::intern(((IParamBlock2 *)gm->GetReference(enum_to_value(W3DMaterialRefID::PassCountBlock)))->GetInt(enum_to_value(W3DMaterialParamID::PassCount), 0));
	}
	return &undefined;
}

def_visible_primitive(wwSetWriteZBuffer, "wwSetWriteZBuffer");
Value *wwSetWriteZBuffer_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetWriteZBuffer, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_bool(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	BOOL value = arg_list[2]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::BlendWriteZBuffer), 0, value);
	}
	return &ok;
}

def_visible_primitive(wwGetWriteZBuffer, "wwGetWriteZBuffer");
Value *wwGetWriteZBuffer_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetWriteZBuffer, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		bool b = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::BlendWriteZBuffer), 0);
		if (b)
		{
			return &true_value;
		}
		else
		{
			return &false_value;
		}
	}
	return &undefined;
}

def_visible_primitive(wwSetDestBlend, "wwSetDestBlend");
Value *wwSetDestBlend_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDestBlend, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::CustomDestMode), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetSrcBlend, "wwSetSrcBlend");
Value *wwSetSrcBlend_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSrcBlend, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::CustomSrcMode), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetAlphaTestFlag, "wwSetAlphaTestFlag");
Value *wwSetAlphaTestFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetAlphaTestFlag, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"pass number", arg_list[1], class_tag(Integer));
	}
	if (!is_bool(arg_list[2]))
	{
		throw TypeError(L"display flag value", arg_list[2], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	BOOL value = arg_list[2]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::AlphaTest), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetPriGradient, "wwSetPriGradient");
Value *wwSetPriGradient_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetPriGradient, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::PriGradient), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetDepthCompare, "wwSetDepthCompare");
Value *wwSetDepthCompare_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDepthCompare, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::DepthCmp), 0, value);
	}
	return &ok;
}

def_visible_primitive(wwSetDetailColor, "wwSetDetailColor");
Value *wwSetDetailColor_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDetailColor, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::DetailColour), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetDetailAlpha, "wwSetDetailAlpha");
Value *wwSetDetailAlpha_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDetailAlpha, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::DetailAlpha), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetSecGradient, "wwSetSecGradient");
Value *wwSetSecGradient_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSecGradient, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::SecGradient), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetOpacity, "wwSetOpacity");
Value *wwSetOpacity_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetOpacity, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Float));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	float value = arg_list[2]->to_float();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::Opacity), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetShininess, "wwSetShininess");
Value *wwSetShininess_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetShininess, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Float));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	float value = arg_list[2]->to_float();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::Shininess), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetTranslucency, "wwSetTranslucency");
Value *wwSetTranslucency_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetTranslucency, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Float));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	float value = arg_list[2]->to_float();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::Translucency), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetUV, "wwSetUV");
Value *wwSetUV_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetUV, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	int value = arg_list[3]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1MappingUVChannel : W3DMaterialParamID::Stage0MappingUVChannel), 0, value);
		auto texmap = gm->GetMaterialPass(pass).ParamBlock->GetTexmap(enum_to_value(stage ? W3DMaterialParamID::Stage1TextureMap : W3DMaterialParamID::Stage0TextureMap));
		if (texmap)
		{
			auto uvgen = texmap->GetTheUVGen();
			if (uvgen)
			{
				uvgen->SetMapChannel(value);
			}
		}
		gm->InvalidateDisplayTexture();
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwGetUV, "wwGetUV");
Value *wwGetUV_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetUV, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int uv = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1MappingUVChannel : W3DMaterialParamID::Stage0MappingUVChannel), 0);
		return Integer::intern(uv);
	}
	return &undefined;
}

def_visible_primitive(wwSetAmbient, "wwSetAmbient");
Value *wwSetAmbient_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetAmbient, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_color(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	COLORREF value = arg_list[2]->to_colorref();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::AmbientColour), 0, (Color)value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetDiffuse, "wwSetDiffuse");
Value *wwSetDiffuse_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDiffuse, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_color(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	COLORREF value = arg_list[2]->to_colorref();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::DiffuseColour), 0, (Color)value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetSpecular, "wwSetSpecular");
Value *wwSetSpecular_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSpecular, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_color(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	COLORREF value = arg_list[2]->to_colorref();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::SpecularColour), 0, (Color)value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetEmissive, "wwSetEmissive");
Value *wwSetEmissive_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetEmissive, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_color(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	COLORREF value = arg_list[2]->to_colorref();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::EmissiveColour), 0, (Color)value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetSpecularToDiffuse, "wwSetSpecularToDiffuse");
Value *wwSetSpecularToDiffuse_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetSpecularToDiffuse, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_bool(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	BOOL value = arg_list[2]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::SpecularToDiffuse), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetMapType, "wwSetMapType");
Value *wwSetMapType_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetMapType, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	int value = arg_list[3]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Mapping : W3DMaterialParamID::Stage0Mapping), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetFrames, "wwSetFrames");
Value *wwSetFrames_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetFrames, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	int value = arg_list[3]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Frames : W3DMaterialParamID::Stage0Frames), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetFrameRate, "wwSetFrameRate");
Value *wwSetFrameRate_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetFrameRate, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Float));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	float value = arg_list[3]->to_float();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1FPS : W3DMaterialParamID::Stage0FPS), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetPublish, "wwSetPublish");
Value *wwSetPublish_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetPublish, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Publish : W3DMaterialParamID::Stage0Publish), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetResize, "wwSetResize");
Value* wwSetResize_cf(Value** arg_list, int count)
{
	check_arg_count(wwSetResize, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Boolean));
	}
	Mtl* mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial* gm = (W3DMaterial*)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Resize : W3DMaterialParamID::Stage0Resize), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetClampU, "wwSetClampU");
Value *wwSetClampU_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetClampU, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1ClampU : W3DMaterialParamID::Stage0ClampU), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetClampV, "wwSetClampV");
Value *wwSetClampV_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetClampV, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1ClampV : W3DMaterialParamID::Stage0ClampV), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetNoLod, "wwSetNoLod");
Value *wwSetNoLod_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetNoLod, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1NoLOD : W3DMaterialParamID::Stage0NoLOD), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetAlphaBitmap, "wwSetAlphaBitmap");
Value *wwSetAlphaBitmap_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetAlphaBitmap, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1AlphaBitmap : W3DMaterialParamID::Stage0AlphaBitmap), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetAnimType, "wwSetAnimType");
Value *wwSetAnimType_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetAnimType, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	int value = arg_list[3]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1AnimMode : W3DMaterialParamID::Stage0AnimMode), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetPassHint, "wwSetPassHint");
Value *wwSetPassHint_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetPassHint, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	int value = arg_list[3]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1PassHint : W3DMaterialParamID::Stage0PassHint), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwSetStageFlag, "wwSetStageFlag");
Value *wwSetStageFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetStageFlag, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"pass number", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"stage number", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"stage flag value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1TextureEnabled : W3DMaterialParamID::Stage0TextureEnabled), 0, value);
		if (!value)
		{
			gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Display : W3DMaterialParamID::Stage0Display), 0, false);
			gm->InvalidateDisplayTexture();
			gm->MaterialDirty();
		}
	}
	return &ok;
}

def_visible_primitive(wwSetDisplayFlag, "wwSetDisplayFlag");
Value *wwSetDisplayFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetDisplayFlag, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"pass number", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"stage number", arg_list[2], class_tag(Integer));
	}
	if (!is_bool(arg_list[3]))
	{
		throw TypeError(L"display flag value", arg_list[3], class_tag(Boolean));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	BOOL value = arg_list[3]->to_bool();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		if (value)
		{
			gm->ClearDisplayFlags();
		}
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Display : W3DMaterialParamID::Stage0Display), 0, value);
		gm->InvalidateDisplayTexture();
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwGetStageFlag, "wwGetStageFlag");
Value *wwGetStageFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetStageFlag, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		bool b = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1TextureEnabled : W3DMaterialParamID::Stage0TextureEnabled), 0);
		if (b)
		{
			return &true_value;
		}
		else
		{
			return &false_value;
		}
	}
	return &undefined;
}

def_visible_primitive(wwSetMapperArgs, "wwSetMapperArgs");
Value *wwSetMapperArgs_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetMapperArgs, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_string(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(String));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	const wchar_t *value = arg_list[3]->to_string();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1MappingArgs : W3DMaterialParamID::Stage0MappingArgs), 0, value);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwGetMapperArgs, "wwGetMapperArgs");
Value *wwGetMapperArgs_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetMapperArgs, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		const wchar_t *str = gm->GetMaterialPass(pass).ParamBlock->GetStr(enum_to_value(stage ? W3DMaterialParamID::Stage1MappingArgs : W3DMaterialParamID::Stage0MappingArgs), 0);
		if (str && str[0])
		{
			return new String(str);
		}
	}
	return &undefined;
}

def_visible_primitive(wwSetTexture, "wwSetTexture");
Value *wwSetTexture_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetTexture, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_string(arg_list[1]))
	{
		throw TypeError(L"texture path and texture name", arg_list[1], class_tag(String));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"index number", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	const wchar_t *value = arg_list[1]->to_string();
	int index = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		BitmapTex* tm = static_cast<BitmapTex*>(gm->GetSubTexmap(index));
		if (!tm)
		{
			tm = NewDefaultBitmapTex();
		}
		tm->ActivateTexDisplay(TRUE);
		tm->SetMapName(value);
		gm->SetSubTexmap(index, tm);
		gm->InvalidateDisplayTexture();
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwGetTexture, "wwGetTexture");
Value *wwGetTexture_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetTexture, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"index number", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int index = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		BitmapTex* tm = static_cast<BitmapTex*>(gm->GetSubTexmap(index));
		if (tm)
		{
			return Name::intern(tm->GetMapName());
		}
	}
	return &undefined;
}

def_visible_primitive(wwGetAmbient, "wwGetAmbient");
Value *wwGetAmbient_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetAmbient, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		Color c = gm->GetMaterialPass(pass).ParamBlock->GetColor(enum_to_value(W3DMaterialParamID::AmbientColour), 0);
		return new ColorValue(c);
	}
	return &undefined;
}

def_visible_primitive(wwGetDiffuse, "wwGetDiffuse");
Value *wwGetDiffuse_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetDiffuse, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		Color c = gm->GetMaterialPass(pass).ParamBlock->GetColor(enum_to_value(W3DMaterialParamID::DiffuseColour), 0);
		return new ColorValue(c);
	}
	return &undefined;
}

def_visible_primitive(wwGetSpecular, "wwGetSpecular");
Value *wwGetSpecular_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetSpecular, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		Color c = gm->GetMaterialPass(pass).ParamBlock->GetColor(enum_to_value(W3DMaterialParamID::SpecularColour), 0);
		return new ColorValue(c);
	}
	return &undefined;
}

def_visible_primitive(wwGetEmissive, "wwGetEmissive");
Value *wwGetEmissive_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetEmissive, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		Color c = gm->GetMaterialPass(pass).ParamBlock->GetColor(enum_to_value(W3DMaterialParamID::EmissiveColour), 0);
		return new ColorValue(c);
	}
	return &undefined;
}

def_visible_primitive(wwGetShininess, "wwGetShininess");
Value *wwGetShininess_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetShininess, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		float f = gm->GetMaterialPass(pass).ParamBlock->GetFloat(enum_to_value(W3DMaterialParamID::Shininess), 0);
		return Float::intern(f);
	}
	return &undefined;
}

def_visible_primitive(wwGetOpacity, "wwGetOpacity");
Value *wwGetOpacity_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetOpacity, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		float f = gm->GetMaterialPass(pass).ParamBlock->GetFloat(enum_to_value(W3DMaterialParamID::Opacity), 0);
		return Float::intern(f);
	}
	return &undefined;
}

def_visible_primitive(wwGetTranslucency, "wwGetTranslucency");
Value *wwGetTranslucency_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetTranslucency, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		float f = gm->GetMaterialPass(pass).ParamBlock->GetFloat(enum_to_value(W3DMaterialParamID::Translucency), 0);
		return Float::intern(f);
	}
	return &undefined;
}

def_visible_primitive(wwGetDepthCompare, "wwGetDepthCompare");
Value *wwGetDepthCompare_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetDepthCompare, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::DepthCmp), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetDestBlend, "wwGetDestBlend");
Value *wwGetDestBlend_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetDestBlend, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::CustomDestMode), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetPriGradient, "wwGetPriGradient");
Value *wwGetPriGradient_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetPriGradient, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::PriGradient), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetSecGradient, "wwGetSecGradient");
Value *wwGetSecGradient_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetSecGradient, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::SecGradient), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetSrcBlend, "wwGetSrcBlend");
Value *wwGetSrcBlend_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetSrcBlend, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::CustomSrcMode), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetAlphaTestFlag, "wwGetAlphaTestFlag");
Value *wwGetAlphaTestFlag_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetAlphaTestFlag, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"pass number", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::AlphaTest), 0);
		if (i)
		{
			return &true_value;
		}
		return &false_value;
	}
	return &undefined;
}

def_visible_primitive(wwGetDetailColor, "wwGetDetailColor");
Value *wwGetDetailColor_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetDetailColor, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::DetailColour), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetDetailAlpha, "wwGetDetailAlpha");
Value *wwGetDetailAlpha_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetDetailAlpha, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::DetailAlpha), 0);
		return Integer::intern(i);
	}
	return &undefined;
}
def_visible_primitive(wwSetMatFlags, "wwSetMatFlags");
Value *wwSetMatFlags_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetMatFlags, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Value", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int value = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int m1 = (value & W3DVERTMAT_STAGE0_MAPPING_MASK) >> 0x10;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::Stage0Mapping), 0, m1);
		int m2 = (value & W3DVERTMAT_STAGE1_MAPPING_MASK) >> 8;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::Stage1Mapping), 0, m2);
		int f = value & W3DVERTMAT_COPY_SPECULAR_TO_DIFFUSE;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(W3DMaterialParamID::SpecularToDiffuse), 0, f == 1);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwGetMatFlags, "wwGetMatFlags");
Value *wwGetMatFlags_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetMatFlags, 2, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int m1 = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::Stage0Mapping), 0);
		int m2 = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::Stage1Mapping), 0);
		int f = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(W3DMaterialParamID::SpecularToDiffuse), 0);
		int flags = m1 | m2 | f;
		return Integer::intern(flags);
	}
	return &undefined;
}
def_visible_primitive(wwSetTexFlags, "wwSetTexFlags");
Value *wwSetTexFlags_cf(Value ** arg_list, int count)
{
	check_arg_count(wwSetTexFlags, 4, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	if (!is_number(arg_list[3]))
	{
		throw TypeError(L"Value", arg_list[3], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	int value = arg_list[3]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int p = (value & W3DTEXTURE_PUBLISH);
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Publish : W3DMaterialParamID::Stage0Publish), 0, p == 1);
		int r = (value & W3DTEXTURE_RESIZE_OBSOLETE);
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1Resize : W3DMaterialParamID::Stage0Resize), 0, r == 1);
		int n = (value & W3DTEXTURE_NO_LOD);
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1NoLOD : W3DMaterialParamID::Stage0NoLOD), 0, n == 1);
		int u = (value & W3DTEXTURE_CLAMP_U);
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1ClampU : W3DMaterialParamID::Stage0ClampU), 0, u == 1);
		int v = (value & W3DTEXTURE_CLAMP_V);
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1ClampV : W3DMaterialParamID::Stage0ClampV), 0, v == 1);
		int a = (value & W3DTEXTURE_ALPHA_BITMAP);
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1AlphaBitmap : W3DMaterialParamID::Stage0AlphaBitmap), 0, a == 1);
		int h = (value & W3DTEXTURE_HINT_MASK & ~W3DTEXTURE_TYPE_MASK) >> W3DTEXTURE_HINT_SHIFT;
		gm->GetMaterialPass(pass).ParamBlock->SetValue(enum_to_value(stage ? W3DMaterialParamID::Stage1PassHint : W3DMaterialParamID::Stage0PassHint), 0, h);
		gm->MaterialDirty();
	}
	return &ok;
}

def_visible_primitive(wwGetTexFlags, "wwGetTexFlags");
Value *wwGetTexFlags_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetTexFlags, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int p = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1Publish : W3DMaterialParamID::Stage0Publish), 0) == 1 ? W3DTEXTURE_PUBLISH : 0;
		int r = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1Resize : W3DMaterialParamID::Stage0Resize), 0) == 1 ? W3DTEXTURE_RESIZE_OBSOLETE : 0;
		int n = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1NoLOD : W3DMaterialParamID::Stage0NoLOD), 0) == 1 ? W3DTEXTURE_NO_LOD : 0;
		int u = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1ClampU : W3DMaterialParamID::Stage0ClampU), 0) == 1 ? W3DTEXTURE_CLAMP_U : 0;
		int v = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1ClampV : W3DMaterialParamID::Stage0ClampV), 0) == 1 ? W3DTEXTURE_CLAMP_V : 0;
		int a = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1AlphaBitmap : W3DMaterialParamID::Stage0AlphaBitmap), 0) == 1 ? W3DTEXTURE_ALPHA_BITMAP : 0;
		int h = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1PassHint : W3DMaterialParamID::Stage0PassHint), 0) << W3DTEXTURE_HINT_SHIFT;
		int flags = p | n | u | v | a | h;
		return Integer::intern(flags);
	}
	return &undefined;
}

def_visible_primitive(wwGetAnimType, "wwGetAnimType");
Value *wwGetAnimType_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetAnimType, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		int i = gm->GetMaterialPass(pass).ParamBlock->GetInt(enum_to_value(stage ? W3DMaterialParamID::Stage1AnimMode : W3DMaterialParamID::Stage0AnimMode), 0);
		return Integer::intern(i);
	}
	return &undefined;
}

def_visible_primitive(wwGetFrames, "wwGetFrames");
Value *wwGetFrames_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetFrames, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		float f = gm->GetMaterialPass(pass).ParamBlock->GetFloat(enum_to_value(stage ? W3DMaterialParamID::Stage1Frames : W3DMaterialParamID::Stage0Frames), 0);
		return Float::intern(f);
	}
	return &undefined;
}

def_visible_primitive(wwGetFrameRate, "wwGetFrameRate");
Value *wwGetFrameRate_cf(Value ** arg_list, int count)
{
	check_arg_count(wwGetFrameRate, 3, count);
	if (!is_material(arg_list[0]))
	{
		throw TypeError(L"W3D Material", arg_list[0], class_tag(MAXMaterial));
	}
	if (!is_number(arg_list[1]))
	{
		throw TypeError(L"Pass", arg_list[1], class_tag(Integer));
	}
	if (!is_number(arg_list[2]))
	{
		throw TypeError(L"Stage", arg_list[2], class_tag(Integer));
	}
	Mtl *mtl = arg_list[0]->to_mtl();
	int pass = arg_list[1]->to_int();
	int stage = arg_list[2]->to_int();
	if (mtl->ClassID() == W3DMaterialClassDesc::Instance()->ClassID())
	{
		W3DMaterial *gm = (W3DMaterial *)mtl;
		float f = gm->GetMaterialPass(pass).ParamBlock->GetFloat(enum_to_value(stage ? W3DMaterialParamID::Stage1FPS : W3DMaterialParamID::Stage0FPS), 0);
		return Float::intern(f);
	}
	return &undefined;
}

// ===========================================================================
// WWSkin scripting helpers — Phase 7 of the WWSkin port. Direct port of EA's
// SkinCopy.cpp ("SceneSetup" workflow). Names and arg-counts match EA so any
// existing user MAXScripts that drove the old toolchain keep working.
//
// Published functions:
//   wwFindSkinNode  <tree_root>                          -> WWSkin WSM node | undefined
//   wwCopySkinInfo  <src> <tgt> <wsm|undefined> <root>   -> WSM node | undefined
//   wwDuplicateSkinWSM <wsm_node> <tree_root>            -> new WSM node | undefined
// ===========================================================================
#include "w3dskin.h"
#include <modstack.h>

namespace
{
	// W3D names are zero-padded, ASCII-uppercase, max 16 chars (W3D_NAME_LEN).
	// Mirrors EA's Set_W3D_Name() — used to compare bones across hierarchies
	// regardless of trailing extensions like ".001" or case differences.
	static void Make_W3D_Name(char* out, const TCHAR* in)
	{
		out[0] = '\0';
		if (!in) return;
		// TCHAR is wchar_t in this build — narrow it then uppercase.
		char narrow[256];
		size_t i = 0;
		for (; in[i] && i < sizeof(narrow) - 1; ++i)
		{
			narrow[i] = (char)in[i];
		}
		narrow[i] = '\0';
		// Truncate at the first '.' (matches Westwood naming convention).
		for (size_t k = 0; narrow[k]; ++k)
			if (narrow[k] == '.') { narrow[k] = '\0'; break; }
		strncpy(out, narrow, W3D_NAME_LEN - 1);
		out[W3D_NAME_LEN - 1] = '\0';
		for (size_t k = 0; out[k]; ++k)
			out[k] = (char)toupper((unsigned char)out[k]);
	}

	static W3D::MaxTools::SkinWSMObjectClass* get_skin_wsm_obj(INode* wsm_node)
	{
		if (!wsm_node) return nullptr;
		Object* obj = wsm_node->GetObjectRef();
		// Burrow through any derived-object stack to reach the base.
		while (obj && obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
		{
			obj = ((IDerivedObject*)obj)->GetObjRef();
		}
		if (!obj) return nullptr;
		if (obj->ClassID() != Class_ID(0x32B37E0C, 0x5A9612E4)) return nullptr;
		return static_cast<W3D::MaxTools::SkinWSMObjectClass*>(obj);
	}

	static W3D::MaxTools::SkinModifierClass* find_skin_binding(INode* skinned_obj)
	{
		if (!skinned_obj) return nullptr;
		IDerivedObject* dobj = skinned_obj->GetWSMDerivedObject();
		if (!dobj) return nullptr;
		const Class_ID modCID(0x6BAD4898, 0x0D1D6CED);
		for (int i = 0; i < dobj->NumModifiers(); ++i)
		{
			Modifier* mod = dobj->GetModifier(i);
			if (mod && mod->ClassID() == modCID)
				return static_cast<W3D::MaxTools::SkinModifierClass*>(mod);
		}
		return nullptr;
	}

	static INode* find_skin_wsm(INode* skinned_obj)
	{
		auto* sm = find_skin_binding(skinned_obj);
		if (!sm) return nullptr;
		return static_cast<INode*>(sm->GetReference(W3D::MaxTools::SkinModifierClass::NODE_REF));
	}

	static INode* find_equivalent_node(INode* source, INode* tree, bool name_is_valid = false)
	{
		if (!source || !tree) return nullptr;
		static char src_name[W3D_NAME_LEN];
		if (!name_is_valid) Make_W3D_Name(src_name, source->GetName());

		char chk_name[W3D_NAME_LEN];
		Make_W3D_Name(chk_name, tree->GetName());
		if (strcmp(src_name, chk_name) == 0) return tree;

		for (int i = 0; i < tree->NumberOfChildren(); ++i)
		{
			INode* hit = find_equivalent_node(source, tree->GetChildNode(i), true);
			if (hit) return hit;
		}
		return nullptr;
	}

	static Value* find_skin_node_in_tree(INode* root)
	{
		if (!root) return &undefined;
		if (get_skin_wsm_obj(root))
		{
			one_typed_value_local(Value* wsm_node);
			vl.wsm_node = MAXNode::intern(root);
			return_value(vl.wsm_node);
		}
		for (int i = 0; i < root->NumChildren(); ++i)
		{
			Value* r = find_skin_node_in_tree(root->GetChildNode(i));
			if (r != &undefined) return r;
		}
		return &undefined;
	}

	static INode* duplicate_wsm(INode* wsm_node, INode* tree)
	{
		auto* wsm_obj = get_skin_wsm_obj(wsm_node);
		if (!wsm_node || !wsm_obj) return nullptr;

		auto* new_wsm_obj = static_cast<W3D::MaxTools::SkinWSMObjectClass*>(
			CreateInstance(WSM_OBJECT_CLASS_ID, Class_ID(0x32B37E0C, 0x5A9612E4)));
		if (!new_wsm_obj) return nullptr;

		INode* new_wsm_node = GetCOREInterface()->CreateObjectNode(new_wsm_obj);
		if (!new_wsm_node) return nullptr;

		// Re-bind every bone slot to the equivalently-named node in the target tree.
		for (int i = 0; i < wsm_obj->Num_Bones(); ++i)
		{
			INode* src_bone = wsm_obj->Get_Bone(i);
			INode* dst_bone = find_equivalent_node(src_bone, tree);
			if (!src_bone || !dst_bone) return nullptr;
			new_wsm_obj->Add_Bone(dst_bone);
		}
		return new_wsm_node;
	}

	static IDerivedObject* setup_wsm_derived_obj(INode* node)
	{
		IDerivedObject* dobj = node->GetWSMDerivedObject();
		const Class_ID modCID(0x6BAD4898, 0x0D1D6CED);
		if (dobj)
		{
			// Strip any pre-existing WWSkin Binding so we don't end up doubly-bound.
			for (int i = 0; i < dobj->NumModifiers(); ++i)
			{
				Modifier* mod = dobj->GetModifier(i);
				if (mod && mod->ClassID() == modCID) { dobj->DeleteModifier(i); break; }
			}
		}
		else
		{
			dobj = CreateWSDerivedObject(node->GetObjectRef());
			if (!dobj) throw RuntimeError(_M("Error setting up the WSMDerivedObject"));
			node->SetObjectRef(dobj);
		}
		return dobj;
	}

	static ModContext* find_skin_mod_context(INode* node)
	{
		if (!node) return nullptr;
		IDerivedObject* dobj = node->GetWSMDerivedObject();
		if (!dobj) return nullptr;
		const Class_ID modCID(0x6BAD4898, 0x0D1D6CED);
		for (int i = 0; i < dobj->NumModifiers(); ++i)
		{
			Modifier* mod = dobj->GetModifier(i);
			if (mod && mod->ClassID() == modCID) return dobj->GetModContext(i);
		}
		return nullptr;
	}

	static Value* copy_skin_info(INode* source, INode* target, INode* wsm)
	{
		auto* source_modifier = find_skin_binding(source);
		if (!source_modifier) return &undefined;

		IDerivedObject* dobj = setup_wsm_derived_obj(target);

		auto* wsm_obj = get_skin_wsm_obj(wsm);
		auto* new_modifier = new W3D::MaxTools::SkinModifierClass(wsm, wsm_obj);
		new_modifier->SubObjSelLevel = source_modifier->SubObjSelLevel;

		// Carry the source's local mod data (vertex-influence table) over so the
		// target inherits the same per-vertex bone assignments and weights.
		ModContext* source_context = find_skin_mod_context(source);
		ModContext* new_context = source_context
			? new ModContext(source_context->tm, source_context->box, source_context->localData)
			: new ModContext();

		dobj->AddModifier(new_modifier, new_context);

		one_typed_value_local(Value* wsm_node);
		vl.wsm_node = MAXNode::intern(wsm);
		return_value(vl.wsm_node);
	}
}

def_visible_primitive(wwFindSkinNode,    "wwFindSkinNode");
def_visible_primitive(wwCopySkinInfo,    "wwCopySkinInfo");
def_visible_primitive(wwDuplicateSkinWSM,"wwDuplicateSkinWSM");
def_visible_primitive(wwAddBone,         "wwAddBone");
def_visible_primitive(wwSetBoneWeight,   "wwSetBoneWeight");

Value* wwFindSkinNode_cf(Value** arg_list, int count)
{
	check_arg_count(wwFindSkinNode, 1, count);
	if (!is_node(arg_list[0])) throw TypeError(L"Tree Root INode", arg_list[0], class_tag(MAXNode));
	return find_skin_node_in_tree(arg_list[0]->to_node());
}

Value* wwCopySkinInfo_cf(Value** arg_list, int count)
{
	check_arg_count(wwCopySkinInfo, 4, count);
	if (!is_node(arg_list[0])) throw TypeError(L"Source INode",      arg_list[0], class_tag(MAXNode));
	if (!is_node(arg_list[1])) throw TypeError(L"Target INode",      arg_list[1], class_tag(MAXNode));
	if (!is_node(arg_list[3])) throw TypeError(L"Tree Root INode",   arg_list[3], class_tag(MAXNode));

	INode* src       = arg_list[0]->to_node();
	INode* dst       = arg_list[1]->to_node();
	INode* tree_root = arg_list[3]->to_node();
	INode* wsm_node  = nullptr;

	if (arg_list[2] == &undefined)
	{
		wsm_node = duplicate_wsm(find_skin_wsm(src), tree_root);
		if (!wsm_node) return &undefined;
	}
	else
	{
		if (!is_node(arg_list[2])) throw TypeError(L"WSM INode or undefined", arg_list[2], class_tag(MAXNode));
		wsm_node = arg_list[2]->to_node();
	}
	return copy_skin_info(src, dst, wsm_node);
}

Value* wwDuplicateSkinWSM_cf(Value** arg_list, int count)
{
	check_arg_count(wwDuplicateSkinWSM, 2, count);
	if (!is_node(arg_list[0])) throw TypeError(L"WWSkin Object INode", arg_list[0], class_tag(MAXNode));
	if (!is_node(arg_list[1])) throw TypeError(L"Target Tree Root INode", arg_list[1], class_tag(MAXNode));

	INode* dupe = duplicate_wsm(arg_list[0]->to_node(), arg_list[1]->to_node());
	if (!dupe) return &undefined;
	one_typed_value_local(Value* wsm_node);
	vl.wsm_node = MAXNode::intern(dupe);
	return_value(vl.wsm_node);
}

// wwAddBone <bone_node> <wsm_or_skinned_mesh_node>
//   - If the second arg is a WWSkin WSM node, adds the bone directly.
//   - If it's a skinned mesh (has a WWSkin Binding), adds the bone to the WSM
//     that the binding references. Convenience for the importer's "for each
//     pivot, add as bone" loop.
// Returns: ok | undefined
Value* wwAddBone_cf(Value** arg_list, int count)
{
	check_arg_count(wwAddBone, 2, count);
	if (!is_node(arg_list[0])) throw TypeError(L"Bone INode",   arg_list[0], class_tag(MAXNode));
	if (!is_node(arg_list[1])) throw TypeError(L"Target INode", arg_list[1], class_tag(MAXNode));

	INode* bone   = arg_list[0]->to_node();
	INode* target = arg_list[1]->to_node();

	W3D::MaxTools::SkinWSMObjectClass* wsm = get_skin_wsm_obj(target);
	if (!wsm)
	{
		// target may be a skinned mesh — go via its binding to the WSM node.
		INode* wsm_node = find_skin_wsm(target);
		wsm = get_skin_wsm_obj(wsm_node);
	}
	if (!wsm) return &undefined;

	wsm->Add_Bone(bone);
	return &ok;
}

// wwSetBoneWeight <skinned_mesh> <vert_idx_0based> <slot 1|2> <bone_node> <weight_0_to_100>
//   Sets one of the two influence slots on a single vertex. Slot 1 = primary,
//   slot 2 = secondary (matches EA's convention used by older importer scripts).
//   Weight is 0..100 (scaled to 0..1 internally to match the on-disk format).
//   The implementation walks the mesh's WWSkin Binding modifier, locates its
//   ModContext-attached SkinDataClass, resolves the bone's index in the WSM's
//   bone tab, and writes BoneIdx/BoneWeight directly.
// Returns: ok | undefined
Value* wwSetBoneWeight_cf(Value** arg_list, int count)
{
	check_arg_count(wwSetBoneWeight, 5, count);
	if (!is_node  (arg_list[0])) throw TypeError(L"Skinned Mesh INode", arg_list[0], class_tag(MAXNode));
	if (!is_number(arg_list[1])) throw TypeError(L"Vertex index",       arg_list[1], class_tag(Integer));
	if (!is_number(arg_list[2])) throw TypeError(L"Slot (1 or 2)",      arg_list[2], class_tag(Integer));
	if (!is_node  (arg_list[3])) throw TypeError(L"Bone INode",         arg_list[3], class_tag(MAXNode));
	if (!is_number(arg_list[4])) throw TypeError(L"Weight 0..100",      arg_list[4], class_tag(Float));

	INode*       mesh   = arg_list[0]->to_node();
	const int    vid    = arg_list[1]->to_int();
	const int    slot   = arg_list[2]->to_int();
	INode*       bone   = arg_list[3]->to_node();
	const float  weight = arg_list[4]->to_float() / 100.0f;

	if (slot != 1 && slot != 2) return &undefined;

	auto* skinMod = find_skin_binding(mesh);
	if (!skinMod || !skinMod->WSMObjectRef) return &undefined;

	auto* skinData = static_cast<W3D::MaxTools::SkinDataClass*>(find_skin_mod_context(mesh)
		? find_skin_mod_context(mesh)->localData : nullptr);
	if (!skinData) return &undefined;

	if (vid < 0 || vid >= skinData->VertData.Count()) return &undefined;

	int boneIdx = skinMod->WSMObjectRef->Find_Bone(bone);
	if (boneIdx < 0)
	{
		// Bone isn't on the WSM yet — add it on demand so the importer doesn't
		// have to pre-walk the pivot list.
		boneIdx = skinMod->WSMObjectRef->Add_Bone(bone);
		if (boneIdx < 0) return &undefined;
	}

	auto& inf = skinData->VertData[vid];
	inf.BoneIdx   [slot - 1] = boneIdx;
	inf.BoneWeight[slot - 1] = weight;
	return &ok;
}

// Promote every normal in a node's MeshNormalSpec to Explicit so Max 2023's
// Nitrous viewport stops re-deriving them from smoothing groups. The W3D
// importer's per-vertex setNormal loop writes the values; this helper just
// flips the Explicit bit. Replaces the importer's deferred Edit_Normals
// MakeExplicit pass — writes the base mesh directly so no modifier is left
// on the stack and no Skin/topology conflict can occur, which means it can
// run inline right after setNormal instead of after binding.
def_visible_primitive(wwMakeNormalsExplicit, "wwMakeNormalsExplicit");
Value *wwMakeNormalsExplicit_cf(Value **arg_list, int count)
{
	check_arg_count(wwMakeNormalsExplicit, 1, count);
	INode* node = arg_list[0]->to_node();
	if (!node) return &false_value;

	Object* obj = node->GetObjectRef();
	while (obj && obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		obj = ((IDerivedObject*)obj)->GetObjRef();
	}
	if (!obj || !obj->IsSubClassOf(triObjectClassID)) return &false_value;

	TriObject* tri = static_cast<TriObject*>(obj);
	Mesh& mesh = tri->GetMesh();

	mesh.SpecifyNormals();
	MeshNormalSpec* spec = mesh.GetSpecifiedNormals();
	if (!spec) return &false_value;

	spec->CheckNormals();
	const int n = spec->GetNumNormals();
	for (int i = 0; i < n; ++i)
	{
		spec->SetNormalExplicit(i, true);
	}

	node->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	return &true_value;
}
