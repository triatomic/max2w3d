// W3D Tools — WWSkin (Westwood skinning) Space Warp + Binding Modifier.
//
// See w3dskin.h for an overview of how this lives alongside Max's native Skin.
// Phase 1+2 of the re-introduction: full data model, bone management on the WSM,
// ModifyObject deformation on the binding, save/load matching EA's chunk layout.
// UI rollouts (Phase 6), bone picker (Phase 3), bone icons (Phase 4), and named
// selection sets (Phase 5) are deliberately stubbed here and filled in later.

#include "w3dskin.h"
#include "boneicon.h"
#include "engine_string.h"
#include "resource.h"

#include <modstack.h>
#include <iskin.h>
#include <triobj.h>
#include <iparamb2.h>
#include <custcont.h>
#include <MeshNormalSpec.h>	// MeshNormalSpec / MeshNormalFace / MESH_NORMAL_MODIFIER_SUPPORT
                                // — only forward-declared by <max.h>, so the full header is
                                // required for the per-frame normal-skinning loop in ModifyObject.

extern HINSTANCE hInstance;

namespace W3D::MaxTools
{
	// =========================================================================
	// SkinDataClass — per-mesh ModContext data (vertex influences + selection)
	// =========================================================================
	SkinDataClass::SkinDataClass(Mesh* mesh)
		: Valid(FALSE), Held(FALSE)
	{
		VertSel = mesh->vertSel;
		VertData.SetCount(mesh->getNumVerts());
		Capture_Base_Normals(mesh);
		Valid = TRUE;
	}

	void SkinDataClass::Validate(Mesh* mesh)
	{
		if (Valid) return;
		VertSel.SetSize(mesh->vertSel.GetSize(), 1);
		VertData.SetCount(mesh->getNumVerts());
		Capture_Base_Normals(mesh);
		Valid = TRUE;
	}

	void SkinDataClass::Capture_Base_Normals(Mesh* mesh)
	{
		BaseNormals.ZeroCount();
		NormalToVert.ZeroCount();

		if (!mesh) return;
		MeshNormalSpec* spec = mesh->GetSpecifiedNormals();
		if (!spec) return;

		const int numNormals = spec->GetNumNormals();
		if (numNormals <= 0) return;

		// Bail if no Explicit entries — ModifyObject only re-skins Explicit normals,
		// so capturing the auto-derived ones would just waste memory.
		bool anyExplicit = false;
		for (int i = 0; i < numNormals; ++i)
		{
			if (spec->GetNormalExplicit(i)) { anyExplicit = true; break; }
		}
		if (!anyExplicit) return;

		BaseNormals.SetCount(numNormals);
		NormalToVert.SetCount(numNormals);
		for (int i = 0; i < numNormals; ++i)
		{
			BaseNormals[i] = spec->Normal(i);
			NormalToVert[i] = -1;
		}

		// Walk faces×corners to associate each normal slot with the vert that drives
		// it. Multiple corners may share a normal index — they always belong to the
		// same vertex (CheckNormals splits per smoothing-group), so the last write
		// is consistent with all earlier writes.
		const int numFaces = spec->GetNumFaces();
		for (int f = 0; f < numFaces; ++f)
		{
			MeshNormalFace& nf = spec->Face(f);
			Face& mf = mesh->faces[f];
			for (int c = 0; c < 3; ++c)
			{
				const int nid = nf.GetNormalID(c);
				if (nid < 0 || nid >= numNormals) continue;
				NormalToVert[nid] = (int)mf.v[c];
			}
		}
	}

	LocalModData* SkinDataClass::Clone()
	{
		auto* nd = new SkinDataClass();
		nd->VertSel      = VertSel;
		nd->VertData     = VertData;
		nd->BaseNormals  = BaseNormals;
		nd->NormalToVert = NormalToVert;
		nd->Valid        = Valid;
		nd->Held         = Held;
		return nd;
	}

	void SkinDataClass::Add_Influence(int boneidx)
	{
		for (int i = 0; i < VertData.Count(); ++i)
		{
			if (VertSel[i]) VertData[i].Set_Influence(boneidx);
		}
	}

	IOResult SkinDataClass::Save(ISave* isave)
	{
		ULONG nb;
		short flags = 0;
		if (Valid) flags |= 0x01;
		if (Held)  flags |= 0x02;

		isave->BeginChunk(FLAGS_CHUNK);
		isave->Write(&flags, sizeof(flags), &nb);
		isave->EndChunk();

		if (VertSel.NumberSet() > 0)
		{
			isave->BeginChunk(VERT_SEL_CHUNK);
			VertSel.Save(isave);
			isave->EndChunk();
		}

		if (VertData.Count() > 0)
		{
			isave->BeginChunk(INFLUENCE_DATA_CHUNK);
			isave->Write(reinterpret_cast<const BYTE*>(VertData.Addr(0)),
			             (ULONG)(VertData.Count() * sizeof(InfluenceStruct)), &nb);
			isave->EndChunk();
		}
		return IO_OK;
	}

