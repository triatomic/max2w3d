#pragma once

// W3D Tools — WWSkin (Westwood skinning) Space Warp + Binding Modifier.
//
// Re-introduced for the 2023 toolchain alongside Max's native Skin modifier.
// Maintains binary-format compatibility with EA's original (Class_IDs and on-disk
// chunk IDs match Westwood's skin.cpp / skindata.cpp), so legacy .max scenes keep
// loading. UI surface is implemented as ParamBlock2-backed rollouts in Phase 6 to
// match the rest of the 2023 toolchain (DPI-aware, dark-mode friendly).
//
// The on-disk format used here is EA's:
//   InfluenceStruct  : 16 bytes — int BoneIdx[2], float BoneWeight[2]
//   SkinData chunks  : 0x0000 flags, 0x0010 vertSel, 0x0020 namedSelSets, 0x0030 influence data
//   WSM bone count   : chunk 0x0001 (NUM_BONES_CHUNK) — bones themselves are References,
//                       added after SimpleWSMObject's base reference slot.

#include <max.h>
#include <simpobj.h>
#include <objmode.h>
#include <geom/bitarray.h>
#include "bpick.h"
#include "namedsel.h"

class IUtil;

namespace W3D::MaxTools
{
	// Forward decls; full impls live in w3dskin.cpp / namedsel.cpp (Phase 5).
	class SkinWSMObjectClass;
	class SkinModifierClass;

	// Bone-influence record stored per vertex. Layout MUST stay 16-bytes / two-int +
	// two-float to remain wire-compatible with EA's saved skindata.
	struct InfluenceStruct
	{
		int   BoneIdx[2];
		float BoneWeight[2];

		InfluenceStruct()
		{
			BoneIdx[0] = -1;
			BoneIdx[1] = -1;
			BoneWeight[0] = 1.0f;
			BoneWeight[1] = 0.0f;
		}

		void Set_Influence(int boneidx) { BoneIdx[0] = boneidx; }
	};

	// Per-mesh ModContext data: the binding's vertex influence table plus per-vertex
	// selection state. Hung off ModContext::localData by SkinModifierClass.
	class SkinDataClass : public LocalModData
	{
	public:
		SkinDataClass() : Valid(FALSE), Held(FALSE) {}
		explicit SkinDataClass(Mesh* mesh);

		void Invalidate() { Valid = FALSE; }
		BOOL IsValid() const { return Valid; }
		void Validate(Mesh* mesh);

		LocalModData* Clone() override;

		void Add_Influence(int boneidx);

		IOResult Save(ISave* isave);
		IOResult Load(ILoad* iload);

		BOOL                  Valid;
		BOOL                  Held;
		BitArray              VertSel;
		NamedSelSetList       VertSelSets;
		Tab<InfluenceStruct>  VertData;

		enum {
			FLAGS_CHUNK            = 0x0000,
			VERT_SEL_CHUNK         = 0x0010,
			NAMED_SEL_SETS_CHUNK   = 0x0020,
			INFLUENCE_DATA_CHUNK   = 0x0030
		};
	};

	// Space Warp Object: the WWSkin scene icon that owns the bone list.
	class SkinWSMObjectClass : public SimpleWSMObject, public BonePickerUserClass
	{
	public:
		SkinWSMObjectClass();
		virtual ~SkinWSMObjectClass();

