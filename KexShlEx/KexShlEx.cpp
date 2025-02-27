#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <InitGuid.h>
#include <OleCtl.h>
#include <WindowsX.h>
#include <stdio.h>
#include <KexComm.h>

#include "resource.h"

// {9AACA888-A5F5-4C01-852E-8A2005C1D45F}
DEFINE_GUID(CLSID_KexShlEx, 0x9aaca888, 0xa5f5, 0x4c01, 0x85, 0x2e, 0x8a, 0x20, 0x5, 0xc1, 0xd4, 0x5f);

#define CLSID_STRING_KEXSHLEX T("{9AACA888-A5F5-4C01-852E-8A2005C1D45F}")
#define APPNAME T("KexShlEx")
#define FRIENDLYAPPNAME T("VxKex Configuration Properties")
#define REPORT_BUG_URL T("https://github.com/vxiiduu/VxKex/issues")
#define DLLAPI EXTERN_C DECLSPEC_EXPORT

#ifdef _WIN64
#  define X64 TRUE
#  define X86 FALSE
#else
#  define X86 TRUE
#  define X64 FALSE
#endif

//
// GLOBAL VARIABLES
//

UINT g_cRefDll = 0;
HINSTANCE g_hInst = NULL;

// Misc function declarations

UINT WINAPI CallbackProc(
	IN		HWND				hWnd,
	IN		UINT				uMsg,
	IN OUT	LPPROPSHEETPAGE		lpPsp);

INT_PTR DialogProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam);

//
// COM INTERFACES
//