	IOResult SkinDataClass::Load(ILoad* iload)
	{
		ULONG nb;
		IOResult res;
		while (IO_OK == (res = iload->OpenChunk()))
		{
			switch (iload->CurChunkID())
			{
				case FLAGS_CHUNK:
				{
					short flags;
					res = iload->Read(&flags, sizeof(flags), &nb);
					Valid = (flags & 0x01) ? TRUE : FALSE;
					Held  = (flags & 0x02) ? TRUE : FALSE;
				}
				break;

				case VERT_SEL_CHUNK:
					res = VertSel.Load(iload);
					break;

				case NAMED_SEL_SETS_CHUNK:
					res = VertSelSets.Load(iload);
					break;

				case INFLUENCE_DATA_CHUNK:
				{
					int n = (int)(iload->CurChunkLength() / sizeof(InfluenceStruct));
					VertData.SetCount(n);
					if (n > 0)
					{
						res = iload->Read(reinterpret_cast<BYTE*>(VertData.Addr(0)),
						                  (ULONG)(n * sizeof(InfluenceStruct)), &nb);
					}
				}
				break;
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}
		Invalidate();
		return IO_OK;
	}

	// =========================================================================
	// Class descriptors (singleton accessors)
	// =========================================================================
	SkinWSMObjectClassDesc* SkinWSMObjectClassDesc::Instance()
	{
		static SkinWSMObjectClassDesc s_instance;
		return &s_instance;
	}

	SkinModifierClassDesc* SkinModifierClassDesc::Instance()
	{
		static SkinModifierClassDesc s_instance;
		return &s_instance;
	}

	// =========================================================================
	// Helpers
	// =========================================================================

	// Convert (or return as-is) the current ObjectState into a TriObject for vertex
	// access. needsdel signals that the caller must DeleteThis() the result.
	static TriObject* Get_Tri_Object(TimeValue t, ObjectState& os,
	                                 Interval& valid, BOOL& needsdel)
	{
		needsdel = FALSE;
		valid &= os.Validity(t);
		if (os.obj->IsSubClassOf(triObjectClassID))
		{
			return static_cast<TriObject*>(os.obj);
		}
		if (os.obj->CanConvertToType(triObjectClassID))
		{
			Object* old = os.obj;
			TriObject* tri = static_cast<TriObject*>(
			    os.obj->ConvertToType(t, triObjectClassID));
			needsdel = (tri != old);
			return tri;
		}
		return nullptr;
	}

	// Average pivot of a bone with its children, distance to the candidate vertex.
	static float Bone_Distance(INode* bone, TimeValue t, const Point3& vertex)
	{
		Point3 c = bone->GetObjectTM(t).GetTrans();
		const int n = bone->NumberOfChildren();
		for (int i = 0; i < n; ++i)
		{
			c += bone->GetChildNode(i)->GetObjectTM(t).GetTrans();
		}
		c = c / float(n + 1);
		return Length(c - vertex);
	}

	// =========================================================================
	// SkinWSMObjectClass
	// =========================================================================

	// Mouse callback for placing a fresh WSM in the viewport.
	class SkinWSMCreateCB : public CreateMouseCallBack
	{
	public:
		int proc(ViewExp* vpt, int msg, int /*point*/, int /*flags*/,
		         IPoint2 m, Matrix3& mat) override
		{
			if (msg == MOUSE_POINT)
			{
				Point3 pos = vpt->GetPointOnCP(m);
				mat.IdentityMatrix();
				mat.SetTrans(pos);
				return CREATE_STOP;
			}
			return TRUE;
		}
	};
	static SkinWSMCreateCB s_SkinCreateCB;

	SkinWSMObjectClass::SkinWSMObjectClass()
		: BasePoseFrame(0), MeshBuilt(FALSE), BoneSelectionMode(BONE_SEL_MODE_NONE),
		  InterfacePtr(nullptr), SkeletonHWND(nullptr), BoneListHWND(nullptr),
		  AddBonesButton(nullptr), RemoveBonesButton(nullptr),
		  PickByNameButton(nullptr), BasePoseSpin(nullptr),
		  ClassicMode(false)
	{
		BoneTab.SetCount(0);
	}

	SkinWSMObjectClass::~SkinWSMObjectClass()
	{
		// EndEditParams normally clears UI. Defensive cleanup if Max destroys us
		// without a matching EndEditParams call (which can happen during plugin
		// reload or class-descriptor teardown).
		if (SkeletonHWND && InterfacePtr)
		{
			InterfacePtr->UnRegisterDlgWnd(SkeletonHWND);
			InterfacePtr->DeleteRollupPage(SkeletonHWND);
			SkeletonHWND = nullptr;
		}
	}

	void SkinWSMObjectClass::GetClassName(MSTR& s, bool /*localized*/) const
	{
		s = _M("WWSkin");
	}

	Class_ID SkinWSMObjectClass::ClassID()
	{
		return SkinWSMObjectClassDesc::Instance()->ClassID();
	}

	// Forward-declared dialog proc — implementation lives below the helpers.
	static INT_PTR CALLBACK Skeleton_DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK BoneInfluence_DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Repaints the bone listbox to reflect the current BoneTab. Filters out the
	// null slots that Remove_Bone leaves behind for slot-reuse on next Add.
	static void UpdateBoneList(SkinWSMObjectClass* obj)
	{
		if (!obj || !obj->BoneListHWND) return;
		SendMessage(obj->BoneListHWND, LB_RESETCONTENT, 0, 0);
		for (int i = 0; i < obj->BoneTab.Count(); ++i)
		{
			INode* n = obj->BoneTab[i];
			if (!n) continue;
			SendMessage(obj->BoneListHWND, LB_ADDSTRING, 0, (LPARAM)n->GetName());
		}
	}

	// Maps a listbox row index back to the corresponding BoneTab slot. The listbox
	// only renders non-null entries (UpdateBoneList skips nulls), so position N in
	// the listbox is the Nth surviving bone in BoneTab — not necessarily BoneTab[N].
	static int BoneTabIndexFromListIndex(SkinWSMObjectClass* obj, int listIdx)
	{
		if (!obj || listIdx < 0) return -1;
		int seen = 0;
		for (int i = 0; i < obj->BoneTab.Count(); ++i)
		{
			if (obj->BoneTab[i] == nullptr) continue;
			if (seen == listIdx) return i;
			++seen;
		}
		return -1;
	}

	// RestoreObj for undo/redo of bone Add / Remove on the WSM.
	//
	// The reference array itself (the variable-length tail past SimpleWSMObject's
	// base refs) is auto-snapshotted by the SDK when ReplaceReference /
	// DeleteReference fire inside theHold.Begin/Accept. But our parallel BoneTab
	// vector — which the SDK doesn't know about — also needs to come back to its
	// pre-op size. This class captures BoneTab on the way down and on first undo
	// captures it again so a subsequent redo can restore the post-op state.
	class BoneTabRestore : public RestoreObj
	{
	public:
		SkinWSMObjectClass* obj;
		INodeTab            undoState;
		INodeTab            redoState;
		bool                haveRedoState;

		BoneTabRestore(SkinWSMObjectClass* o) : obj(o), haveRedoState(false)
		{
			undoState = o->BoneTab;
		}

		void Restore(int isUndo) override
		{
			if (!obj) return;
			if (isUndo)
			{
				redoState     = obj->BoneTab;
				haveRedoState = true;
			}
			obj->BoneTab = undoState;
			UpdateBoneList(obj);
			obj->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		}

		void Redo() override
		{
			if (!obj || !haveRedoState) return;
			obj->BoneTab = redoState;
			UpdateBoneList(obj);
			obj->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
		}

		int  Size() override { return sizeof(*this); }
		TSTR Description() override { return TSTR(_T("WWSkin Bones")); }
	};

	// RestoreObj for undo/redo of per-vertex selection on a WWSkin Binding.
	// Snapshots the SkinDataClass::VertSel BitArray so a marquee/click pick,
	// Select All, Invert, or Clear can be undone like any other Max edit.
	class VertSelRestore : public RestoreObj
	{
	public:
		SkinModifierClass* mod;
		SkinDataClass*     sd;
		BitArray           undoState;
		BitArray           redoState;
		bool               haveRedoState;

		VertSelRestore(SkinModifierClass* m, SkinDataClass* s)
			: mod(m), sd(s), haveRedoState(false)
		{
			undoState = s->VertSel;
		}

		void Restore(int isUndo) override
		{
			if (!sd) return;
			if (isUndo)
			{
				redoState     = sd->VertSel;
				haveRedoState = true;
			}
			sd->VertSel = undoState;
			if (mod) mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		}

		void Redo() override
		{
			if (!sd || !haveRedoState) return;
			sd->VertSel = redoState;
			if (mod) mod->NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
		}

		int  Size() override { return sizeof(*this); }
		TSTR Description() override { return TSTR(_T("Vertex Selection")); }
	};

	void SkinWSMObjectClass::BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev)
	{
		SimpleWSMObject::BeginEditParams(ip, flags, prev);
		InterfacePtr = ip;

		// The Skeleton rollout belongs in the Modify panel only. During interactive
		// creation (BEGIN_EDIT_CREATE) the user just clicks to place the gizmo — no
		// bone management needed yet. Showing the rollout in Create mode causes it to
		// appear in the shared Create-panel rollup window, which is not scoped to any
		// object type and therefore leaks into Helpers, Cameras, etc.
		if (flags & BEGIN_EDIT_CREATE)
			return;

		if (!SkeletonHWND)
		{
			SkeletonHWND = ip->AddRollupPage(
				hInstance,
				MAKEINTRESOURCE(IDD_WWSKIN_SKELETON),
				Skeleton_DlgProc,
				_M("Skeleton"),
				(LPARAM)this);
		}
		else
		{
			SetWindowLongPtr(SkeletonHWND, GWLP_USERDATA, (LONG_PTR)this);
		}
		UpdateBoneList(this);
	}

