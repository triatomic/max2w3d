// Bone picker implementation — Phase 3 of the WWSkin port.
//
// Two pick paths share this class:
//   - Viewport pick (via IObjParam::SetPickMode): user clicks a node in a
//     viewport. Filter() gates which nodes are valid; Pick() forwards the
//     hit to the BonePickerUserClass.
//   - "Pick By Name" dialog (IObjParam::DoHitByNameDialog): the H-key /
//     by-name flow. proc() forwards the multi-select result.
//
// EA's version pulled the dialog title and button text from string-table
// resources via Get_String(IDS_*). We use plain inline _M("…") literals here;
// when Phase 6 wires up the .rc additions we can swap to LoadString-backed
// resources for localization parity, but the literals work for both light/
// dark themes since Max handles HitByNameDlg chrome itself.

#include "bpick.h"

namespace W3D::MaxTools
{
	BonePickerClass TheBonePicker;

	BOOL BonePickerClass::Filter(INode* node)
	{
		if (BoneList == nullptr)
		{
			// No restriction list: any node that evaluates to a real object.
			ObjectState os = node->EvalWorldState(0);
			return os.obj != nullptr;
		}
		// Restricted list: the node must be in BoneList.
		for (int i = 0; i < BoneList->Count(); ++i)
		{
			if ((*BoneList)[i] == node) return TRUE;
		}
		return FALSE;
	}

	BOOL BonePickerClass::HitTest(IObjParam* ip, HWND hwnd, ViewExp* /*vpt*/,
	                              IPoint2 m, int /*flags*/)
	{
		return ip->PickNode(hwnd, m, GetFilter()) != nullptr;
	}

	BOOL BonePickerClass::Pick(IObjParam* /*ip*/, ViewExp* vpt)
	{
		INode* node = vpt->GetClosestHit();
		if (node && User)
		{
			User->User_Picked_Bone(node);
		}
		// One-shot: clear the User reference so a stale picker can't fire twice.
		User = nullptr;
		BoneList = nullptr;
		return TRUE;
	}

	int BonePickerClass::filter(INode* inode)
	{
		// HitByNameDlgCallback uses int (not BOOL) but the meaning is the same.
		return Filter(inode) ? TRUE : FALSE;
	}

	void BonePickerClass::proc(INodeTab& nodetab)
	{
		if (User) User->User_Picked_Bones(nodetab);
		User = nullptr;
		BoneList = nullptr;
	}
}