typedef interface CKexShlEx : public IShellExtInit, IShellPropSheetExt
{
private:
	INT m_cRef;

public:
	TCHAR m_szExe[MAX_PATH];

	CKexShlEx()
	{
		++g_cRefDll;
		m_cRef = 1;
		m_szExe[0] = '\0';
	}

	~CKexShlEx()
	{
		--g_cRefDll;
	}

	// IUnknown
	STDMETHODIMP QueryInterface(
		IN	REFIID	riid,
		OUT	PPVOID	ppv)
	{
		if (IsEqualIID(riid, IID_IUnknown)) {
			*ppv = this;
		} else if (IsEqualIID(riid, IID_IShellExtInit)) {
			*ppv = (LPSHELLEXTINIT) this;
		} else if (IsEqualIID(riid, IID_IShellPropSheetExt)) {
			*ppv = (LPSHELLPROPSHEETEXT) this;
		} else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef(
		VOID)
	{
		return ++m_cRef;
	}

	STDMETHODIMP_(ULONG) Release(
		VOID)
	{
		if (--m_cRef == 0) {
			delete this;
		}

		return m_cRef;
	}

	// IShellExtInit
	STDMETHODIMP Initialize(
		IN	LPCITEMIDLIST	lpidlFolder OPTIONAL,
		IN	LPDATAOBJECT	lpDataObj,
		IN	HKEY			hKeyProgID)
	{
		UINT uCount; // number of files that were selected
		FORMATETC fmt;
		STGMEDIUM med;

		if (lpDataObj == NULL) {
			return E_INVALIDARG;
		}

		// get filename of the .exe
		fmt.cfFormat	= CF_HDROP;
		fmt.ptd			= NULL;
		fmt.dwAspect	= DVASPECT_CONTENT;
		fmt.lindex		= -1;
		fmt.tymed		= TYMED_HGLOBAL;

		if (SUCCEEDED(lpDataObj->GetData(&fmt, &med))) {
			HDROP hDrop = (HDROP) med.hGlobal;
			uCount = DragQueryFile(hDrop, (UINT) -1, NULL, 0);

			// if the user selected more than one exe, don't display the property sheet
			if (uCount != 1) {
				ReleaseStgMedium(&med);
				return E_NOTIMPL;
			}

			DragQueryFile(hDrop, 0, m_szExe, ARRAYSIZE(m_szExe));
			ReleaseStgMedium(&med);
		} else {
			return E_FAIL;
		}

		// full path of the .exe file is now stored in m_szExe
		return S_OK;
	}

	// IShellPropSheetExt
	STDMETHODIMP AddPages(
		IN	LPFNADDPROPSHEETPAGE	lpfnAddPage,
		IN	LPARAM					lParam)
	{
		PROPSHEETPAGE psp;
		HPROPSHEETPAGE hPsp;

		if (m_szExe[0] == '\0') {
			return E_UNEXPECTED;
		}

		ZeroMemory(&psp, sizeof(psp));
		psp.dwSize		= sizeof(psp);
		psp.dwFlags		= PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
		psp.hInstance	= g_hInst;
		psp.pszTemplate	= MAKEINTRESOURCE(IDD_DIALOG1);
		psp.hIcon		= NULL;
		psp.pszTitle	= T("VxKex");
		psp.pfnDlgProc	= (DLGPROC) DialogProc;
		psp.pcRefParent	= &g_cRefDll;
		psp.pfnCallback	= CallbackProc;
		psp.lParam		= (LPARAM) this;

		hPsp = CreatePropertySheetPage(&psp);

		if (hPsp) {
			if (lpfnAddPage(hPsp, lParam)) {
				AddRef();
				return S_OK;
			} else {
				DestroyPropertySheetPage(hPsp);
				return E_FAIL;
			}
		} else {
			return E_OUTOFMEMORY;
		}
	}

	STDMETHODIMP ReplacePage(
		IN	UINT					uPageID,
		IN	LPFNADDPROPSHEETPAGE	lpfnReplaceWith,
		IN	LPARAM					lParam)
	{
		return E_NOTIMPL;
	}
} CKEXSHLEX, *PCKEXSHLEX, *LPCKEXSHLEX;

typedef interface CClassFactory : public IClassFactory
{
protected:
	INT m_cRef;

public:
	CClassFactory()
	{
		++g_cRefDll;
		m_cRef = 1;
	}

	~CClassFactory()
	{
		--g_cRefDll;
	}

	// IUnknown
	STDMETHODIMP QueryInterface(
		IN	REFIID	riid,
		OUT	PPVOID	ppv)
	{
		if (IsEqualIID(riid, IID_IUnknown)) {
			*ppv = this;
		} else if (IsEqualIID(riid, IID_IClassFactory)) {
			*ppv = (LPCLASSFACTORY) this;
		} else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef(
		VOID)
	{
		return ++m_cRef;
	}
	
	STDMETHODIMP_(ULONG) Release(
		VOID)
	{
		if (--m_cRef == 0) {
			delete this;
		}
		
		return m_cRef;
	}

	// IClassFactory
	STDMETHODIMP CreateInstance(
		IN	LPUNKNOWN	pUnk,
		IN	REFIID		riid,
		OUT	PPVOID		ppv)
	{
		HRESULT hr;
		LPCKEXSHLEX lpKexShlEx;

		*ppv = NULL;

		if (pUnk) {
			return CLASS_E_NOAGGREGATION;
		}

		if ((lpKexShlEx = new CKexShlEx) == NULL) {
			return E_OUTOFMEMORY;
		}

		hr = lpKexShlEx->QueryInterface(riid, ppv);
		lpKexShlEx->Release();
		return hr;
	}

	STDMETHODIMP LockServer(
		BOOL)
	{
		return E_NOTIMPL;
	}
} CCLASSFACTORY, *PCCLASSFACTORY, *LPCCLASSFACTORY;

//
// DIALOG FUNCTIONS
//

HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPTSTR	lpszText)
{
	TOOLINFO ToolInfo;
	HWND hWndTool;
	HWND hWndTip;

	if (!iToolID || !hDlg || !lpszText) {
		return NULL;
	}

	// Get the window of the tool.
	hWndTool = GetDlgItem(hDlg, iToolID);

	// Create the tooltip.
	hWndTip = CreateWindowEx(
		0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL, 
		NULL, NULL);

	if (!hWndTool || !hWndTip) {
		return NULL;
	}

	// Associate the tooltip with the tool.
	ZeroMemory(&ToolInfo, sizeof(ToolInfo));
	ToolInfo.cbSize = sizeof(ToolInfo);
	ToolInfo.hwnd = hDlg;
	ToolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ToolInfo.uId = (UINT_PTR) hWndTool;
	ToolInfo.lpszText = lpszText;
	SendMessage(hWndTip, TTM_ADDTOOL, 0, (LPARAM) &ToolInfo);
	SendMessage(hWndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM) 300);

	return hWndTip;
}