	void SkinWSMObjectClass::EndEditParams(IObjParam* ip, ULONG flags, Animatable* next)
	{
		if ((flags & END_EDIT_REMOVEUI) && SkeletonHWND)
		{
			ip->UnRegisterDlgWnd(SkeletonHWND);
			ip->DeleteRollupPage(SkeletonHWND);
			SkeletonHWND = nullptr;
			BoneListHWND = nullptr;
		}
		InterfacePtr = nullptr;
		SimpleWSMObject::EndEditParams(ip, flags, next);
	}

	int SkinWSMObjectClass::NumRefs()
	{
		return SimpleWSMObject::NumRefs() + Num_Bones();
	}

	RefTargetHandle SkinWSMObjectClass::GetReference(int i)
	{
		if (i < SimpleWSMObject::NumRefs()) return SimpleWSMObject::GetReference(i);
		const int boneidx = To_Bone_Index(i);
		if (boneidx < 0 || boneidx >= BoneTab.Count()) return nullptr;
		return BoneTab[boneidx];
	}

	void SkinWSMObjectClass::SetReference(int i, RefTargetHandle rtarg)
	{
		if (i < SimpleWSMObject::NumRefs())
		{
			SimpleWSMObject::SetReference(i, rtarg);
			return;
		}
		const int boneidx = To_Bone_Index(i);
		if (boneidx < 0 || boneidx >= BoneTab.Count()) return;
		BoneTab[boneidx] = static_cast<INode*>(rtarg);
	}

	RefResult SkinWSMObjectClass::NotifyRefChanged(const Interval&, RefTargetHandle hTarget,
	                                               PartID&, RefMessage message, BOOL)
	{
		if (message == REFMSG_TARGET_DELETED)
		{
			for (int i = 0; i < BoneTab.Count(); ++i)
			{
				if (BoneTab[i] == hTarget)
				{
					BoneTab.Delete(i, 1);
					break;
				}
			}
		}
		return REF_SUCCEED;
	}

	RefTargetHandle SkinWSMObjectClass::Clone(RemapDir& /*remap*/)
	{
		return new SkinWSMObjectClass();
	}

	CreateMouseCallBack* SkinWSMObjectClass::GetCreateMouseCallBack()
	{
		return &s_SkinCreateCB;
	}

	Modifier* SkinWSMObjectClass::CreateWSMMod(INode* node)
	{
		return new SkinModifierClass(node, this);
	}

	void SkinWSMObjectClass::BuildMesh(TimeValue /*t*/)
	{
		// Mesh data doesn't animate, so build it once and pin validity to FOREVER.
		if (MeshBuilt) return;
		ivalid = FOREVER;

		mesh.setNumVerts(NumBoneIconVerts);
		mesh.setNumFaces(NumBoneIconFaces);
		for (int i = 0; i < NumBoneIconVerts; ++i)
		{
			const BoneIconVertex& v = BoneIconVerts[i];
			mesh.setVert(i, Point3(v.X, v.Y, v.Z));
		}
		for (int i = 0; i < NumBoneIconFaces; ++i)
		{
			const BoneIconFace& bf = BoneIconFaces[i];
			Face* f = &mesh.faces[i];
			f->setVerts(bf.V0, bf.V1, bf.V2);
			f->setSmGroup(0);
			f->setEdgeVisFlags(1, 1, 1);
		}
		mesh.InvalidateGeomCache();
		MeshBuilt = TRUE;
	}

	IOResult SkinWSMObjectClass::Save(ISave* isave)
	{
		ULONG nb;
		SimpleWSMObject::Save(isave);

		ULONG numbones = (ULONG)BoneTab.Count();
		if (numbones > 0)
		{
			isave->BeginChunk(NUM_BONES_CHUNK);
			isave->Write(&numbones, sizeof(ULONG), &nb);
			isave->EndChunk();
		}
		return IO_OK;
	}

