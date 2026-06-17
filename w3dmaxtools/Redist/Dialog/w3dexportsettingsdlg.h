#pragma once

#include <max.h>
#include <vector>
#include "w3dappdatachunk.h"

namespace W3D::MaxTools
{
	float GetScreenSizeFromNode(INode* node);

	class W3DUtilities;
	struct W3DExportFlagsStruct;

	class W3DExportSettingsDlg
	{
	public:
		W3DExportSettingsDlg(W3DUtilities& utilities);
		~W3DExportSettingsDlg();

		void Initialise(Interface* ip);
		void Close(Interface* ip);
		void RefreshUI();

		void OpenFloater();
		void CloseFloater();
	private:
		static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
		static INT_PTR CALLBACK FloaterDlgProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);

		void ReleaseControls();
		void ConnectControls(HWND dialogRoot);
		void ConnectFloaterControls(HWND hWnd);
		void ReleaseFloaterControls();

		// Selection source. Resolves to W3DUtilities::SelectedNodes() when the
		// utility panel is active; otherwise reads the live selection from
		// GetCOREInterface(). The floater calls this so it keeps working after
		// the user switches away from the W3D Utilities tab (which destroys the
		// W3DUtilities instance).
		const std::vector<INode*>& ResolveSelection(bool fromFloater) const;

		INT_PTR HandleCommand(uint16 controlID, uint16 commandID);
		INT_PTR HandleSpinner(uint16 controlID);

		// Static counterparts used by the floater when no W3DUtilities instance
		// is alive (user switched away from the Utilities tab).
		static INT_PTR HandleFloaterCommand(HWND src, uint16 controlID, uint16 commandID);
		static INT_PTR HandleFloaterSpinner(HWND src, uint16 controlID);

		// After RefreshUI() — drives the floater's own controls from
		// s_FloaterSelection.
		static void RefreshFloaterFromCore();

		// Static implementations. None of these touch m_Utilities or any
		// per-instance state; the selection is supplied by the caller. This is
		// what lets the floater keep working after the W3DUtilities instance is
		// destroyed (user switched away from the Utilities tab).
		static void GetW3DExportFlags(W3DExportFlagsStruct *str, const std::vector<INode*>& selection);
		static void StaticRefreshDialogUI(HWND root, HWND selEdit,
		                                  ISpinnerControl* sortSpin, ISpinnerControl* screenSpin,
		                                  HWND dazzleCombo,
		                                  const std::vector<INode*>& selection);

		static void SetDazzleType(const TSTR& dazzle, const std::vector<INode*>& selection);
		static void SetGeometryType(W3DGeometryType type, const std::vector<INode*>& selection);
		static void SetStaticSortLevel(int sortLevel, const std::vector<INode*>& selection);
		static void SetScreenSize(float size, const std::vector<INode*>& selection);
		static void ModifyExportFlags(W3DExportFlags flags, bool add, const std::vector<INode*>& selection);
		static void ModifyGeometryFlags(W3DGeometryFlags flags, bool add, const std::vector<INode*>& selection);
		static void ModifyCollisionFlags(W3DCollisionFlags flags, bool add, const std::vector<INode*>& selection);

		// Refresh-after-mutate dispatcher. Static so floater handlers can call
		// it without an instance.
		static void RefreshAllUI();

		// Called by Max when the global selection changes while the floater is
		// alive but no W3DUtilities instance is active. Refreshes the floater UI.
		static void OnSelectionChangedNotify(void* param, NotifyInfo* info);

		W3DUtilities&    m_Utilities;

		// Rollup page (embedded in the Utility panel)
		HWND             m_RollupRoot;
		HWND             m_DialogRoot;
		HWND             m_DazzleType;
		HWND             m_SelectionEdit;
		ISpinnerControl* m_StaticSortingSpinner;
		ISpinnerControl* m_ScreenSizeSpinner;

		// Set by DlgProc/FloaterDlgProc before each HandleCommand/HandleSpinner call
		// so the handlers read checkbox/spinner state from the correct window.
		HWND             m_CommandSource;

		// Floater: static so it outlives individual W3DUtilities activations.
		// Max calls DeleteThis() on the utility every time the user leaves the
		// panel, so per-instance storage would close the window on every switch.
		static HWND             s_FloaterHWND;
		static HWND             s_FloaterDazzleType;
		static HWND             s_FloaterSelectEdit;
		static ISpinnerControl* s_FloaterStaticSortingSpinner;
		static ISpinnerControl* s_FloaterScreenSizeSpinner;
		// Current live instance; nullptr while the utility panel is inactive.
		static W3DExportSettingsDlg* s_ActiveInstance;
		// Selection snapshot fed from Max's NOTIFY_SELECTIONSET_CHANGED while the
		// floater is alive but the utility panel is not. Buffer is owned here so
		// ResolveSelection() can return it by const-reference like SelectedNodes().
		static std::vector<INode*> s_FloaterSelection;
		// True between RegisterNotification and UnRegisterNotification calls.
		static bool s_FloaterNotifyRegistered;
	};
}