UINT WINAPI CallbackProc(
	IN		HWND				hWnd,
	IN		UINT				uMsg,
	IN OUT	LPPROPSHEETPAGE		lpPsp)
{
	if (uMsg == PSPCB_RELEASE) {
	}

	return TRUE;
}

INT_PTR DialogProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	static BOOL bSettingsChanged = FALSE;

	if (uMsg == WM_INITDIALOG) {
		DWORD dwEnableVxKex = FALSE;
		TCHAR szWinVerSpoof[6];
		DWORD dwAlwaysShowDebug = FALSE;
		DWORD dwDisableForChild = FALSE;
		DWORD dwDisableAppSpecific = FALSE;
		DWORD dwWaitForChild = FALSE;
		TCHAR szVxKexLdrPath[MAX_PATH];
		TCHAR szIfeoDebugger[MAX_PATH];
		TCHAR szIfeoKey[74 + MAX_PATH] = T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
		TCHAR szKexIfeoKey[54 + MAX_PATH] = T("SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\");
		LPCTSTR lpszExePath = ((LPCKEXSHLEX) (((LPPROPSHEETPAGE) lParam)->lParam))->m_szExe;
		TCHAR szExePath[MAX_PATH];
		TCHAR szExeName[MAX_PATH];
		CONST HWND hWndWinVer = GetDlgItem(hWnd, IDWINVERCOMBOBOX);

		// store the pointer to the name of the .EXE in the userdata of the dialog
		// for convenience purposes. These casts are confusing and horrible, but
		// it works so don't mess with it. :^)
		strcpy_s(szExePath, ARRAYSIZE(szExePath), lpszExePath);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) lpszExePath);

		// populate and set up the Windows version selector
		ComboBox_AddString(hWndWinVer, T("Windows 8"));
		ComboBox_AddString(hWndWinVer, T("Windows 8.1"));
		ComboBox_AddString(hWndWinVer, T("Windows 10"));
		ComboBox_AddString(hWndWinVer, T("Windows 11"));
		ComboBox_SetItemData(hWndWinVer, 0, T("WIN8"));
		ComboBox_SetItemData(hWndWinVer, 1, T("WIN81"));
		ComboBox_SetItemData(hWndWinVer, 2, T("WIN10"));
		ComboBox_SetItemData(hWndWinVer, 3, T("WIN11"));
		ComboBox_SetCurSel(hWndWinVer, 2); // set default selection to Windows 10

		strcat_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), szExePath);
		strcpy_s(szExeName, ARRAYSIZE(szExeName), szExePath);
		PathStripPath(szExeName);
		strcat_s(szIfeoKey, ARRAYSIZE(szIfeoKey), szExeName);
		CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath)));
		strcat_s(szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath), T("\\VxKexLdr.exe"));

		if (RegReadSz(HKEY_LOCAL_MACHINE, szIfeoKey, T("Debugger"), szIfeoDebugger, ARRAYSIZE(szIfeoDebugger)) &&
			!lstrcmp(szIfeoDebugger, szVxKexLdrPath) &&
			RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, T("EnableVxKex"), &dwEnableVxKex) && dwEnableVxKex) {
			CheckDlgButton(hWnd, IDUSEVXKEX, TRUE);
		}

		if (RegReadSz(HKEY_CURRENT_USER, szKexIfeoKey, T("WinVerSpoof"), szWinVerSpoof, ARRAYSIZE(szWinVerSpoof))) {
			if (lstrcmpi(szWinVerSpoof, T("NONE"))) {
				CheckDlgButton(hWnd, IDSPOOFVERSIONCHECK, TRUE);
				ComboBox_Enable(hWndWinVer, TRUE);
				
				if (!lstrcmpi(szWinVerSpoof, T("WIN8"))) {
					ComboBox_SetCurSel(hWndWinVer, 0);
				} else if (!lstrcmpi(szWinVerSpoof, T("WIN81"))) {
					ComboBox_SetCurSel(hWndWinVer, 1);
				} else if (!lstrcmpi(szWinVerSpoof, T("WIN10"))) {
					ComboBox_SetCurSel(hWndWinVer, 2);
				} else if (!lstrcmpi(szWinVerSpoof, T("WIN11"))) {
					ComboBox_SetCurSel(hWndWinVer, 3);
				}
			}
		}

		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, T("AlwaysShowDebug"), &dwAlwaysShowDebug);
		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, T("DisableForChild"), &dwDisableForChild);
		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, T("DisableAppSpecific"), &dwDisableAppSpecific);
		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, T("WaitForChild"), &dwWaitForChild);

		CheckDlgButton(hWnd, IDALWAYSSHOWDEBUG, dwAlwaysShowDebug);
		CheckDlgButton(hWnd, IDDISABLEFORCHILD, dwDisableForChild);
		CheckDlgButton(hWnd, IDDISABLEAPPSPECIFIC, dwDisableAppSpecific);
		CheckDlgButton(hWnd, IDWAITONCHILD, dwWaitForChild);

		PathRemoveFileSpec(szVxKexLdrPath);

		if (PathIsPrefix(szVxKexLdrPath, szExePath)) {
			// Discourage the user from enabling VxKex for VxKex executables, since they certainly
			// don't need it and it could cause major problems (especially for VxKexLdr.exe itself)
			EnableWindow(GetDlgItem(hWnd, IDUSEVXKEX), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDSPOOFVERSIONCHECK), FALSE);
			EnableWindow(hWndWinVer, FALSE);
			EnableWindow(GetDlgItem(hWnd, IDALWAYSSHOWDEBUG), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDDISABLEFORCHILD), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDDISABLEAPPSPECIFIC), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDWAITONCHILD), FALSE);
		}

		CreateToolTip(hWnd, IDUSEVXKEX, T("Enable or disable the main VxKex compatibility layer."));
		CreateToolTip(hWnd, IDSPOOFVERSIONCHECK, T("Some applications check the Windows version and refuse to run if it is incorrect. ")
												 T("This option can help these applications to run correctly."));
		CreateToolTip(hWnd, IDALWAYSSHOWDEBUG, T("Creates a console window and displays some additional ")
											   T("information which may be useful for troubleshooting."));
		CreateToolTip(hWnd, IDDISABLEFORCHILD, T("By default, if this program launches another program, VxKex will be enabled ")
											   T("for the second program and all subsequent programs which are launched. This option ")
											   T("disables such behavior."));
		CreateToolTip(hWnd, IDDISABLEAPPSPECIFIC, T("Disable application-specific code. This is mainly useful for VxKex developers. ")
												  T("Usually, enabling this option will degrade application compatibility."));
		CreateToolTip(hWnd, IDWAITONCHILD, T("Wait for the child process to exit before exiting the loader. This is useful in combination ")
										   T("with the 'Always show debugging information' option which lets you view debug strings. It ")
										   T("may also be useful to support programs that wait for child processes to exit."));
		CreateToolTip(hWnd, IDREPORTBUG, REPORT_BUG_URL);

		SetFocus(GetDlgItem(hWnd, IDUSEVXKEX));
		return TRUE;
	} else if (uMsg == WM_COMMAND) {
		if (LOWORD(wParam) == IDSPOOFVERSIONCHECK) {
			EnableWindow(GetDlgItem(hWnd, IDWINVERCOMBOBOX), !!IsDlgButtonChecked(hWnd, IDSPOOFVERSIONCHECK));
		}
		
		// enable the "Apply" button if it's not already enabled
		PropSheet_Changed(GetParent(hWnd), hWnd);
		bSettingsChanged = TRUE;

		return TRUE;
	} else if (uMsg == WM_NOTIFY && ((LPNMHDR) lParam)->code == PSN_APPLY && bSettingsChanged) {
		// OK or Apply button was clicked and we need to apply new settings

		HWND hWndWinVer = GetDlgItem(hWnd, IDWINVERCOMBOBOX);
		STARTUPINFO startupInfo;
		PROCESS_INFORMATION procInfo;
		LPCTSTR lpszExeFullPath = (LPCTSTR) GetWindowLongPtr(hWnd, GWLP_USERDATA);
		TCHAR szKexCfgFullPath[MAX_PATH];
		TCHAR szKexCfgCmdLine[542];

		CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexCfgFullPath, ARRAYSIZE(szKexCfgFullPath)));
		strcat_s(szKexCfgFullPath, MAX_PATH, T("\\KexCfg.exe"));

		sprintf_s(szKexCfgCmdLine, ARRAYSIZE(szKexCfgCmdLine), T("\"%s\" \"%s\" %d \"%s\" %d %d %d %d"),
				  szKexCfgFullPath, lpszExeFullPath,
				  !!IsDlgButtonChecked(hWnd, IDUSEVXKEX),
				  !!IsDlgButtonChecked(hWnd, IDSPOOFVERSIONCHECK) ? (LPCTSTR) ComboBox_GetItemData(hWndWinVer, ComboBox_GetCurSel(hWndWinVer))
																  : T("NONE"),
				  !!IsDlgButtonChecked(hWnd, IDALWAYSSHOWDEBUG),
				  !!IsDlgButtonChecked(hWnd, IDDISABLEFORCHILD),
				  !!IsDlgButtonChecked(hWnd, IDDISABLEAPPSPECIFIC),
				  !!IsDlgButtonChecked(hWnd, IDWAITONCHILD));
		GetStartupInfo(&startupInfo);

		if (CreateProcess(szKexCfgFullPath, szKexCfgCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo) == FALSE) {
			ErrorBoxF(T("Failed to start KexCfg helper process. Error code: %#010I32x: %s"),
							  GetLastError(), GetLastErrorAsString());
			goto Error;
		}

		CloseHandle(procInfo.hThread);
		CloseHandle(procInfo.hProcess);

		bSettingsChanged = FALSE;
		return TRUE;
	} else if ((uMsg == WM_NOTIFY && ((LPNMHDR) lParam)->code == NM_CLICK ||
			    uMsg == WM_NOTIFY && ((LPNMHDR) lParam)->code == NM_RETURN) &&
			   wParam == IDREPORTBUG) {
		// user wants to report a bug on the github
		ShellExecute(hWnd, T("open"), REPORT_BUG_URL, NULL, NULL, SW_NORMAL);

		return TRUE;
	}

