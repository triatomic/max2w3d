#include <maxheapdirect.h>
#include <iInstanceMgr.h>
#include <notify.h>
#include <sstream>
#include "Dialog/w3dexportsettingsdlg.h"
#include "filefactoryclass.h"
#include "fileclass.h"
#include "engine_string.h"
#include "resource.h"
#include "w3dutilities.h"
#include "iniclass.h"
#include <charconv>

extern HINSTANCE hInstance;

namespace
{
	const std::array<uint16, enum_to_value(W3D::MaxTools::W3DGeometryType::Num)>& GeometryTypeRadioIDs()
	{
		static const std::array<uint16, enum_to_value(W3D::MaxTools::W3DGeometryType::Num)> s_ids
		{
			IDC_GEOM_CAM_PARAL,
			IDC_GEOM_NORMAL,
			IDC_GEOM_OBBOX,
			IDC_GEOM_AABOX,
			IDC_GEOM_CAM_ORIENT,
			IDC_GEOM_NULL_LOD,
			IDC_GEOM_DAZZLE,
			IDC_GEOM_AGGREGATE,
			IDC_GEOM_CAMERA_Z_ORIENTED,
			IDC_GEOM_LIGHT
		};

		return s_ids;
	}


	const std::array<uint16, 10>& GeometryFlagCheckIDs()
	{
		static const std::array<uint16, 10> s_ids 
		{
			IDC_HIDE,
			IDC_TWO_SIDED,
			IDC_SHADOW,
			IDC_VALPHA,
			IDC_Z_NORMAL,
			IDC_SHATTER,
			IDC_TANGENTS,
			IDC_KEEP_NORMAL,
			IDC_PRELIT,
			IDC_ALWAYSDYNLIGHT
		};

		return s_ids;
	}

	const std::array<uint16, 5>& CollisionFlagCheckIDs()
	{
		static const std::array<uint16, 5> s_ids
		{
			IDC_PHYSICAL,
			IDC_PROJECTILE,
			IDC_VIS,
			IDC_CAMERA,
			IDC_VEHICLE
		};

		return s_ids;
	}

	const std::vector<TSTR>& DazzleStrings()
	{
		static const std::vector<TSTR> s_strings = []() -> std::vector<TSTR>
		{
			std::vector<TSTR> result;
			FileClass* iniFile = _TheFileFactory->Get_File("dazzle.ini");
			assert(iniFile != nullptr);

			if (iniFile)
			{
				INIClass dazzleINI(*iniFile);
				INISection* dazzleList = dazzleINI.Find_Section("Dazzles_List");
				assert(dazzleList);

				if (dazzleList)
				{
					result.reserve(dazzleList->EntryList.Get_Valid_Count());
					for (INIEntry* it = dazzleList->EntryList.First(); it->Is_Valid(); it = it->Next())
					{
						result.emplace_back(CStr(it->Value).ToWStr());
					}
				}
			}

			return result;
		}();

		return s_strings;
	}

	int IndexOfDazzleString(const TCHAR* str)
	{
		auto search_it = std::find(DazzleStrings().cbegin(), DazzleStrings().cend(), str);
		return static_cast<int>(search_it == DazzleStrings().cend() ? 0 : search_it - DazzleStrings().cbegin());
	}

	template <typename FUNC>
	void VisitNodeAppData(const std::vector<INode*>& selectedNodes, FUNC&& f)
	{
		using namespace W3D::MaxTools;

		for (INode* node : selectedNodes)
		{
			INodeTab nodes;
			IInstanceMgr::GetInstanceMgr()->GetInstances(*node, nodes);
			for (int i = 0; i < nodes.Count(); ++i)
			{
				f(W3DUtilities::GetOrCreateW3DAppDataChunk(*nodes[i]));
			}
		}
	}

}

namespace W3D::MaxTools
{
	// Static floater state — persists across W3DUtilities activations.
	HWND             W3DExportSettingsDlg::s_FloaterHWND                = nullptr;
	HWND             W3DExportSettingsDlg::s_FloaterDazzleType          = nullptr;
	HWND             W3DExportSettingsDlg::s_FloaterSelectEdit          = nullptr;
	ISpinnerControl* W3DExportSettingsDlg::s_FloaterStaticSortingSpinner = nullptr;
	ISpinnerControl* W3DExportSettingsDlg::s_FloaterScreenSizeSpinner   = nullptr;
	W3DExportSettingsDlg* W3DExportSettingsDlg::s_ActiveInstance        = nullptr;
	std::vector<INode*> W3DExportSettingsDlg::s_FloaterSelection;
	bool             W3DExportSettingsDlg::s_FloaterNotifyRegistered    = false;

	namespace
	{
		// Reads the current scene selection from CoreInterface into the supplied
		// buffer. Used when the floater is open but no W3DUtilities instance is
		// active (the user has switched the Utilities panel away from W3D Tools).
		void FillSelectionFromCore(std::vector<INode*>& sel)
		{
			sel.clear();
			Interface* ip = GetCOREInterface();
			if (!ip) return;
			const int n = ip->GetSelNodeCount();
			sel.reserve(n);
			for (int i = 0; i < n; ++i)
				sel.push_back(ip->GetSelNode(i));
		}
	}

	const std::vector<INode*>& W3DExportSettingsDlg::ResolveSelection(bool fromFloater) const
	{
		if (fromFloater && !s_ActiveInstance)
		{
			FillSelectionFromCore(s_FloaterSelection);
			return s_FloaterSelection;
		}
		return m_Utilities.SelectedNodes();
	}

