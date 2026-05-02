#pragma once

#include <max.h>
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

		void GetW3DExportFlags(W3DExportFlagsStruct *str);
		INT_PTR HandleCommand(uint16 controlID, uint16 commandID);
		INT_PTR HandleSpinner(uint16 controlID);

		// Updates any dialog window that uses the export-settings control layout.
		void RefreshDialogUI(HWND root, HWND selEdit,
		                     ISpinnerControl* sortSpin, ISpinnerControl* screenSpin,
		                     HWND dazzleCombo);

		void SetDazzleType(const TSTR& dazzle);
		void SetGeometryType(W3DGeometryType type);
		void SetStaticSortLevel(int sortLevel);
		void SetScreenSize(float size);
		void ModifyExportFlags(W3DExportFlags flags, bool add);
		void ModifyGeometryFlags(W3DGeometryFlags flags, bool add);
		void ModifyCollisionFlags(W3DCollisionFlags flags, bool add);

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
	};
}