Error:
	return FALSE;
}

//
// DLL EXPORTED FUNCTIONS
//

DLLAPI BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		g_hInst = hInstance;
		DisableThreadLibraryCalls(hInstance);
		SetFriendlyAppName(FRIENDLYAPPNAME);
	}

	return TRUE;
}

STDAPI DllCanUnloadNow(
	VOID)
{
	return ((g_cRefDll == 0) ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(
	IN	REFCLSID	rclsid,
	IN	REFIID		riid,
	OUT	PPVOID		ppv)
{
	*ppv = NULL;

	if (IsEqualIID(rclsid, CLSID_KexShlEx)) {
		HRESULT hr;
		LPCCLASSFACTORY lpCClassFactory = new CClassFactory;

		if (lpCClassFactory == NULL) {
			return E_OUTOFMEMORY;
		}

		hr = lpCClassFactory->QueryInterface(riid, ppv);
		lpCClassFactory->Release();
		return hr;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

// NEVER REMOVE THE DELETION OF ANY REGISTRY ENTRIES HERE! Even if a newer version
// of the software stops creating a particular registry key, it may still be left over
// from a previous version.
STDAPI DllUnregisterServer(
	VOID)
{
	SHDeleteKey(HKEY_CLASSES_ROOT, T("CLSID\\") CLSID_STRING_KEXSHLEX);

	if (X64) {
		SHDeleteKey(HKEY_CLASSES_ROOT, T("Wow6432Node\\CLSID\\") CLSID_STRING_KEXSHLEX);
	}

	return S_OK;
}

// IF YOU ADD A REGISTRY ENTRY HERE, YOU MUST ALSO UPDATE DllUnregisterServer TO
// REMOVE THAT REGISTRY ENTRY! YOU MUST ALSO UPDATE THE FOLLOWING TREE DIAGRAM:
//
// HKEY_CLASSES_ROOT
//   CLSID
//     {9AACA888-A5F5-4C01-852E-8A2005C1D45F} (*)
//       InProcServer32
//         (Default)		= REG_SZ "<VxKexLdr installation directory>\KexShlEx.dll"
//         ThreadingModel	= REG_SZ "Apartment"
//
// A (*) next to a key indicates that this key is "owned" by VxKexLdr and may be
// safely deleted, along with all its subkeys, when the program is uninstalled.
//
// The above tree diagram must also be replicated into the Wow6432Node (but only
// if running on a 64-bits system). For 32-bits, the name of KexShlEx.dll changes
// to KexShl32.dll.
//
// The value of <VxKexLdr installation directory> can be found from the registry key
// HKEY_LOCAL_MACHINE\Software\VXsoft\VxKexLdr\KexDir (REG_SZ). This registry key is
// created by the VxKexLdr installer, and its absence indicates an incorrect usage of
// the DllRegisterServer function.
STDAPI DllRegisterServer(
	VOID)
{
	BOOL bSuccess;
	TCHAR szKexShlExDll32[MAX_PATH];
	TCHAR szKexShlExDll64[MAX_PATH];
	LPCTSTR szKexShlExDllNative = NULL;

	if (X64) {
		szKexShlExDllNative = szKexShlExDll64;
	} else if (X86) {
		szKexShlExDllNative = szKexShlExDll32;
	}

	bSuccess = RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexShlExDll32, ARRAYSIZE(szKexShlExDll32));
	if (bSuccess == FALSE) return E_UNEXPECTED;

	strcpy_s(szKexShlExDll64, MAX_PATH, szKexShlExDll32);
	strcat_s(szKexShlExDll32, MAX_PATH, T("\\KexShl32.dll"));
	strcat_s(szKexShlExDll64, MAX_PATH, T("\\KexShlEx.dll"));

	CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("CLSID\\") CLSID_STRING_KEXSHLEX T("\\InProcServer32"), NULL, szKexShlExDllNative));
	CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("CLSID\\") CLSID_STRING_KEXSHLEX T("\\InProcServer32"), T("ThreadingModel"), T("Apartment")));

	if (X64) {
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("Wow6432Node\\CLSID\\") CLSID_STRING_KEXSHLEX T("\\InProcServer32"), NULL, szKexShlExDll32));
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("Wow6432Node\\CLSID\\") CLSID_STRING_KEXSHLEX T("\\InProcServer32"), T("ThreadingModel"), T("Apartment")));
	}

	return S_OK;