	void W3DExportSettingsDlg::OnSelectionChangedNotify(void* /*param*/, NotifyInfo* /*info*/)
	{
		// Only matters while the floater is open AND no utility panel is feeding
		// us SelectionSetChanged() calls — otherwise W3DUtilities::SelectionSetChanged
		// already drives RefreshUI() on its own.
		if (!s_FloaterHWND) return;
		if (s_ActiveInstance) return;
		FillSelectionFromCore(s_FloaterSelection);
		StaticRefreshDialogUI(s_FloaterHWND, s_FloaterSelectEdit,
		                      s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner,
		                      s_FloaterDazzleType, s_FloaterSelection);
	}

	float GetScreenSizeFromNode(INode* node)
	{
#ifdef _DEBUG
		MSTR buf;
		node->GetUserPropBuffer(buf);
#endif
		// NOTE: GetUserPropFloat is *not* usable in any way since it parses the string using the system locale,
		//        which will break when reading a value saved on other systems. std::from_chars is locale independent.

		MSTR size_str(_M("1.0f"));
		if (!node->GetUserPropString(_M("MaxScreenSize"), size_str))
			return 1.0f;

		// This is just for files that were previously saved with a locale using ',' as decimal separator. Future files will not have this problem.
		// We need a plain ASCII char copy anyways for std::from_chars.
		StringClass size_str_fixed(size_str.data(), true);

		bool fixup = false;
		for (int i = 0; i < size_str_fixed.Get_Length(); i++)
		{
			if (size_str_fixed[i] == ',') {
				size_str_fixed[i] = '.';
				fixup = true;
				break;
			}
		}

		float size = 1.0f;
		std::from_chars_result res = std::from_chars(size_str_fixed.Peek_Buffer(), size_str_fixed.Peek_Buffer() + size_str_fixed.Get_Length(), size);
		(void)res;
		return size;
	}


	W3DExportSettingsDlg::W3DExportSettingsDlg(W3DUtilities& utilities)
		: m_Utilities(utilities)
		  , m_RollupRoot(nullptr)
		  , m_DialogRoot(nullptr)
		  , m_DazzleType(nullptr)
		  , m_SelectionEdit(nullptr)
		  , m_StaticSortingSpinner(nullptr)
		  , m_ScreenSizeSpinner(nullptr)
		  , m_CommandSource(nullptr)
	{
		s_ActiveInstance = this;
		// If the floater survived the previous deactivation, refresh it immediately
		// so it reflects the current selection in the new utility session.
		if (s_FloaterHWND)
			StaticRefreshDialogUI(s_FloaterHWND, s_FloaterSelectEdit,
			                s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner,
			                s_FloaterDazzleType, m_Utilities.SelectedNodes());
	}

	void W3DExportSettingsDlg::Initialise(Interface * ip)
	{
		if (m_RollupRoot == nullptr)
		{
			m_RollupRoot = ip->AddRollupPage(hInstance, MAKEINTRESOURCE(IDD_W3D_UTIL_EXPORT_SETTINGS), DlgProc, L"W3D Export Settings", reinterpret_cast<LPARAM>(this));

			if (m_DialogRoot)
			{
				RefreshUI();
			}
		}
	}

	void W3DExportSettingsDlg::ConnectControls(HWND dialogRoot)
	{
		m_DialogRoot = dialogRoot;

		m_DazzleType = GetDlgItem(m_DialogRoot, IDC_DAZZLE_MODE);
		for (const TSTR& str : DazzleStrings())
		{
			ComboBox_AddString(m_DazzleType, str.data());
		}

		m_SelectionEdit = GetDlgItem(m_DialogRoot, IDC_SELECTED_EDIT);

		m_StaticSortingSpinner = SetupIntSpinner(m_DialogRoot, IDC_STATIC_SORT_LEVEL_SPIN, IDC_STATIC_SORT_LEVEL_EDIT, 0, 32, 0);
		m_ScreenSizeSpinner = SetupFloatSpinner(m_DialogRoot, IDC_SCREEN_SPIN, IDC_SCREEN_EDIT, 0, FLT_MAX, 0);
	}

	void W3DExportSettingsDlg::ReleaseControls()
	{
		ReleaseISpinner(m_StaticSortingSpinner);
		ReleaseISpinner(m_ScreenSizeSpinner);
	}

	W3DExportSettingsDlg::~W3DExportSettingsDlg()
	{
		if (s_ActiveInstance == this)
			s_ActiveInstance = nullptr;
		// The floater window is static-lifetime — don't destroy it here.
		// CloseFloater() is only called when Max itself shuts down (plugin unload).
		// Re-seed the floater UI from CoreInterface now that no instance is
		// feeding it selection updates, so it doesn't keep showing stale state.
		if (s_FloaterHWND)
			RefreshFloaterFromCore();
	}

	void W3DExportSettingsDlg::Close(Interface* ip)
	{
		if (m_RollupRoot)
			ip->DeleteRollupPage(m_RollupRoot);
	}

	void W3DExportSettingsDlg::OpenFloater()
	{
		if (s_FloaterHWND)
		{
			SetForegroundWindow(s_FloaterHWND);
			return;
		}
		// s_FloaterHWND is set inside WM_INITDIALOG of FloaterDlgProc.
		CreateDialogParam(
			hInstance,
			MAKEINTRESOURCE(IDD_W3D_UTIL_SETTINGS_FLOATER),
			GetCOREInterface()->GetMAXHWnd(),
			FloaterDlgProc,
			reinterpret_cast<LPARAM>(this));
		if (s_FloaterHWND)
			ShowWindow(s_FloaterHWND, SW_SHOW);
	}