	IOResult SkinWSMObjectClass::Load(ILoad* iload)
	{
		SimpleWSMObject::Load(iload);
		IOResult res;
		ULONG nb;
		while (IO_OK == (res = iload->OpenChunk()))
		{
			if (iload->CurChunkID() == NUM_BONES_CHUNK)
			{
				ULONG n;
				res = iload->Read(&n, sizeof(n), &nb);
				BoneTab.SetCount((int)n);
				for (int i = 0; i < BoneTab.Count(); ++i) BoneTab[i] = nullptr;
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}
		return IO_OK;
	}

	int SkinWSMObjectClass::Add_Bone(INode* node)
	{
		if (node == nullptr) return -1;

		// Already present — no state change, no undo entry.
		int boneidx = Find_Bone(node);
		if (boneidx != -1) return boneidx;

		// theHold.Begin/Accept brackets the operation as an undoable step. The
		// BoneTabRestore captures our parallel array; ReplaceReference auto-pushes
		// its own RestoreObj for the reference-array side once Holding is active.
		theHold.Begin();
		if (theHold.Holding()) theHold.Put(new BoneTabRestore(this));

		// Reuse an empty (deleted) slot if there is one.
		boneidx = Find_Bone(nullptr);
		if (boneidx != -1)
		{
			ReplaceReference(To_Ref_Index(boneidx), node);
		}
		else
		{
			// Append a new slot.
			BoneTab.Append(1, &node);
			boneidx = BoneTab.Count() - 1;
			ReplaceReference(To_Ref_Index(boneidx), node);
		}

		theHold.Accept(_T("Add Bone"));
		return boneidx;
	}

	void SkinWSMObjectClass::Add_Bones(INodeTab& nodetab)
	{
		for (int i = 0; i < nodetab.Count(); ++i) Add_Bone(nodetab[i]);
	}

	void SkinWSMObjectClass::Remove_Bone(INode* node)
	{
		const int boneidx = Find_Bone(node);
		if (boneidx == -1) return;

		theHold.Begin();
		if (theHold.Holding()) theHold.Put(new BoneTabRestore(this));

		BoneTab[boneidx] = nullptr;
		DeleteReference(To_Ref_Index(boneidx));

		theHold.Accept(_T("Remove Bone"));
	}

	void SkinWSMObjectClass::Remove_Bones(INodeTab& nodetab)
	{
		for (int i = 0; i < nodetab.Count(); ++i) Remove_Bone(nodetab[i]);
	}

	int SkinWSMObjectClass::Find_Bone(INode* node)
	{
		for (int i = 0; i < BoneTab.Count(); ++i)
		{
			if (BoneTab[i] == node) return i;
		}
		return -1;
	}

	int SkinWSMObjectClass::Find_Closest_Bone(const Point3& vertex)
	{
		float mindist = FLT_MAX;
		int   minidx  = -1;
		const TimeValue baset = Get_Base_Pose_Time();
		for (int i = 0; i < BoneTab.Count(); ++i)
		{
			INode* b = BoneTab[i];
			if (b == nullptr) continue;
			const float d = Bone_Distance(b, baset, vertex);
			if (d < mindist) { mindist = d; minidx = i; }
		}
		return minidx;
	}

	void SkinWSMObjectClass::User_Picked_Bone(INode* node)
	{
		// Dispatch on the current mode. BoneSelectionMode is NOT reset here because
		// the viewport pick mode stays alive across multiple clicks (Pick() returns
		// FALSE). User_Exited_Pick_Mode() resets mode and button state when the user
		// right-clicks or escapes out of the pick mode.
		switch (BoneSelectionMode)
		{
			case BONE_SEL_MODE_ADD:    Add_Bone(node);    break;
			case BONE_SEL_MODE_REMOVE: Remove_Bone(node); break;
			default: break;
		}
		UpdateBoneList(this);
	}

	void SkinWSMObjectClass::User_Exited_Pick_Mode()
	{
		Set_Bone_Selection_Mode(BONE_SEL_MODE_NONE);
		if (AddBonesButton)    AddBonesButton->SetCheck(FALSE);
		if (RemoveBonesButton) RemoveBonesButton->SetCheck(FALSE);
	}

	void SkinWSMObjectClass::User_Picked_Bones(INodeTab& nodetab)
	{
		switch (BoneSelectionMode)
		{
			case BONE_SEL_MODE_ADD_MANY:    Add_Bones(nodetab);    break;
			case BONE_SEL_MODE_REMOVE_MANY: Remove_Bones(nodetab); break;
			default: break;
		}
		Set_Bone_Selection_Mode(BONE_SEL_MODE_NONE);
		UpdateBoneList(this);
	}

	// =========================================================================
	// SkinModifierClass
	// =========================================================================
	SkinModifierClass::SkinModifierClass()
		: SubObjSelLevel(VERTEX_SEL_LEVEL), WSMObjectRef(nullptr), WSMNodeRef(nullptr),
		  InterfacePtr(nullptr), BoneInfluenceHWND(nullptr),
		  LinkButton(nullptr), LinkByNameButton(nullptr),
		  AutoLinkButton(nullptr), UnlinkButton(nullptr),
		  SelectMode(nullptr)
	{
	}

	SkinModifierClass::SkinModifierClass(INode* node, SkinWSMObjectClass* wsmobject)
		: SubObjSelLevel(VERTEX_SEL_LEVEL), WSMObjectRef(nullptr), WSMNodeRef(nullptr),
		  InterfacePtr(nullptr), BoneInfluenceHWND(nullptr),
		  LinkButton(nullptr), LinkByNameButton(nullptr),
		  AutoLinkButton(nullptr), UnlinkButton(nullptr),
		  SelectMode(nullptr)
	{
		ReplaceReference(NODE_REF, node);
		ReplaceReference(OBJ_REF,  wsmobject);
	}

	SkinModifierClass::~SkinModifierClass() = default;

	void SkinModifierClass::GetClassName(MSTR& s, bool /*localized*/) const
	{
		s = _M("WWSkin");
	}

	Class_ID SkinModifierClass::ClassID()
	{
		// IMPORTANT: must return the Modifier's own ClassID, NOT the WSM Object's.
		// The previous shim returned SkinWSMObjectClassDesc::Instance()->ClassID(),
		// which collapsed the two distinct classes into one and confused Max's
		// class-dispatcher on save/load round-trips.
		return SkinModifierClassDesc::Instance()->ClassID();
	}

	void SkinModifierClass::BeginEditParams(IObjParam* ip, ULONG /*flags*/, Animatable* /*prev*/)
	{
		InterfacePtr = ip;

		// Allocate the command mode that drives vertex-level selection. Max
		// gets a pointer to this back via ActivateSubobjSel's `modes` out-param;
		// without it, marquee/click selections at vertex level are no-ops.
		if (SelectMode == nullptr)
			SelectMode = new SelectModBoxCMode(this, ip);

		// Sub-object types (declared via NumSubObjTypes/GetSubObjType — Max 4+
		// way) are picked up automatically. Restore the persisted level so the
		// modifier comes up where the user left it.
		if (InterfacePtr) InterfacePtr->SetSubObjectLevel(SubObjSelLevel);

		// Bone Influence rollout only makes sense at vertex level; ActivateSubobjSel
		// installs/removes it as the user toggles between Object and Vertex.
		if (SubObjSelLevel == VERTEX_SEL_LEVEL && !BoneInfluenceHWND)
		{
			BoneInfluenceHWND = ip->AddRollupPage(
				hInstance,
				MAKEINTRESOURCE(IDD_WWSKIN_BONE_INFLUENCE),
				BoneInfluence_DlgProc,
				_M("Bone Influences"),
				(LPARAM)this);
		}
	}

	void SkinModifierClass::EndEditParams(IObjParam* ip, ULONG flags, Animatable* /*next*/)
	{
		// Force-exit sub-object level FIRST, before destroying any rollup or
		// command mode. Without this, Max's command-mode stack can be left
		// holding our (about-to-be-deleted) SelectMode as the active mode —
		// the symptom is that after closing the modifier, viewport clicks no
		// longer deselect the mesh until the user manually changes tool.
		// Setting level back to OBJECT_SEL_LEVEL pops our mode cleanly.
		if (ip)
		{
			ip->SetSubObjectLevel(0);
			ip->ClearPickMode();
		}
		if ((flags & END_EDIT_REMOVEUI) && BoneInfluenceHWND && ip)
		{
			ip->UnRegisterDlgWnd(BoneInfluenceHWND);
			ip->DeleteRollupPage(BoneInfluenceHWND);
			BoneInfluenceHWND = nullptr;
		}
		// Tear down the sub-object command mode in lock-step with the dialog;
		// EA's WSM did this in EndEditParams and the SDK expects DeleteMode
		// before the BaseObject* it was bound to may be invalidated.
		if (ip && SelectMode)
		{
			ip->DeleteMode(SelectMode);
		}
		if (SelectMode)
		{
			delete SelectMode;
			SelectMode = nullptr;
		}
		InterfacePtr = nullptr;
	}

	RefTargetHandle SkinModifierClass::GetReference(int i)
	{
		switch (i)
		{
			case OBJ_REF:  return WSMObjectRef;
			case NODE_REF: return WSMNodeRef;
		}
		return nullptr;
	}

	void SkinModifierClass::SetReference(int i, RefTargetHandle rtarg)
	{
		switch (i)
		{
			case OBJ_REF:  WSMObjectRef = static_cast<SkinWSMObjectClass*>(rtarg); break;
			case NODE_REF: WSMNodeRef   = static_cast<INode*>(rtarg);              break;
		}
	}

	RefResult SkinModifierClass::NotifyRefChanged(const Interval&, RefTargetHandle /*hTarget*/,
	                                              PartID&, RefMessage message, BOOL)
	{
		if (message == REFMSG_TARGET_DELETED)
		{
			DeleteMe();
			return REF_STOP;
		}
		return REF_SUCCEED;
	}

	RefTargetHandle SkinModifierClass::Clone(RemapDir& /*remap*/)
	{
		return new SkinModifierClass(WSMNodeRef, WSMObjectRef);
	}

	IOResult SkinModifierClass::Save(ISave* isave)
	{
		ULONG nb;
		Modifier::Save(isave);
		short sl = (short)SubObjSelLevel;
		isave->BeginChunk(SEL_LEVEL_CHUNK);
		isave->Write(&sl, sizeof(short), &nb);
		isave->EndChunk();
		return IO_OK;
	}

	IOResult SkinModifierClass::Load(ILoad* iload)
	{
		Modifier::Load(iload);
		IOResult res;
		ULONG nb;
		while (IO_OK == (res = iload->OpenChunk()))
		{
			if (iload->CurChunkID() == SEL_LEVEL_CHUNK)
			{
				short sl;
				res = iload->Read(&sl, sizeof(short), &nb);
				SubObjSelLevel = sl;
			}
			iload->CloseChunk();
			if (res != IO_OK) return res;
		}
		return IO_OK;
	}

	IOResult SkinModifierClass::SaveLocalData(ISave* isave, LocalModData* ld)
	{
		return static_cast<SkinDataClass*>(ld)->Save(isave);
	}

	IOResult SkinModifierClass::LoadLocalData(ILoad* iload, LocalModData** pld)
	{
		if (*pld == nullptr) *pld = new SkinDataClass();
		return static_cast<SkinDataClass*>(*pld)->Load(iload);
	}

	Interval SkinModifierClass::Get_Validity(TimeValue t)
	{
		// Conservative: bones may animate, so the binding's deformation is only
		// valid for the current frame.
		return Interval(t, t + 1);
	}

	void SkinModifierClass::ModifyObject(TimeValue t, ModContext& mc, ObjectState* os, INode* /*node*/)
	{
		if (!os->obj || !os->obj->IsSubClassOf(triObjectClassID)) return;
		TriObject* triobj = static_cast<TriObject*>(os->obj);

		SkinDataClass* skindata = static_cast<SkinDataClass*>(mc.localData);
		if (skindata == nullptr)
		{
			skindata = new SkinDataClass(&triobj->mesh);
			mc.localData = skindata;
		}
		if (!skindata->IsValid()) skindata->Validate(&triobj->mesh);

		// Show vertex tick marks while in vertex sub-object mode.
		if (SubObjSelLevel == VERTEX_SEL_LEVEL)
		{
			triobj->mesh.vertSel = skindata->VertSel;
			triobj->mesh.SetDispFlag(DISP_VERTTICKS | DISP_SELVERTS);
			if (triobj->mesh.selLevel != MESH_VERTEX) triobj->mesh.selLevel = MESH_VERTEX;
		}
		else
		{
			triobj->mesh.selLevel = MESH_OBJECT;
			triobj->mesh.ClearDispFlag(DISP_VERTTICKS | DISP_SELVERTS);
		}

		if (WSMObjectRef == nullptr)
		{
			triobj->PointsWereChanged();
			triobj->UpdateValidity(GEOM_CHAN_NUM, Get_Validity(t));
			return;
		}

		const TimeValue basetime = WSMObjectRef->Get_Base_Pose_Time();
		Matrix3 worldTM;
		if (os->GetTM()) worldTM = *os->GetTM(); else worldTM.IdentityMatrix();
		const Matrix3 invWorld = Inverse(worldTM);

		const int numPts = triobj->NumPoints();
		for (int v = 0; v < numPts; ++v)
		{
			InfluenceStruct& inf = skindata->VertData[v];
			const int boneidx = inf.BoneIdx[0];
			if (boneidx < 0 || boneidx >= WSMObjectRef->Num_Bones()) continue;

			INode* bone = WSMObjectRef->Get_Bone(boneidx);
			if (bone == nullptr) { inf.BoneIdx[0] = -1; continue; }

			Point3 p = triobj->GetPoint(v);
			p = worldTM * p;

			const Matrix3 baseTM = bone->GetObjectTM(basetime);
			const Matrix3 curTM  = bone->GetObjectTM(t);
			p = (p * Inverse(baseTM)) * curTM;

			p = invWorld * p;
			triobj->SetPoint(v, p);
		}

		triobj->PointsWereChanged();
		triobj->UpdateValidity(GEOM_CHAN_NUM, Get_Validity(t));

		// Re-skin Explicit base-mesh normals so they follow the bones. Without this,
		// "Use 3dsMax8 Normals" + WWSkin leaves the normals frozen in bind pose and
		// shading goes wrong as soon as the rig animates. Only runs when the user has
		// actually marked normals Explicit (Capture_Base_Normals returns empty arrays
		// otherwise), so non-Max8-Normals workflows pay no cost.
		MeshNormalSpec* spec = triobj->mesh.GetSpecifiedNormals();
		if (spec)
		{
			// MESH_NORMAL_MODIFIER_SUPPORT must be set for any modifier that alters
			// PART_GEOM/PART_TOPO of a TriObject, otherwise Max clears all Specified/
			// Explicit normals after our evaluation. We do alter PART_GEOM (SetPoint +
			// PointsWereChanged above), so we have to opt in here regardless of whether
			// the per-frame re-skin loop below runs — even just preserving normals
			// untouched is enough reason to set this flag.
			spec->SetFlag(MESH_NORMAL_MODIFIER_SUPPORT);

			if (skindata->BaseNormals.Count() > 0
			    && spec->GetNumNormals() == skindata->BaseNormals.Count())
			{
				const int numBones = WSMObjectRef->Num_Bones();
				Tab<Matrix3> boneDelta;   // Inverse(baseTM) * curTM, per bone
				Tab<bool>    boneValid;
				boneDelta.SetCount(numBones);
				boneValid.SetCount(numBones);
				for (int b = 0; b < numBones; ++b)
				{
					boneValid[b] = false;
					INode* bn = WSMObjectRef->Get_Bone(b);
					if (!bn) continue;
					const Matrix3 baseTM = bn->GetObjectTM(basetime);
					const Matrix3 curTM  = bn->GetObjectTM(t);
					boneDelta[b] = Inverse(baseTM) * curTM;
					boneValid[b] = true;
				}

				const int numNormals = spec->GetNumNormals();
				for (int n = 0; n < numNormals; ++n)
				{
					if (!spec->GetNormalExplicit(n)) continue;
					const int vertIdx = skindata->NormalToVert[n];
					if (vertIdx < 0 || vertIdx >= skindata->VertData.Count()) continue;
					const int boneidx = skindata->VertData[vertIdx].BoneIdx[0];
					if (boneidx < 0 || boneidx >= numBones || !boneValid[boneidx]) continue;

					// Mirror the position pipeline: mesh-local -> world -> bone-delta -> mesh-local.
					// VectorTransform applies rotation only (drops the translation row), which is
					// what we want for direction vectors.
					Point3 nrm = skindata->BaseNormals[n];
					nrm = VectorTransform(worldTM,            nrm);
					nrm = VectorTransform(boneDelta[boneidx], nrm);
					nrm = VectorTransform(invWorld,           nrm);
					spec->Normal(n) = Normalize(nrm);
				}

				triobj->mesh.InvalidateGeomCache();
			}
		}
	}

	// --- Sub-object selection (the bulk of HitTest/SelectSubComponent/Clear/Select/
	// InvertSelection are direct ports of EA's logic, with the const-ref Interval and
	// Max-2023 ModContextList signatures). -----------------------------------------

	void SkinModifierClass::ActivateSubobjSel(int level, XFormModes& modes)
	{
		SubObjSelLevel = level;

		// Hand Max our SelectModBoxCMode for the vertex level so marquee /
		// click selections in the viewport actually drive HitTest and
		// SelectSubComponent. The other XFormModes slots (move/rotate/scale)
		// stay null because vertex influences aren't transformable — only
		// selection matters.
		if (level == VERTEX_SEL_LEVEL && SelectMode)
		{
			modes = XFormModes(nullptr, nullptr, nullptr, nullptr, nullptr, SelectMode);
		}
		else if (level == OBJECT_SEL_LEVEL && InterfacePtr)
		{
			// Returning to object level: drop any pick mode our SelectMode
			// might still own so the user can deselect the mesh by clicking
			// in empty viewport without first having to change tool.
			InterfacePtr->ClearPickMode();
		}

		// Install/remove the bone-influence rollout to mirror sub-object level.
		// EA showed it only at vertex level; we follow suit since the buttons
		// operate on selected vertices.
		if (InterfacePtr)
		{
			if (level == VERTEX_SEL_LEVEL && !BoneInfluenceHWND)
			{
				BoneInfluenceHWND = InterfacePtr->AddRollupPage(
					hInstance,
					MAKEINTRESOURCE(IDD_WWSKIN_BONE_INFLUENCE),
					BoneInfluence_DlgProc,
					_M("Bone Influences"),
					(LPARAM)this);
			}
			else if (level != VERTEX_SEL_LEVEL && BoneInfluenceHWND)
			{
				InterfacePtr->UnRegisterDlgWnd(BoneInfluenceHWND);
				InterfacePtr->DeleteRollupPage(BoneInfluenceHWND);
				BoneInfluenceHWND = nullptr;
			}
		}

		Create_Named_Selection_Sets();
		NotifyDependents(FOREVER, PART_SUBSEL_TYPE | PART_DISPLAY, REFMSG_CHANGE);
		if (InterfacePtr) InterfacePtr->PipeSelLevelChanged();
		NotifyDependents(FOREVER, SELECT_CHANNEL | DISP_ATTRIB_CHANNEL | SUBSEL_TYPE_CHANNEL,
		                 REFMSG_CHANGE);
	}

	int SkinModifierClass::HitTest(TimeValue, INode* inode, int type, int crossing, int flags,
	                               IPoint2* p, ViewExp* vpt, ModContext* mc)
	{
		if (!InterfacePtr) return 0;
		Interval valid = FOREVER;
		BOOL needsdel = FALSE;
		HitRegion hr;
		MakeHitRegion(hr, type, crossing, 4, p);
		Matrix3 mat = inode->GetObjectTM(InterfacePtr->GetTime());

		GraphicsWindow* gw = vpt->getGW();
		const DWORD savedLimits = gw->getRndLimits();
		gw->setHitRegion(&hr);
		gw->setTransform(mat);
		gw->setRndLimits((savedLimits | GW_PICK | GW_BACKCULL) & ~GW_ILLUM);
		gw->clearHitCode();

		ObjectState os = inode->EvalWorldState(InterfacePtr->GetTime());
		TriObject* tobj = Get_Tri_Object(InterfacePtr->GetTime(), os, valid, needsdel);
		int res = 0;
		if (tobj)
		{
			SubObjHitList hitlist;
			res = tobj->mesh.SubObjectHitTest(gw, gw->getMaterial(), &hr,
			                                  flags | SUBHIT_VERTS, hitlist);
			// Max 2017+ exposes SubObjHitList as a Tab-backed iterable.
			for (auto it = hitlist.begin(); it != hitlist.end(); ++it)
			{
				vpt->LogHit(inode, mc, it->dist, it->index, nullptr);
			}
			if (needsdel) tobj->DeleteThis();
		}

		gw->setRndLimits(savedLimits);
		return res;
	}

	void SkinModifierClass::SelectSubComponent(HitRecord* hitRec, BOOL selected, BOOL all,
	                                           BOOL invert)
	{
		if (SubObjSelLevel != VERTEX_SEL_LEVEL) return;

		// One undo entry per call (single click or full marquee). Snapshot each
		// distinct SkinDataClass we touch — multi-mesh edits get one Restore per
		// mesh, all replayed atomically on undo/redo.
		theHold.Begin();
		Tab<SkinDataClass*> snapshotted;
		while (hitRec)
		{
			SkinDataClass* sd = static_cast<SkinDataClass*>(hitRec->modContext->localData);
			if (sd)
			{
				bool seen = false;
				for (int i = 0; i < snapshotted.Count(); ++i)
					if (snapshotted[i] == sd) { seen = true; break; }
				if (!seen)
				{
					if (theHold.Holding()) theHold.Put(new VertSelRestore(this, sd));
					snapshotted.Append(1, &sd);
				}

				BitArray& sel = sd->VertSel;
				if (all && invert)
				{
					if (sel[hitRec->hitInfo]) sel.Clear(hitRec->hitInfo);
					else                       sel.Set(hitRec->hitInfo, selected);
				}
				else
				{
					sel.Set(hitRec->hitInfo, selected);
				}
			}
			if (!all) break;
			hitRec = hitRec->Next();
		}
		theHold.Accept(_T("Select Vertices"));
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	}

	// Helper for ClearSelection/SelectAll/InvertSelection: walk every modContext we
	// have, mutate the SkinDataClass::VertSel under op(), and notify dependents.
	template <typename Op>
	static void For_Each_Skindata(IObjParam* ip, Op op)
	{
		if (!ip) return;
		ModContextList mcl;
		INodeTab nodes;
		ip->GetModContexts(mcl, nodes);
		for (int i = 0; i < mcl.Count(); ++i)
		{
			auto* sd = static_cast<SkinDataClass*>(mcl[i]->localData);
			if (sd) op(sd);
		}
		nodes.DisposeTemporary();
	}

	void SkinModifierClass::ClearSelection(int /*selLevel*/)
	{
		theHold.Begin();
		SkinModifierClass* self = this;
		For_Each_Skindata(InterfacePtr, [self](SkinDataClass* sd) {
			if (theHold.Holding()) theHold.Put(new VertSelRestore(self, sd));
			sd->VertSel.ClearAll();
		});
		theHold.Accept(_T("Clear Selection"));
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	}

	void SkinModifierClass::SelectAll(int /*selLevel*/)
	{
		theHold.Begin();
		SkinModifierClass* self = this;
		For_Each_Skindata(InterfacePtr, [self](SkinDataClass* sd) {
			if (theHold.Holding()) theHold.Put(new VertSelRestore(self, sd));
			sd->VertSel.SetAll();
		});
		theHold.Accept(_T("Select All"));
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	}

	void SkinModifierClass::InvertSelection(int /*selLevel*/)
	{
		theHold.Begin();
		SkinModifierClass* self = this;
		For_Each_Skindata(InterfacePtr, [self](SkinDataClass* sd) {
			if (theHold.Holding()) theHold.Put(new VertSelRestore(self, sd));
			BitArray& sel = sd->VertSel;
			for (int j = 0; j < sel.GetSize(); ++j)
			{
				if (sel[j]) sel.Clear(j); else sel.Set(j);
			}
		});
		theHold.Accept(_T("Invert Selection"));
		NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
	}

	// Sub-object types — Max 4+ way. Vertices is our only level.
	static GenSubObjType s_SubObjTypeVertex(1);
	int  SkinModifierClass::NumSubObjTypes() { return 1; }
	ISubObjType* SkinModifierClass::GetSubObjType(int i)
	{
		static bool inited = false;
		if (!inited) { inited = true; s_SubObjTypeVertex.SetName(_M("Vertices")); }
		if (i == -1)
		{
			if (GetSubObjectLevel() > 0) return GetSubObjType(GetSubObjectLevel() - 1);
		}
		return &s_SubObjTypeVertex;
	}

	// Bone-picker callbacks (Phase 3 hooks the picker UI to these).
	void SkinModifierClass::User_Picked_Bone(INode* node)
	{
		if (!InterfacePtr || !WSMObjectRef) return;
		ModContextList mcl;
		INodeTab nodes;
		InterfacePtr->GetModContexts(mcl, nodes);
		if (mcl.Count() == 0) { nodes.DisposeTemporary(); return; }
		auto* sd = static_cast<SkinDataClass*>(mcl[0]->localData);
		if (!sd) { nodes.DisposeTemporary(); return; }

		const int boneidx = WSMObjectRef->Find_Bone(node);
		if (boneidx >= 0) sd->Add_Influence(boneidx);

		Create_Named_Selection_Sets();
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		InterfacePtr->RedrawViews(InterfacePtr->GetTime());
		nodes.DisposeTemporary();
	}

	void SkinModifierClass::User_Picked_Bones(INodeTab& nodetab)
	{
		for (int i = 0; i < nodetab.Count() && i < 2; ++i) User_Picked_Bone(nodetab[i]);
	}

	void SkinModifierClass::Auto_Attach_Verts(BOOL all)
	{
		if (!InterfacePtr || !WSMObjectRef) return;
		ModContextList mcl;
		INodeTab nodes;
		InterfacePtr->GetModContexts(mcl, nodes);
		if (mcl.Count() == 0) { nodes.DisposeTemporary(); return; }
		auto* sd = static_cast<SkinDataClass*>(mcl[0]->localData);
		if (!sd) { nodes.DisposeTemporary(); return; }

		const TimeValue baset = WSMObjectRef->Get_Base_Pose_Time();
		Interval valid = FOREVER;
		BOOL needsdel = FALSE;
		ObjectState os = nodes[0]->EvalWorldState(baset);
		TriObject* tri = Get_Tri_Object(baset, os, valid, needsdel);

		if (tri)
		{
			for (int v = 0; v < sd->VertData.Count() && v < tri->NumPoints(); ++v)
			{
				if (!all && !sd->VertSel[v]) continue;
				const Point3 p = tri->GetPoint(v);
				const int b = WSMObjectRef->Find_Closest_Bone(p);
				if (b >= 0) sd->VertData[v].Set_Influence(b);
			}
			if (needsdel) tri->DeleteThis();
		}

		Create_Named_Selection_Sets();
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
		InterfacePtr->RedrawViews(InterfacePtr->GetTime());
		nodes.DisposeTemporary();
	}

	void SkinModifierClass::Unlink_Verts()
	{
		For_Each_Skindata(InterfacePtr, [](SkinDataClass* sd) {
			for (int v = 0; v < sd->VertData.Count(); ++v)
			{
				if (sd->VertSel[v]) sd->VertData[v].BoneIdx[0] = -1;
			}
		});
		NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
	}

	// Named selection sets — minimal stubs until Phase 5. The data is preserved on
	// load via SkinDataClass::Load so legacy scenes don't lose anything.
	void SkinModifierClass::ActivateSubSelSet(MSTR& /*setName*/) {}
	void SkinModifierClass::NewSetFromCurSel(MSTR& /*setName*/) { Install_Named_Selection_Sets(); }
	void SkinModifierClass::RemoveSubSelSet (MSTR& /*setName*/) { Install_Named_Selection_Sets(); }

	void SkinModifierClass::Create_Named_Selection_Sets()
	{
		if (!InterfacePtr || !WSMObjectRef) return;
		ModContextList mcl;
		INodeTab nodes;
		InterfacePtr->GetModContexts(mcl, nodes);
		if (mcl.Count() == 0) { nodes.DisposeTemporary(); return; }
		auto* sd = static_cast<SkinDataClass*>(mcl[0]->localData);
		if (!sd) { nodes.DisposeTemporary(); return; }

		sd->VertSelSets.Reset();
		for (int b = 0; b < WSMObjectRef->Num_Bones(); ++b)
		{
			INode* bone = WSMObjectRef->Get_Bone(b);
			if (!bone) continue;
			BitArray verts;
			verts.SetSize(sd->VertData.Count());
			for (int v = 0; v < sd->VertData.Count(); ++v)
			{
				if (sd->VertData[v].BoneIdx[0] == b) verts.Set(v);
				else                                  verts.Clear(v);
			}
			MSTR name = bone->GetName();
			sd->VertSelSets.Append_Set(verts, name);
		}
		Install_Named_Selection_Sets();
		nodes.DisposeTemporary();
	}

	void SkinModifierClass::Install_Named_Selection_Sets()
	{
		if (SubObjSelLevel != VERTEX_SEL_LEVEL || !InterfacePtr) return;
		ModContextList mcl;
		INodeTab nodes;
		InterfacePtr->GetModContexts(mcl, nodes);
		if (mcl.Count() == 0) { nodes.DisposeTemporary(); return; }
		auto* sd = static_cast<SkinDataClass*>(mcl[0]->localData);
		if (sd)
		{
			InterfacePtr->ClearSubObjectNamedSelSets();
			for (int i = 0; i < sd->VertSelSets.Count(); ++i)
			{
				if (sd->VertSelSets.Names[i])
					InterfacePtr->AppendSubObjectNamedSelSet(*sd->VertSelSets.Names[i]);
			}
		}
		nodes.DisposeTemporary();
	}

	// =========================================================================
	// Dialog procs (Phase 6)
	//
	// Both rollouts use Max's CustEdit/SpinnerControl/CustButton custom widgets
	// (declared in the .rc), so they pick up the active 3ds Max theme — light or
	// dark — and DPI scaling automatically. We hold the C++ ICustButton/
	// ISpinnerControl wrappers from WM_INITDIALOG and release them in WM_DESTROY
	// to match the SDK's reference-counting protocol.
	// =========================================================================

	static INT_PTR CALLBACK Skeleton_DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto* obj = (SkinWSMObjectClass*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		switch (msg)
		{
			case WM_INITDIALOG:
			{
				obj = (SkinWSMObjectClass*)lParam;
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)obj);
				if (!obj) return TRUE;

				obj->SkeletonHWND        = hWnd;
				obj->BoneListHWND        = GetDlgItem(hWnd, IDC_WWSKIN_BONE_LIST);
				obj->AddBonesButton      = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_ADD_BONES));
				obj->RemoveBonesButton   = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_REMOVE_BONES));
				obj->PickByNameButton    = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_PICK_BY_NAME));
				obj->BasePoseSpin        = GetISpinner  (GetDlgItem(hWnd, IDC_WWSKIN_BASE_POSE_SPIN));

				if (obj->AddBonesButton)    obj->AddBonesButton->SetType(CBT_CHECK);
				if (obj->RemoveBonesButton) obj->RemoveBonesButton->SetType(CBT_CHECK);
				if (obj->BasePoseSpin)
				{
					obj->BasePoseSpin->SetLimits(0, 99999, FALSE);
					obj->BasePoseSpin->SetValue(obj->BasePoseFrame, FALSE);
					obj->BasePoseSpin->LinkToEdit(GetDlgItem(hWnd, IDC_WWSKIN_BASE_POSE_EDIT), EDITTYPE_INT);
				}

				// Restore persistent Classic mode state into the freshly-created
				// checkbox so it survives dialog recreations.
				CheckDlgButton(hWnd, IDC_WWSKIN_CLASSIC_MODE,
					obj->ClassicMode ? BST_CHECKED : BST_UNCHECKED);

				UpdateBoneList(obj);
				return TRUE;
			}

			case WM_COMMAND:
			{
				if (!obj) return FALSE;

				// Keep the persistent flag in sync whenever the user clicks the
				// checkbox; subsequent reads in this proc (and across dialog
				// recreations) come from obj->ClassicMode, so the user's choice
				// survives even if Max tears the rollout down and rebuilds it.
				if (LOWORD(wParam) == IDC_WWSKIN_CLASSIC_MODE &&
				    HIWORD(wParam) == BN_CLICKED)
				{
					obj->ClassicMode =
						IsDlgButtonChecked(hWnd, IDC_WWSKIN_CLASSIC_MODE) == BST_CHECKED;
					return TRUE;
				}

				// Classic mode (off by default) re-enables the legacy
				// viewport-pick + checked-button "armed" UX. With it off (modern
				// default) Add behaves like Pick By Name and Remove operates on
				// the listbox selection — neither button enters a held state.
				const bool classicMode = obj->ClassicMode;

				switch (LOWORD(wParam))
				{
					case IDC_WWSKIN_ADD_BONES:
					{
						if (classicMode)
						{
							// Legacy: arm pick mode, user clicks a bone in the viewport.
							obj->Set_Bone_Selection_Mode(SkinWSMObjectClass::BONE_SEL_MODE_ADD);
							TheBonePicker.Set_User(obj, TRUE);
							if (obj->InterfacePtr) obj->InterfacePtr->SetPickMode(&TheBonePicker);
						}
						else
						{
							// Modern: open the Select-By-Name dialog (multi-select scene
							// objects), like 3ds Max's native Skin "Add Bones".
							obj->Set_Bone_Selection_Mode(SkinWSMObjectClass::BONE_SEL_MODE_ADD_MANY);
							TheBonePicker.Set_User(obj, FALSE);
							if (obj->InterfacePtr) obj->InterfacePtr->DoHitByNameDialog(&TheBonePicker);
							if (obj->AddBonesButton) obj->AddBonesButton->SetCheck(FALSE);
							UpdateBoneList(obj);
						}
						return TRUE;
					}
					case IDC_WWSKIN_REMOVE_BONES:
					{
						if (classicMode)
						{
							// Legacy: arm pick mode, click the bone in the viewport.
							// (If a listbox row is selected we still take the fast path —
							// it's strictly more convenient than viewport-clicking.)
							const LRESULT sel = SendMessage(obj->BoneListHWND, LB_GETCURSEL, 0, 0);
							if (sel != LB_ERR)
							{
								const int boneIdx = BoneTabIndexFromListIndex(obj, (int)sel);
								if (boneIdx >= 0 && boneIdx < obj->BoneTab.Count())
								{
									INode* bone = obj->BoneTab[boneIdx];
									if (bone) obj->Remove_Bone(bone);
									UpdateBoneList(obj);
								}
								return TRUE;
							}
							obj->Set_Bone_Selection_Mode(SkinWSMObjectClass::BONE_SEL_MODE_REMOVE);
							TheBonePicker.Set_User(obj, TRUE, &obj->BoneTab);
							if (obj->InterfacePtr) obj->InterfacePtr->SetPickMode(&TheBonePicker);
							return TRUE;
						}
						// Modern: open the Select-By-Name dialog filtered to the
						// bones already in the list so the user can multi-select
						// which ones to remove, mirroring native Skin's "Remove Bones".
						obj->Set_Bone_Selection_Mode(SkinWSMObjectClass::BONE_SEL_MODE_REMOVE_MANY);
						TheBonePicker.Set_User(obj, FALSE, &obj->BoneTab);
						if (obj->InterfacePtr) obj->InterfacePtr->DoHitByNameDialog(&TheBonePicker);
						if (obj->RemoveBonesButton) obj->RemoveBonesButton->SetCheck(FALSE);
						UpdateBoneList(obj);
						return TRUE;
					}
					case IDC_WWSKIN_PICK_BY_NAME:
					{
						obj->Set_Bone_Selection_Mode(SkinWSMObjectClass::BONE_SEL_MODE_ADD_MANY);
						TheBonePicker.Set_User(obj, FALSE);
						if (obj->InterfacePtr) obj->InterfacePtr->DoHitByNameDialog(&TheBonePicker);
						UpdateBoneList(obj);
						return TRUE;
					}
				}
				break;
			}

			case CC_SPINNER_CHANGE:
			{
				if (!obj) return FALSE;
				if (LOWORD(wParam) == IDC_WWSKIN_BASE_POSE_SPIN && obj->BasePoseSpin)
				{
					obj->BasePoseFrame = obj->BasePoseSpin->GetIVal();
					obj->NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
				}
				return TRUE;
			}

			case WM_DESTROY:
			{
				if (!obj) return FALSE;
				if (obj->AddBonesButton)    { ReleaseICustButton(obj->AddBonesButton);    obj->AddBonesButton = nullptr; }
				if (obj->RemoveBonesButton) { ReleaseICustButton(obj->RemoveBonesButton); obj->RemoveBonesButton = nullptr; }
				if (obj->PickByNameButton)  { ReleaseICustButton(obj->PickByNameButton);  obj->PickByNameButton = nullptr; }
				if (obj->BasePoseSpin)      { ReleaseISpinner   (obj->BasePoseSpin);      obj->BasePoseSpin = nullptr; }
				obj->BoneListHWND = nullptr;
				return TRUE;
			}
		}
		return FALSE;
	}

	static INT_PTR CALLBACK BoneInfluence_DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto* mod = (SkinModifierClass*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

		switch (msg)
		{
			case WM_INITDIALOG:
			{
				mod = (SkinModifierClass*)lParam;
				SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)mod);
				if (!mod) return TRUE;

				mod->BoneInfluenceHWND = hWnd;
				mod->LinkButton        = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_LINK));
				mod->LinkByNameButton  = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_LINK_BY_NAME));
				mod->AutoLinkButton    = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_AUTO_LINK));
				mod->UnlinkButton      = GetICustButton(GetDlgItem(hWnd, IDC_WWSKIN_UNLINK));
				return TRUE;
			}

			case WM_COMMAND:
			{
				if (!mod || !mod->WSMObjectRef) return FALSE;
				switch (LOWORD(wParam))
				{
					case IDC_WWSKIN_LINK:
					{
						INodeTab* bones = &mod->WSMObjectRef->Get_Bone_List();
						TheBonePicker.Set_User(mod, TRUE, bones);
						if (mod->InterfacePtr) mod->InterfacePtr->SetPickMode(&TheBonePicker);
						return TRUE;
					}
					case IDC_WWSKIN_LINK_BY_NAME:
					{
						INodeTab* bones = &mod->WSMObjectRef->Get_Bone_List();
						TheBonePicker.Set_User(mod, FALSE, bones);
						if (mod->InterfacePtr) mod->InterfacePtr->DoHitByNameDialog(&TheBonePicker);
						return TRUE;
					}
					case IDC_WWSKIN_AUTO_LINK:
						mod->Auto_Attach_Verts(FALSE);
						return TRUE;
					case IDC_WWSKIN_UNLINK:
						mod->Unlink_Verts();
						return TRUE;
				}
				break;
			}

			case WM_DESTROY:
			{
				if (!mod) return FALSE;
				if (mod->LinkButton)       { ReleaseICustButton(mod->LinkButton);       mod->LinkButton = nullptr; }
				if (mod->LinkByNameButton) { ReleaseICustButton(mod->LinkByNameButton); mod->LinkByNameButton = nullptr; }
				if (mod->AutoLinkButton)   { ReleaseICustButton(mod->AutoLinkButton);   mod->AutoLinkButton = nullptr; }
				if (mod->UnlinkButton)     { ReleaseICustButton(mod->UnlinkButton);     mod->UnlinkButton = nullptr; }
				return TRUE;
			}
		}
		return FALSE;
	}
}
