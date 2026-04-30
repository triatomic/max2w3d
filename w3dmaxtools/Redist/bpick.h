#pragma once

// Bone picker — viewport- and dialog-based interactive bone selection for the
// WWSkin Space Warp + Binding modifier. Mirrors EA's bpick.h API so existing
// MAXScript / external callers that referenced the global `TheBonePicker`
// or `BonePickerUserClass` keep compiling. Phase 3 of the WWSkin port.

#include <max.h>

namespace W3D::MaxTools
{
	// Implemented by anything that wants to receive picked-bone callbacks
	// (SkinWSMObjectClass and SkinModifierClass both implement it).
	class BonePickerUserClass
	{
	public:
		virtual ~BonePickerUserClass() = default;
		virtual void User_Picked_Bone(INode* node) = 0;
		virtual void User_Picked_Bones(INodeTab& nodetab) = 0;
	};

	// Picker. Wears three Max-callback hats: PickNodeCallback (filter scene
	// nodes), PickModeCallback (viewport pick mode), HitByNameDlgCallback
	// (the Pick By Name / H-key dialog).
	class BonePickerClass : public PickNodeCallback,
	                        public PickModeCallback,
	                        public HitByNameDlgCallback
	{
	public:
		BonePickerClass() : User(nullptr), BoneList(nullptr), SinglePick(FALSE) {}

		// Configure before invoking. If bonelist == nullptr, any scene node is
		// pickable; otherwise picks are restricted to that list.
		void Set_User(BonePickerUserClass* user, BOOL singlepick = FALSE,
		              INodeTab* bonelist = nullptr)
		{
			User = user; SinglePick = singlepick; BoneList = bonelist;
		}

		// PickNodeCallback
		BOOL Filter(INode* node) override;

		// PickModeCallback
		BOOL HitTest(IObjParam* ip, HWND hWnd, ViewExp* vpt, IPoint2 m, int flags) override;
		BOOL Pick(IObjParam* ip, ViewExp* vpt) override;
		void EnterMode(IObjParam* /*ip*/) override {}
		void ExitMode(IObjParam* /*ip*/)  override {}
		PickNodeCallback* GetFilter() override { return this; }
		BOOL RightClick(IObjParam* /*ip*/, ViewExp* /*vpt*/) override { return TRUE; }

		// HitByNameDlgCallback
		const MCHAR* dialogTitle() override { return _M("Pick Bone(s)"); }
		const MCHAR* buttonText()  override { return _M("Select"); }
		BOOL singleSelect()  override { return SinglePick; }
		BOOL useFilter()     override { return TRUE; }
		int  filter(INode* inode) override;
		BOOL useProc()       override { return TRUE; }
		void proc(INodeTab& nodeTab) override;
		BOOL doCustomHilite() override { return FALSE; }

	protected:
		BonePickerUserClass* User;
		INodeTab*            BoneList;
		BOOL                 SinglePick;
	};

	// Process-wide singleton (matches EA's `extern BonePickerClass TheBonePicker`).
	extern BonePickerClass TheBonePicker;
}