	void W3DExportSettingsDlg::CloseFloater()
	{
		if (s_FloaterHWND)
			DestroyWindow(s_FloaterHWND);
		// s_FloaterHWND cleared by WM_DESTROY in FloaterDlgProc.
	}

	INT_PTR W3DExportSettingsDlg::DlgProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		W3DExportSettingsDlg* dlg = reinterpret_cast<W3DExportSettingsDlg*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		switch (message)
		{
		case WM_INITDIALOG:
			dlg = reinterpret_cast<W3DExportSettingsDlg*>(lparam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, lparam);
			dlg->ConnectControls(hWnd);
			return TRUE;
		case WM_DESTROY:
			dlg->ReleaseControls();
			return TRUE;
		case WM_COMMAND:
			dlg->m_CommandSource = hWnd;
			return dlg->HandleCommand(LOWORD(wparam), HIWORD(wparam));
		case CC_SPINNER_CHANGE:
			dlg->m_CommandSource = hWnd;
			return dlg->HandleSpinner(LOWORD(wparam));
		}
		return FALSE;
	}

	INT_PTR W3DExportSettingsDlg::FloaterDlgProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		switch (message)
		{
		case WM_INITDIALOG:
			s_FloaterHWND = hWnd;
			if (s_ActiveInstance)
			{
				s_ActiveInstance->ConnectFloaterControls(hWnd);
			}
			else
			{
				// Cold-open path: no W3DUtilities instance is alive. Wire up
				// child controls statically and seed from the current scene.
				s_FloaterDazzleType = GetDlgItem(hWnd, IDC_DAZZLE_MODE);
				s_FloaterSelectEdit = GetDlgItem(hWnd, IDC_SELECTED_EDIT);
				for (const TSTR& str : DazzleStrings())
					ComboBox_AddString(s_FloaterDazzleType, str.data());
				s_FloaterStaticSortingSpinner = SetupIntSpinner(hWnd, IDC_STATIC_SORT_LEVEL_SPIN, IDC_STATIC_SORT_LEVEL_EDIT, 0, 32, 0);
				s_FloaterScreenSizeSpinner    = SetupFloatSpinner(hWnd, IDC_SCREEN_SPIN, IDC_SCREEN_EDIT, 0, FLT_MAX, 0);
				FillSelectionFromCore(s_FloaterSelection);
				StaticRefreshDialogUI(hWnd, s_FloaterSelectEdit,
				                      s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner,
				                      s_FloaterDazzleType, s_FloaterSelection);
			}
			// Listen for selection changes outside the W3D Utility panel so the
			// floater can refresh itself when the user picks different nodes.
			if (!s_FloaterNotifyRegistered)
			{
				RegisterNotification(&W3DExportSettingsDlg::OnSelectionChangedNotify, nullptr, NOTIFY_SELECTIONSET_CHANGED);
				s_FloaterNotifyRegistered = true;
			}
			return TRUE;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			return TRUE;
		case WM_DESTROY:
			if (s_FloaterNotifyRegistered)
			{
				UnRegisterNotification(&W3DExportSettingsDlg::OnSelectionChangedNotify, nullptr, NOTIFY_SELECTIONSET_CHANGED);
				s_FloaterNotifyRegistered = false;
			}
			// Release spinner wrappers directly — no instance pointer needed.
			ReleaseISpinner(s_FloaterStaticSortingSpinner); s_FloaterStaticSortingSpinner = nullptr;
			ReleaseISpinner(s_FloaterScreenSizeSpinner);    s_FloaterScreenSizeSpinner    = nullptr;
			s_FloaterDazzleType = nullptr;
			s_FloaterSelectEdit = nullptr;
			s_FloaterHWND       = nullptr;
			s_FloaterSelection.clear();
			return TRUE;
		case WM_COMMAND:
			if (s_ActiveInstance)
			{
				s_ActiveInstance->m_CommandSource = hWnd;
				return s_ActiveInstance->HandleCommand(LOWORD(wparam), HIWORD(wparam));
			}
			return HandleFloaterCommand(hWnd, LOWORD(wparam), HIWORD(wparam));
		case CC_SPINNER_CHANGE:
			if (s_ActiveInstance)
			{
				s_ActiveInstance->m_CommandSource = hWnd;
				return s_ActiveInstance->HandleSpinner(LOWORD(wparam));
			}
			return HandleFloaterSpinner(hWnd, LOWORD(wparam));
		}
		return FALSE;
	}

	void W3DExportSettingsDlg::ConnectFloaterControls(HWND hWnd)
	{
		s_FloaterDazzleType = GetDlgItem(hWnd, IDC_DAZZLE_MODE);
		s_FloaterSelectEdit = GetDlgItem(hWnd, IDC_SELECTED_EDIT);
		for (const TSTR& str : DazzleStrings())
			ComboBox_AddString(s_FloaterDazzleType, str.data());

		s_FloaterStaticSortingSpinner = SetupIntSpinner(hWnd, IDC_STATIC_SORT_LEVEL_SPIN, IDC_STATIC_SORT_LEVEL_EDIT, 0, 32, 0);
		s_FloaterScreenSizeSpinner    = SetupFloatSpinner(hWnd, IDC_SCREEN_SPIN, IDC_SCREEN_EDIT, 0, FLT_MAX, 0);

		StaticRefreshDialogUI(hWnd, s_FloaterSelectEdit,
		                s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner,
		                s_FloaterDazzleType, m_Utilities.SelectedNodes());
	}

	void W3DExportSettingsDlg::ReleaseFloaterControls()
	{
		ReleaseISpinner(s_FloaterStaticSortingSpinner); s_FloaterStaticSortingSpinner = nullptr;
		ReleaseISpinner(s_FloaterScreenSizeSpinner);    s_FloaterScreenSizeSpinner    = nullptr;
		s_FloaterDazzleType = nullptr;
		s_FloaterSelectEdit = nullptr;
	}

	INT_PTR W3DExportSettingsDlg::HandleCommand(uint16 controlID, uint16 commandID)
	{
		const bool fromFloater = (m_CommandSource == s_FloaterHWND);
		const std::vector<INode*>& sel = ResolveSelection(fromFloater);

		switch (commandID)
		{
		case BN_CLICKED:
		{
			switch (controlID)
			{
			case IDC_CREATE_SETTINGS_FLOATER:
				OpenFloater();
				return TRUE;

				//Geometry Type Radio Buttons
			case IDC_GEOM_NORMAL:
				SetGeometryType(W3DGeometryType::Normal, sel);
				return TRUE;
			case IDC_GEOM_CAM_PARAL:
				SetGeometryType(W3DGeometryType::CamParal, sel);
				return TRUE;
			case IDC_GEOM_CAM_ORIENT:
				SetGeometryType(W3DGeometryType::CamOrient, sel);
				return TRUE;
			case IDC_GEOM_AABOX:
				SetGeometryType(W3DGeometryType::AABox, sel);
				return TRUE;
			case IDC_GEOM_OBBOX:
				SetGeometryType(W3DGeometryType::OBBox, sel);
				return TRUE;
			case IDC_GEOM_NULL_LOD:
				SetGeometryType(W3DGeometryType::NullLOD, sel);
				return TRUE;
			case IDC_GEOM_AGGREGATE:
				SetGeometryType(W3DGeometryType::Aggregate, sel);
				return TRUE;
			case IDC_GEOM_DAZZLE:
				SetGeometryType(W3DGeometryType::Dazzle, sel);
				return TRUE;
			case IDC_GEOM_CAMERA_Z_ORIENTED:
				SetGeometryType(W3DGeometryType::CamZOrient, sel);
				return TRUE;
			case IDC_GEOM_LIGHT:
				SetGeometryType(W3DGeometryType::Light, sel);
				return TRUE;

				//Export Flags
			case IDC_EXPORT_GEOMETRY:
				ModifyExportFlags(W3DExportFlags::ExportGeometry, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_EXPORT_TRANSFORM:
				ModifyExportFlags(W3DExportFlags::ExportTransform, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_STATIC_SORTING:
				if (IsDlgButtonChecked(m_CommandSource, controlID))
				{
					SetStaticSortLevel(1, sel);
				}
				else
				{
					SetStaticSortLevel(0, sel);
				}
				return TRUE;

				//Geometry Flags
			case IDC_TWO_SIDED:
				ModifyGeometryFlags(W3DGeometryFlags::TwoSided, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_HIDE:
				ModifyGeometryFlags(W3DGeometryFlags::Hide, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_Z_NORMAL:
				ModifyGeometryFlags(W3DGeometryFlags::ZNormal, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_KEEP_NORMAL:
				ModifyGeometryFlags(W3DGeometryFlags::KeepNml, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_VALPHA:
				ModifyGeometryFlags(W3DGeometryFlags::VAlpha, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_SHADOW:
				ModifyGeometryFlags(W3DGeometryFlags::Shadow, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_SHATTER:
				ModifyGeometryFlags(W3DGeometryFlags::Shatter, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_TANGENTS:
				ModifyGeometryFlags(W3DGeometryFlags::Tangents, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_PRELIT:
				ModifyGeometryFlags(W3DGeometryFlags::Prelit, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_ALWAYSDYNLIGHT:
				ModifyGeometryFlags(W3DGeometryFlags::AlwaysDynLight, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;

				// Collision Flags
			case IDC_PHYSICAL:
				ModifyCollisionFlags(W3DCollisionFlags::Physical, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_PROJECTILE:
				ModifyCollisionFlags(W3DCollisionFlags::Projectile, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_VEHICLE:
				ModifyCollisionFlags(W3DCollisionFlags::Vehicle, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_VIS:
				ModifyCollisionFlags(W3DCollisionFlags::Vis, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			case IDC_CAMERA:
				ModifyCollisionFlags(W3DCollisionFlags::Camera, IsDlgButtonChecked(m_CommandSource, controlID), sel);
				return TRUE;
			}
			break;
		} //BN_Clicked
		case CBN_SELCHANGE:
		{
			switch (controlID)
			{
			case IDC_DAZZLE_MODE:
			{
				HWND combo = fromFloater ? s_FloaterDazzleType : m_DazzleType;
				SetDazzleType(DazzleStrings()[ComboBox_GetCurSel(combo)], sel);
				return TRUE;
			}
			} //switch (controlID)
			break;
		} //CBN_SELCHANGE
		}

		return FALSE;
	}

	INT_PTR W3DExportSettingsDlg::HandleSpinner(uint16 controlID)
	{
		const bool fromFloater = (m_CommandSource == s_FloaterHWND);
		const std::vector<INode*>& sel = ResolveSelection(fromFloater);
		switch (controlID)
		{
		case IDC_STATIC_SORT_LEVEL_SPIN:
		{
			ISpinnerControl* spin = fromFloater ? s_FloaterStaticSortingSpinner : m_StaticSortingSpinner;
			if (spin) SetStaticSortLevel(spin->GetIVal(), sel);
			return TRUE;
		}
		case IDC_SCREEN_SPIN:
		{
			ISpinnerControl* spin = fromFloater ? s_FloaterScreenSizeSpinner : m_ScreenSizeSpinner;
			if (spin) SetScreenSize(spin->GetFVal(), sel);
			return TRUE;
		}
		}

		return FALSE;
	}

	INT_PTR W3DExportSettingsDlg::HandleFloaterCommand(HWND src, uint16 controlID, uint16 commandID)
	{
		FillSelectionFromCore(s_FloaterSelection);
		const std::vector<INode*>& sel = s_FloaterSelection;

		switch (commandID)
		{
		case BN_CLICKED:
		{
			switch (controlID)
			{
			case IDC_GEOM_NORMAL:           SetGeometryType(W3DGeometryType::Normal,     sel); return TRUE;
			case IDC_GEOM_CAM_PARAL:        SetGeometryType(W3DGeometryType::CamParal,   sel); return TRUE;
			case IDC_GEOM_CAM_ORIENT:       SetGeometryType(W3DGeometryType::CamOrient,  sel); return TRUE;
			case IDC_GEOM_AABOX:            SetGeometryType(W3DGeometryType::AABox,      sel); return TRUE;
			case IDC_GEOM_OBBOX:            SetGeometryType(W3DGeometryType::OBBox,      sel); return TRUE;
			case IDC_GEOM_NULL_LOD:         SetGeometryType(W3DGeometryType::NullLOD,    sel); return TRUE;
			case IDC_GEOM_AGGREGATE:        SetGeometryType(W3DGeometryType::Aggregate,  sel); return TRUE;
			case IDC_GEOM_DAZZLE:           SetGeometryType(W3DGeometryType::Dazzle,     sel); return TRUE;
			case IDC_GEOM_CAMERA_Z_ORIENTED:SetGeometryType(W3DGeometryType::CamZOrient, sel); return TRUE;
			case IDC_GEOM_LIGHT:            SetGeometryType(W3DGeometryType::Light,      sel); return TRUE;

			case IDC_EXPORT_GEOMETRY:  ModifyExportFlags(W3DExportFlags::ExportGeometry,  IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_EXPORT_TRANSFORM: ModifyExportFlags(W3DExportFlags::ExportTransform, IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_STATIC_SORTING:   SetStaticSortLevel(IsDlgButtonChecked(src, controlID) ? 1 : 0, sel); return TRUE;

			case IDC_TWO_SIDED:      ModifyGeometryFlags(W3DGeometryFlags::TwoSided,       IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_HIDE:           ModifyGeometryFlags(W3DGeometryFlags::Hide,           IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_Z_NORMAL:       ModifyGeometryFlags(W3DGeometryFlags::ZNormal,        IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_KEEP_NORMAL:    ModifyGeometryFlags(W3DGeometryFlags::KeepNml,        IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_VALPHA:         ModifyGeometryFlags(W3DGeometryFlags::VAlpha,         IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_SHADOW:         ModifyGeometryFlags(W3DGeometryFlags::Shadow,         IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_SHATTER:        ModifyGeometryFlags(W3DGeometryFlags::Shatter,        IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_TANGENTS:       ModifyGeometryFlags(W3DGeometryFlags::Tangents,       IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_PRELIT:         ModifyGeometryFlags(W3DGeometryFlags::Prelit,         IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_ALWAYSDYNLIGHT: ModifyGeometryFlags(W3DGeometryFlags::AlwaysDynLight, IsDlgButtonChecked(src, controlID), sel); return TRUE;

			case IDC_PHYSICAL:   ModifyCollisionFlags(W3DCollisionFlags::Physical,   IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_PROJECTILE: ModifyCollisionFlags(W3DCollisionFlags::Projectile, IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_VEHICLE:    ModifyCollisionFlags(W3DCollisionFlags::Vehicle,    IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_VIS:        ModifyCollisionFlags(W3DCollisionFlags::Vis,        IsDlgButtonChecked(src, controlID), sel); return TRUE;
			case IDC_CAMERA:     ModifyCollisionFlags(W3DCollisionFlags::Camera,     IsDlgButtonChecked(src, controlID), sel); return TRUE;
			}
			break;
		}
		case CBN_SELCHANGE:
		{
			if (controlID == IDC_DAZZLE_MODE)
			{
				SetDazzleType(DazzleStrings()[ComboBox_GetCurSel(s_FloaterDazzleType)], sel);
				return TRUE;
			}
			break;
		}
		}
		return FALSE;
	}

	INT_PTR W3DExportSettingsDlg::HandleFloaterSpinner(HWND /*src*/, uint16 controlID)
	{
		FillSelectionFromCore(s_FloaterSelection);
		const std::vector<INode*>& sel = s_FloaterSelection;

		switch (controlID)
		{
		case IDC_STATIC_SORT_LEVEL_SPIN:
			if (s_FloaterStaticSortingSpinner) SetStaticSortLevel(s_FloaterStaticSortingSpinner->GetIVal(), sel);
			return TRUE;
		case IDC_SCREEN_SPIN:
			if (s_FloaterScreenSizeSpinner) SetScreenSize(s_FloaterScreenSizeSpinner->GetFVal(), sel);
			return TRUE;
		}
		return FALSE;
	}

	struct W3DExportFlagsStruct
	{
		int ExportBone;
		int ExportGeometry;
		bool GeometryType[size_t(W3D::MaxTools::W3DGeometryType::Num)];
		int GeometryFlags[10];
		int CollisionFlags[5];
		int StaticSortLevel;
		int DazzleCount;
		char DazzleTypeName[128];
	};

	void W3DExportSettingsDlg::GetW3DExportFlags(W3DExportFlagsStruct *str, const std::vector<INode*>& selection)
	{
		str->ExportBone = 0;
		str->ExportGeometry = 0;
		for (int i = 0; i < ARRAYSIZE(str->GeometryFlags); i++)
		{
			str->GeometryFlags[i] = 0;
		}
		for (int i = 0; i < ARRAYSIZE(str->GeometryType); i++)
		{
			str->GeometryType[i] = false;
		}
		for (int i = 0; i < ARRAYSIZE(str->CollisionFlags); i++)
		{
			str->CollisionFlags[i] = 0;
		}
		str->StaticSortLevel = 0;
		str->DazzleCount = 0;
		if (selection.empty())
		{
			strcpy(str->DazzleTypeName, "DEFAULT");
		}
		else
		{
			const char *dazzle = W3DUtilities::GetDazzleTypeFromAppData(selection[0]);
			if (!dazzle)
			{
				strcpy(str->DazzleTypeName, "DEFAULT");
			}
			else
			{
				strcpy(str->DazzleTypeName, dazzle);
			}
		}
		for (int i = 0;i < selection.size();i++)
		{
			INode* curNode = selection[i];
			const W3DAppDataChunk &w3dData = W3DUtilities::GetOrCreateW3DAppDataChunk(*curNode);
			str->ExportBone += enum_has_flags(w3dData.ExportFlags, W3DExportFlags::ExportTransform);
			str->ExportGeometry += enum_has_flags(w3dData.ExportFlags, W3DExportFlags::ExportGeometry);
			for (int f = 0; f < GeometryFlagCheckIDs().size(); ++f)
			{
				str->GeometryFlags[f] += (((1 << f) & enum_to_value(w3dData.GeometryFlags)) == (1 << f));
			}
			for (int f = 0; f < CollisionFlagCheckIDs().size(); ++f)
			{
				str->CollisionFlags[f] += (((1 << f) & enum_to_value(w3dData.CollisionFlags)) == (1 << f));
			}
			int geoType = enum_to_value(w3dData.GeometryType);

			// this is a fix for objects that somehow have a Geometry Type of 0. (These tend to be things imported from the Max 8 plugin, for some reason)
			if (geoType == 0)
				geoType = 2;

			str->GeometryType[geoType - 1] = true;
			if (w3dData.GeometryType == W3DGeometryType::Dazzle)
			{
				str->DazzleCount += 1;
			}
			if (i)
			{
				if (str->StaticSortLevel != w3dData.StaticSortLevel)
				{
					str->StaticSortLevel = -1;
				}
			}
			else
			{
				str->StaticSortLevel = w3dData.StaticSortLevel;
			}
			if (!W3DUtilities::GetDazzleTypeFromAppData(curNode) || strcmp(str->DazzleTypeName, W3DUtilities::GetDazzleTypeFromAppData(curNode)))
			{
				strcpy(str->DazzleTypeName, "DEFAULT");
			}
		}
		int count = (int)selection.size();
		if (str->ExportBone)
		{
			str->ExportBone = (str->ExportBone != count) + 1;
		}
		else
		{
			str->ExportBone = 0;
		}
		if (str->ExportGeometry)
		{
			str->ExportGeometry = (str->ExportGeometry != count) + 1;
		}
		else
		{
			str->ExportGeometry = 0;
		}
		for (int f = 0; f < GeometryFlagCheckIDs().size(); ++f)
		{
			if (str->GeometryFlags[f])
			{
				str->GeometryFlags[f] = (str->GeometryFlags[f] != count) + 1;
			}
			else
			{
				str->GeometryFlags[f] = 0;
			}
		}
		for (int f = 0; f < CollisionFlagCheckIDs().size(); ++f)
		{
			if (str->CollisionFlags[f])
			{
				str->CollisionFlags[f] = (str->CollisionFlags[f] != count) + 1;
			}
			else
			{
				str->CollisionFlags[f] = 0;
			}
		}
	}

	void W3DExportSettingsDlg::RefreshUI()
	{
		if (m_DialogRoot)
			StaticRefreshDialogUI(m_DialogRoot, m_SelectionEdit,
			                m_StaticSortingSpinner, m_ScreenSizeSpinner, m_DazzleType,
			                m_Utilities.SelectedNodes());
		if (s_FloaterHWND)
			StaticRefreshDialogUI(s_FloaterHWND, s_FloaterSelectEdit,
			                s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner, s_FloaterDazzleType,
			                m_Utilities.SelectedNodes());
	}

	void W3DExportSettingsDlg::RefreshAllUI()
	{
		if (s_ActiveInstance)
		{
			s_ActiveInstance->RefreshUI();
		}
		else if (s_FloaterHWND)
		{
			FillSelectionFromCore(s_FloaterSelection);
			StaticRefreshDialogUI(s_FloaterHWND, s_FloaterSelectEdit,
			                s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner,
			                s_FloaterDazzleType, s_FloaterSelection);
		}
	}

	void W3DExportSettingsDlg::RefreshFloaterFromCore()
	{
		if (!s_FloaterHWND) return;
		FillSelectionFromCore(s_FloaterSelection);
		StaticRefreshDialogUI(s_FloaterHWND, s_FloaterSelectEdit,
		                s_FloaterStaticSortingSpinner, s_FloaterScreenSizeSpinner,
		                s_FloaterDazzleType, s_FloaterSelection);
	}

	void W3DExportSettingsDlg::StaticRefreshDialogUI(HWND root, HWND selEdit,
	    ISpinnerControl* sortSpin, ISpinnerControl* screenSpin, HWND dazzleCombo,
	    const std::vector<INode*>& selection)
	{
		HWND m_DialogRoot         = root;
		HWND m_SelectionEdit      = selEdit;
		ISpinnerControl* m_StaticSortingSpinner = sortSpin;
		ISpinnerControl* m_ScreenSizeSpinner    = screenSpin;
		HWND m_DazzleType         = dazzleCombo;

		EnableWindow(GetDlgItem(m_DialogRoot, IDC_SELECTED_EDIT), FALSE);
		if (selection.empty())
		{
			SetWindowText(m_SelectionEdit, L"( Nothing Selected )");
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_EXPORT_TRANSFORM), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_EXPORT_GEOMETRY), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_EDIT), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_SPIN), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_LABEL), FALSE);
			for (int i = 0; i < GeometryTypeRadioIDs().size(); i++)
			{
				EnableWindow(GetDlgItem(m_DialogRoot, GeometryTypeRadioIDs()[i]), FALSE);
			}
			for (int i = 0; i < GeometryFlagCheckIDs().size(); i++)
			{
				EnableWindow(GetDlgItem(m_DialogRoot, GeometryFlagCheckIDs()[i]), FALSE);
			}
			for (int i = 0; i < CollisionFlagCheckIDs().size(); i++)
			{
				EnableWindow(GetDlgItem(m_DialogRoot, CollisionFlagCheckIDs()[i]), FALSE);
			}
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORTING), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORT_LEVEL_LABEL), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORT_LEVEL_EDIT), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORT_LEVEL_SPIN), FALSE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_DAZZLE_MODE), FALSE);
			CheckDlgButton(m_DialogRoot, IDC_EXPORT_TRANSFORM, BST_UNCHECKED);
			CheckDlgButton(m_DialogRoot, IDC_EXPORT_GEOMETRY, BST_UNCHECKED);
			for (int i = 0; i < GeometryTypeRadioIDs().size(); i++)
			{
				CheckDlgButton(m_DialogRoot, GeometryTypeRadioIDs()[i], BST_UNCHECKED);
			}
			for (int i = 0; i < GeometryFlagCheckIDs().size(); i++)
			{
				CheckDlgButton(m_DialogRoot, GeometryFlagCheckIDs()[i], BST_UNCHECKED);
			}
			for (int i = 0; i < CollisionFlagCheckIDs().size(); i++)
			{
				CheckDlgButton(m_DialogRoot, CollisionFlagCheckIDs()[i], BST_UNCHECKED);
			}
		}
		else
		{
			if (selection.size() == 1)
			{
				SetWindowText(m_SelectionEdit, selection.front()->GetName());
			}
			else
			{
				std::wstringstream selection_text;
				selection_text << selection.size() << L" - Objects Selected";
				SetWindowText(m_SelectionEdit, selection_text.str().data());
			}
			W3DExportFlagsStruct flags;
			GetW3DExportFlags(&flags, selection);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_EXPORT_TRANSFORM), TRUE);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_EXPORT_GEOMETRY), TRUE);
			if (flags.ExportGeometry == 1)
			{
				for (int i = 0; i < GeometryTypeRadioIDs().size(); i++)
				{
					EnableWindow(GetDlgItem(m_DialogRoot, GeometryTypeRadioIDs()[i]), TRUE);
				}
				for (int i = 0; i < GeometryFlagCheckIDs().size(); i++)
				{
					EnableWindow(GetDlgItem(m_DialogRoot, GeometryFlagCheckIDs()[i]), TRUE);
				}
				for (int i = 0; i < CollisionFlagCheckIDs().size(); i++)
				{
					EnableWindow(GetDlgItem(m_DialogRoot, CollisionFlagCheckIDs()[i]), TRUE);
				}
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORTING), TRUE);
			}
			else
			{
				for (int i = 0; i < GeometryTypeRadioIDs().size(); i++)
				{
					EnableWindow(GetDlgItem(m_DialogRoot, GeometryTypeRadioIDs()[i]), FALSE);
				}
				for (int i = 0; i < GeometryFlagCheckIDs().size(); i++)
				{
					EnableWindow(GetDlgItem(m_DialogRoot, GeometryFlagCheckIDs()[i]), FALSE);
				}
				for (int i = 0; i < CollisionFlagCheckIDs().size(); i++)
				{
					EnableWindow(GetDlgItem(m_DialogRoot, CollisionFlagCheckIDs()[i]), FALSE);
				}
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORTING), FALSE);
			}
			if (flags.ExportBone && selection.size() == 1 && !flags.ExportGeometry)
			{
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_LABEL), TRUE);
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_EDIT), TRUE);
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_SPIN), TRUE);
				float size = GetScreenSizeFromNode(selection.front());
				m_ScreenSizeSpinner->SetValue(size, 0);
			}
			else
			{
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_LABEL), FALSE);
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_EDIT), FALSE);
				EnableWindow(GetDlgItem(m_DialogRoot, IDC_SCREEN_SPIN), FALSE);
				m_ScreenSizeSpinner->SetValue(0, 0);
			}
			CheckDlgButton(m_DialogRoot, IDC_EXPORT_TRANSFORM, flags.ExportBone);
			CheckDlgButton(m_DialogRoot, IDC_EXPORT_GEOMETRY, flags.ExportGeometry);
			for (int i = 0; i < GeometryFlagCheckIDs().size(); i++)
			{
				CheckDlgButton(m_DialogRoot, GeometryFlagCheckIDs()[i], flags.GeometryFlags[i]);
			}
			for (int i = 0; i < CollisionFlagCheckIDs().size(); i++)
			{
				CheckDlgButton(m_DialogRoot, CollisionFlagCheckIDs()[i], flags.CollisionFlags[i]);
			}
			bool enable = false;
			if (flags.ExportGeometry == 1)
			{
				if (flags.StaticSortLevel == -1)
				{
					m_StaticSortingSpinner->SetIndeterminate(true);
					CheckDlgButton(m_DialogRoot, IDC_STATIC_SORTING, BST_INDETERMINATE);
				}
				else
				{
					if (flags.StaticSortLevel)
					{
						CheckDlgButton(m_DialogRoot, IDC_STATIC_SORTING, BST_CHECKED);
						m_StaticSortingSpinner->SetIndeterminate(false);
						m_StaticSortingSpinner->SetValue(flags.StaticSortLevel, 0);
						enable = true;
					}
					else
					{
						CheckDlgButton(m_DialogRoot, IDC_STATIC_SORTING, BST_UNCHECKED);
						m_StaticSortingSpinner->SetValue(0, 0);
					}
				}
			}
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORT_LEVEL_LABEL), enable);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORT_LEVEL_EDIT), enable);
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_STATIC_SORT_LEVEL_SPIN), enable);
			bool dazzle = false;
			if (flags.ExportGeometry == 1 && flags.DazzleCount == selection.size())
			{
				dazzle = true;
			}
			EnableWindow(GetDlgItem(m_DialogRoot, IDC_DAZZLE_MODE), dazzle);
			WideStringClass dazzleType = flags.DazzleTypeName;
			ComboBox_SetCurSel(m_DazzleType, IndexOfDazzleString(dazzleType));
			for (unsigned int i = 0; i < GeometryTypeRadioIDs().size(); i++)
			{
				CheckDlgButton(m_DialogRoot, GeometryTypeRadioIDs()[i], flags.GeometryType[i]);
			}
		}
	}

	void W3DExportSettingsDlg::SetScreenSize(float size, const std::vector<INode*>& selection)
	{
		if (selection.size() == 1)
		{
			// NOTE: SetUserPropFloat is *not* usable in any way since it prints the value using the system locale,
			//        which will break reading the value back on other systems. std::to_chars is locale independent.
			constexpr size_t buf_size = 78;
			char buf[buf_size];
			std::to_chars_result res = std::to_chars(buf, buf + buf_size, size);
			*res.ptr = '\0'; // to_chars does not null terminate!

			MSTR str = MSTR::FromCStr(buf);
			selection.front()->SetUserPropString(_M("MaxScreenSize"), str);
#ifdef _DEBUG
			MSTR userpropbuf;
			selection.front()->GetUserPropBuffer(userpropbuf);
#endif
		}
	}

	void W3DExportSettingsDlg::SetDazzleType(const TSTR& dazzle, const std::vector<INode*>& selection)
	{
		for (INode* node : selection)
		{
			CStr str = dazzle.ToCStr();
			W3DUtilities::SetDazzleTypeInAppData(node, str);
		}
		RefreshAllUI();
	}

	void W3DExportSettingsDlg::SetGeometryType(W3DGeometryType type, const std::vector<INode*>& selection)
	{
		VisitNodeAppData(selection, [type](W3DAppDataChunk& chunk) { chunk.GeometryType = type; });
		// Dazzle combo enable/disable is part of the full refresh below.
		RefreshAllUI();
	}

	void W3DExportSettingsDlg::SetStaticSortLevel(int sortLevel, const std::vector<INode*>& selection)
	{
		VisitNodeAppData(selection, [sortLevel](W3DAppDataChunk& chunk) { chunk.StaticSortLevel = sortLevel; });
		RefreshAllUI();
	}

	void W3DExportSettingsDlg::ModifyExportFlags(W3DExportFlags flags, bool add, const std::vector<INode*>& selection)
	{
		VisitNodeAppData(selection,
			[add, flags](W3DAppDataChunk& chunk)
		{
			chunk.ExportFlags = add ? (chunk.ExportFlags | flags) : (chunk.ExportFlags & ~flags);
		}
		);

		RefreshAllUI();
	}

	void W3DExportSettingsDlg::ModifyGeometryFlags(W3DGeometryFlags flags, bool add, const std::vector<INode*>& selection)
	{
		VisitNodeAppData(selection,
			[add, flags](W3DAppDataChunk& chunk)
		{
			chunk.GeometryFlags = add ? (chunk.GeometryFlags | flags) : (chunk.GeometryFlags & ~flags);
		});

		RefreshAllUI();
	}

	void W3DExportSettingsDlg::ModifyCollisionFlags(W3DCollisionFlags flags, bool add, const std::vector<INode*>& selection)
	{
		VisitNodeAppData(selection,
			[add, flags](W3DAppDataChunk& chunk)
		{
			chunk.CollisionFlags = add ? (chunk.CollisionFlags | flags) : (chunk.CollisionFlags & ~flags);
		});

		RefreshAllUI();
	}
}