Error:
	DllUnregisterServer();
	return SELFREG_E_CLASS;
}

// HKEY_CLASSES_ROOT
//   exefile
//     shellex
//       PropertySheetHandlers
//         KexShlEx Property Page (*)
//           (Default)	= REG_SZ "{9AACA888-A5F5-4C01-852E-8A2005C1D45F}"
STDAPI DllInstall(
	IN	BOOL	bInstall,
	IN	LPCWSTR	lpszCmdLine OPTIONAL)
{
	if (bInstall) {
		TCHAR szVxKexLdr[MAX_PATH];
		TCHAR szOpenVxKexCommand[MAX_PATH + 17];

		CHECKED(DllRegisterServer() == S_OK);
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("exefile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page"), NULL, CLSID_STRING_KEXSHLEX));
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("exefile\\shell\\open_vxkex"), NULL, T("Run with VxKex enabled")));
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("exefile\\shell\\open_vxkex"), T("Extended"), T("")));
		CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szVxKexLdr, ARRAYSIZE(szVxKexLdr)));
		CHECKED(!strcat_s(szVxKexLdr, ARRAYSIZE(szVxKexLdr), T("\\VxKexLdr.exe")));
		CHECKED(sprintf_s(szOpenVxKexCommand, ARRAYSIZE(szOpenVxKexCommand), T("\"%s\" /FORCE \"%%1\" \"%%*\""), szVxKexLdr) != -1);
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, T("exefile\\shell\\open_vxkex\\command"), NULL, szOpenVxKexCommand));
	} else {
		// Important note: You cannot "goto Error" from this section, so no CHECKED(x)
		// is permitted here. Otherwise it will cause recursion in the case of an error.

		// We use SHDeleteKeyA in preference to RegDeleteTree because it basically does
		// the same thing but is available since Win2k (instead of only since Vista)
		SHDeleteKey(HKEY_CLASSES_ROOT, T("exefile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page"));
		SHDeleteKey(HKEY_CLASSES_ROOT, T("exefile\\shell\\open_vxkex"));
		DllUnregisterServer();
	}

	return S_OK;

Error:
	DllInstall(FALSE, NULL);
	return E_FAIL;
}