		// Animatable / Object
		void GetClassName(MSTR& s, bool localized) const override;
		Class_ID  ClassID() override;
		SClass_ID SuperClassID() override { return WSM_OBJECT_CLASS_ID; }
		void DeleteThis() override { delete this; }
		void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev = nullptr) override;
		void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next = nullptr) override;
		const MCHAR* GetObjectName(bool localized) const override { return _M("WWSkin"); }
		int DoOwnSelectHilite() { return TRUE; }
		CreateMouseCallBack* GetCreateMouseCallBack() override;

		// References — SimpleWSMObject reserves its own reference slot at index 0;
		// our bones occupy reference slots [SimpleWSMObject::NumRefs() .. NumRefs()-1].
		int NumRefs() override;
		RefTargetHandle GetReference(int i) override;
		void SetReference(int i, RefTargetHandle rtarg) override;
		RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		                           PartID& partID, RefMessage message, BOOL propagate) override;
		RefTargetHandle Clone(RemapDir& remap) override;

		// WSMObject
		Modifier* CreateWSMMod(INode* node) override;

		// SimpleWSMObject
		void BuildMesh(TimeValue t) override;

		// Save/Load
		IOResult Save(ISave* isave) override;
		IOResult Load(ILoad* iload) override;

		// Bone management
		int  Add_Bone(INode* node);
		void Add_Bones(INodeTab& nodetab);
		void Remove_Bone(INode* node);
		void Remove_Bones(INodeTab& nodetab);
		int  Find_Bone(INode* node);
		int  Find_Closest_Bone(const Point3& vertex);
		int  Num_Bones() { return BoneTab.Count(); }
		INode* Get_Bone(int idx) { return BoneTab[idx]; }
		INodeTab& Get_Bone_List() { return BoneTab; }
		int  Get_Base_Pose_Frame() const { return BasePoseFrame; }
		int  Get_Base_Pose_Time()  const { return BasePoseFrame * GetTicksPerFrame(); }
		void Set_Base_Pose_Frame(int frame) { BasePoseFrame = frame; }

		// BonePickerUserClass overrides.
		void User_Picked_Bone(INode* node) override;
		void User_Picked_Bones(INodeTab& nodetab) override;
		void User_Exited_Pick_Mode() override;
		void Set_Bone_Selection_Mode(int mode) { BoneSelectionMode = mode; }
		int  Get_Bone_Selection_Mode() { return BoneSelectionMode; }

		// Index translation between bone-list slots and reference indices.
		int To_Bone_Index(int refidx) { return refidx - SimpleWSMObject::NumRefs(); }
		int To_Ref_Index(int boneidx) { return SimpleWSMObject::NumRefs() + boneidx; }

		// Legacy data: kept as `nodelist` alias in case external code refers to it; just
		// returns BoneTab. EA called this BoneTab; the prior shim used `nodelist`. Both
		// names refer to the same storage now to avoid a churn of touch-points elsewhere.
		INodeTab& nodelist_alias() { return BoneTab; }

		enum {
			BONE_SEL_MODE_NONE = 0,
			BONE_SEL_MODE_ADD,
			BONE_SEL_MODE_REMOVE,
			BONE_SEL_MODE_ADD_MANY,
			BONE_SEL_MODE_REMOVE_MANY
		};

		enum { NUM_BONES_CHUNK = 0x0001 };

		// Persisted state
		INodeTab BoneTab;
		int      BasePoseFrame;

		// Runtime state
		BOOL     MeshBuilt;
		int      BoneSelectionMode;

		// Modifier-panel UI state (Phase 6). Populated by BeginEditParams,
		// torn down by EndEditParams. May be null when the rollout isn't open.
		IObjParam*        InterfacePtr;
		HWND              SkeletonHWND;
		HWND              BoneListHWND;
		ICustButton*      AddBonesButton;
		ICustButton*      RemoveBonesButton;
		ICustButton*      PickByNameButton;
		ISpinnerControl*  BasePoseSpin;

		// Persistent across dialog tear-down/recreation: the Classic mode
		// checkbox state. Stored on the WSM itself rather than read from the
		// checkbox window, since Max can recreate the rollout (e.g. during
		// REFMSG_SUBANIM_STRUCTURE_CHANGED refreshes) and the new HWND would
		// re-initialise to unchecked, losing the user's choice mid-session.
		bool              ClassicMode;
	};

	class SkinWSMObjectClassDesc : public ClassDesc2
	{
	public:
		int           IsPublic() override { return TRUE; }            // Show in Space Warps panel.
		void*         Create(BOOL loading = FALSE) override { return new SkinWSMObjectClass(); }
		const TCHAR*  ClassName() override { return _T("WWSkin"); }
		const TCHAR*  NonLocalizedClassName() override { return _T("WWSkin"); }
		// MAXScript-visible name used as the constructor symbol — `WWSkin()` in
		// Listener/scripts. Must be distinct from the Modifier's InternalName
		// because both classes ship the same ClassName, which would otherwise
		// produce an ambiguous MAXScript registration.
		const TCHAR*  InternalName() override { return _T("WWSkin"); }
		SClass_ID     SuperClassID() override { return WSM_OBJECT_CLASS_ID; }
		Class_ID      ClassID() override { return Class_ID(0x32B37E0C, 0x5A9612E4); }
		const TCHAR*  Category() override { return _T("Westwood Space Warps"); }
		static SkinWSMObjectClassDesc* Instance();

	private:
		SkinWSMObjectClassDesc() = default;
	};

	// Per-mesh "WWSkin Binding" Modifier. Stores per-vertex InfluenceStructs in its
	// ModContext::localData (SkinDataClass) and refers back to the chosen WSM Object.
	class SkinModifierClass : public Modifier, public BonePickerUserClass
	{
	public:
		SkinModifierClass();
		SkinModifierClass(INode* node, SkinWSMObjectClass* wsmobject);
		virtual ~SkinModifierClass();

		// Animatable / Modifier
		void GetClassName(MSTR& s, bool localized) const override;
		Class_ID  ClassID() override;
		SClass_ID SuperClassID() override { return WSM_CLASS_ID; }
		void DeleteThis() override { delete this; }
		void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev = nullptr) override;
		void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next = nullptr) override;
		const MCHAR* GetObjectName(bool localized) const override { return _M("WWSkin Binding"); }
		CreateMouseCallBack* GetCreateMouseCallBack() override { return nullptr; }

		// References — fixed [WSMObject, Node].
		int NumRefs() override { return 2; }
		RefTargetHandle GetReference(int i) override;
		void SetReference(int i, RefTargetHandle rtarg) override;
		RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
		                           PartID& partID, RefMessage message, BOOL propagate) override;
		RefTargetHandle Clone(RemapDir& remap) override;

		// Modifier interface
		ChannelMask ChannelsUsed() override
		    { return SELECT_CHANNEL | SUBSEL_TYPE_CHANNEL | GEOM_CHANNEL; }
		ChannelMask ChannelsChanged() override
		    { return SELECT_CHANNEL | SUBSEL_TYPE_CHANNEL | GEOM_CHANNEL; }
		void NotifyInputChanged(const Interval&, PartID, RefMessage, ModContext*) override {}
		void ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* node) override;
		BOOL DependOnTopology(ModContext&) override { return TRUE; }
		Class_ID InputType() override { return Class_ID(TRIOBJ_CLASS_ID, 0); }

		// Save/Load
		IOResult Save(ISave* isave) override;
		IOResult Load(ILoad* iload) override;
		IOResult LoadLocalData(ILoad* iload, LocalModData** pld) override;
		IOResult SaveLocalData(ISave* isave, LocalModData* ld) override;

		// Sub-object selection (vertex level)
		void ActivateSubobjSel(int level, XFormModes& modes) override;
		int  HitTest(TimeValue t, INode* inode, int type, int crossing, int flags,
		             IPoint2* p, ViewExp* vpt, ModContext* mc) override;
		void SelectSubComponent(HitRecord* hitRec, BOOL selected, BOOL all,
		                        BOOL invert = FALSE) override;
		void ClearSelection(int selLevel) override;
		void SelectAll(int selLevel) override;
		void InvertSelection(int selLevel) override;
		BOOL SupportsNamedSubSels() override { return TRUE; }
		void ActivateSubSelSet(MSTR& setName) override;
		void NewSetFromCurSel(MSTR& setName) override;
		void RemoveSubSelSet(MSTR& setName) override;
		int  NumSubObjTypes() override;
		ISubObjType* GetSubObjType(int i) override;

		// BonePickerUserClass overrides.
		void User_Picked_Bone(INode* node) override;
		void User_Picked_Bones(INodeTab& nodetab) override;
		void User_Exited_Pick_Mode() override {}

		// Auto-attach selected (or all) verts to the closest base-pose bone.
		void Auto_Attach_Verts(BOOL all = FALSE);
		// Unlink selected verts (set BoneIdx[0] back to -1).
		void Unlink_Verts();

		// Returns the bound WSM Object via the OBJ_REF reference slot. Member is
		// named WSMObjectRef (not WSMObject) to avoid shadowing the SDK's WSMObject
		// base class type.
		::WSMObject* Get_WSMObject() { return (::WSMObject*)GetReference(OBJ_REF); }
		Interval     Get_Validity(TimeValue t);

		void Create_Named_Selection_Sets();
		void Install_Named_Selection_Sets();

		enum { OBJ_REF = 0, NODE_REF = 1 };
		enum { OBJECT_SEL_LEVEL = 0, VERTEX_SEL_LEVEL = 1 };
		enum { SEL_LEVEL_CHUNK = 0xAA01 };

		int                  SubObjSelLevel;
		SkinWSMObjectClass*  WSMObjectRef;
		INode*               WSMNodeRef;

		// Modifier-panel UI state (Phase 6). Populated by BeginEditParams +
		// ActivateSubobjSel, torn down by EndEditParams. May be null when the
		// rollout isn't open.
		IObjParam*           InterfacePtr;
		HWND                 BoneInfluenceHWND;
		ICustButton*         LinkButton;
		ICustButton*         LinkByNameButton;
		ICustButton*         AutoLinkButton;
		ICustButton*         UnlinkButton;

		// Sub-object selection command mode. Allocated in BeginEditParams,
		// returned to Max via the `modes` out-parameter of ActivateSubobjSel
		// when the user switches to vertex level, and freed in EndEditParams.
		// Without this, Max has no command mode registered for the modifier's
		// vertex level and the user's marquee/click selections in the
		// viewport are silently ignored.
		SelectModBoxCMode*   SelectMode;
	};

	class SkinModifierClassDesc : public ClassDesc2
	{
	public:
		int           IsPublic() override { return FALSE; }           // Created via the WSM, not directly.
		void*         Create(BOOL loading = FALSE) override { return new SkinModifierClass(); }
		const TCHAR*  ClassName() override { return _T("WWSkin"); }
		const TCHAR*  NonLocalizedClassName() override { return _T("WWSkin"); }
		// Distinct from the WSM Object's InternalName so MAXScript can name
		// the modifier separately. Matches the existing importer's lookup at
		// `mesh.modifiers[#WWSkin_Binding]`.
		const TCHAR*  InternalName() override { return _T("WWSkin_Binding"); }
		SClass_ID     SuperClassID() override { return WSM_CLASS_ID; }
		Class_ID      ClassID() override { return Class_ID(0x6BAD4898, 0x0D1D6CED); }
		const TCHAR*  Category() override { return _T("Westwood Space Warps"); }
		static SkinModifierClassDesc* Instance();

	private:
		SkinModifierClassDesc() = default;
	};
}
