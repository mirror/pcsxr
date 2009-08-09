/*  Cheat Support for PCSX-Reloaded
 *
 *  Copyright (C) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include "psxcommon.h"
#include "psxmem.h"
#include "cheat.h"
#include "resource.h"
#include "Win32.h"

static void UpdateCheatDlg(HWND hW) {
    HWND		List;
    LV_ITEM		item;
	int			i;

	List = GetDlgItem(hW, IDC_CODELIST);

	ListView_DeleteAllItems(List);

	for (i = 0; i < NumCheats; i++) {
		memset(&item, 0, sizeof(item));

		item.mask		= LVIF_TEXT;
		item.iItem		= i;
		item.pszText	= Cheats[i].Descr;
		item.iSubItem	= 0;

		SendMessage(List, LVM_INSERTITEM, 0, (LPARAM)&item);

		item.pszText	= (Cheats[i].Enabled ? _("Yes") : _("No"));
		item.iSubItem	= 1;

		SendMessage(List, LVM_SETITEM, 0, (LPARAM)&item);
	}
}

static int iEditItem = -1;

static LRESULT WINAPI CheatEditDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char	szDescr[256], szCode[1024];
	int		i;

	switch (uMsg) {
		case WM_INITDIALOG:
			SetWindowText(hW, _("Edit Cheat"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_DESCR), _("Description:"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_CODE), _("Cheat Code:"));
			Button_SetText(GetDlgItem(hW, IDOK), _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL), _("Cancel"));

			assert(iEditItem != -1 && iEditItem < NumCheats);

			Edit_SetText(GetDlgItem(hW, IDC_DESCR), Cheats[iEditItem].Descr);

			szCode[0] = '\0';
			for (i = Cheats[iEditItem].First; i < Cheats[iEditItem].First + Cheats[iEditItem].n; i++) {
				sprintf(szDescr, "%.8X %.4X\r\n", CheatCodes[i].Addr, CheatCodes[i].Val);
				strcat(szCode, szDescr);
			}
			Edit_SetText(GetDlgItem(hW, IDC_CODE), szCode);
			break;

		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					Edit_GetText(GetDlgItem(hW, IDC_DESCR), szDescr, 256);
					Edit_GetText(GetDlgItem(hW, IDC_CODE), szCode, 1024);

					if (EditCheat(iEditItem, szDescr, szCode) != 0) {
						SysMessage(_("Error"), _("Invalid cheat code!"));
					}
					else {
                        EndDialog(hW, TRUE);
                        return TRUE;
					}
					break;

				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
			}
			break;

		case WM_CLOSE:
			EndDialog(hW, TRUE);
			return TRUE;
	}

	return FALSE;
}

static LRESULT WINAPI CheatAddDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char	szDescr[256], szCode[1024];

	switch (uMsg) {
		case WM_INITDIALOG:
			SetWindowText(hW, _("Add New Cheat"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_DESCR), _("Description:"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_CODE), _("Cheat Code:"));
			Button_SetText(GetDlgItem(hW, IDOK), _("OK"));
			Button_SetText(GetDlgItem(hW, IDCANCEL), _("Cancel"));
			break;

		case WM_COMMAND:
			switch (wParam) {
				case IDOK:
					Edit_GetText(GetDlgItem(hW, IDC_DESCR), szDescr, 256);
					Edit_GetText(GetDlgItem(hW, IDC_CODE), szCode, 1024);

					if (AddCheat(szDescr, szCode) != 0) {
						SysMessage(_("Error"), _("Invalid cheat code!"));
					}
					else {
                        EndDialog(hW, TRUE);
                        return TRUE;
					}
					break;

				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;
			}
			break;

		case WM_CLOSE:
			EndDialog(hW, TRUE);
			return TRUE;
	}

	return FALSE;
}

LRESULT WINAPI CheatDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND			List;
	LV_COLUMN		col;
	LV_ITEM			item;
	int				i;
	OPENFILENAME	ofn;
	char			szFileName[256];
	char			szFileTitle[256];
	char			szFilter[256];

	switch (uMsg) {
		case WM_INITDIALOG:
			SetWindowText(hW, _("Edit Cheat Codes"));

			Button_SetText(GetDlgItem(hW, IDC_ADDCODE), _("&Add Code"));
			Button_SetText(GetDlgItem(hW, IDC_EDITCODE), _("&Edit Code"));
			Button_SetText(GetDlgItem(hW, IDC_REMOVECODE), _("&Remove Code"));
			Button_SetText(GetDlgItem(hW, IDC_TOGGLECODE), _("&Enable/Disable"));
			Button_SetText(GetDlgItem(hW, IDC_LOADCODE), _("&Load..."));
			Button_SetText(GetDlgItem(hW, IDC_SAVECODE), _("&Save As..."));
			Button_SetText(GetDlgItem(hW, IDCANCEL), _("&Close"));

			List = GetDlgItem(hW, IDC_CODELIST);

            SendMessage(List, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

			memset(&col, 0, sizeof(col));

			col.mask	= LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			col.fmt		= LVCFMT_LEFT;

			col.pszText		= _("Description");
			col.cx			= 400;

			SendMessage(List, LVM_INSERTCOLUMN, 0, (LPARAM)&col);

			col.pszText		= _("Enabled");
			col.cx			= 55;

			SendMessage(List, LVM_INSERTCOLUMN, 1, (LPARAM)&col);

			UpdateCheatDlg(hW);
			break;

		case WM_COMMAND:
			switch (wParam) {
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;

				case IDC_ADDCODE:
					i = NumCheats;
					DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CHEATEDIT), hW, CheatAddDlgProc);

					if (NumCheats > i) {
						// new cheat added
						List = GetDlgItem(hW, IDC_CODELIST);
						memset(&item, 0, sizeof(item));

						item.mask		= LVIF_TEXT;
						item.iItem		= i;
						item.pszText	= Cheats[i].Descr;
						item.iSubItem	= 0;

						SendMessage(List, LVM_INSERTITEM, 0, (LPARAM)&item);

						item.pszText	= (Cheats[i].Enabled ? _("Yes") : _("No"));
						item.iSubItem	= 1;

						SendMessage(List, LVM_SETITEM, 0, (LPARAM)&item);
					}
					break;

				case IDC_EDITCODE:
					List = GetDlgItem(hW, IDC_CODELIST);
					iEditItem = ListView_GetSelectionMark(List);

					if (iEditItem != -1) {
						DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_CHEATEDIT), hW, CheatEditDlgProc);

						memset(&item, 0, sizeof(item));

						item.mask		= LVIF_TEXT;
						item.iItem		= iEditItem;
						item.pszText	= Cheats[iEditItem].Descr;
						item.iSubItem	= 0;

						SendMessage(List, LVM_SETITEM, 0, (LPARAM)&item);
					}
					break;

				case IDC_REMOVECODE:
					List = GetDlgItem(hW, IDC_CODELIST);
					i = ListView_GetSelectionMark(List);

					if (i != -1) {
						RemoveCheat(i);
						ListView_DeleteItem(List, i);
						ListView_SetSelectionMark(List, -1);
					}
					break;

				case IDC_TOGGLECODE:
					List = GetDlgItem(hW, IDC_CODELIST);
					i = ListView_GetSelectionMark(List);

					if (i != -1) {
						Cheats[i].Enabled ^= 1;

						memset(&item, 0, sizeof(item));

						item.mask		= LVIF_TEXT;
						item.iItem		= i;
						item.pszText	= (Cheats[i].Enabled ? _("Yes") : _("No"));
						item.iSubItem	= 1;

						SendMessage(List, LVM_SETITEM, 0, (LPARAM)&item);
					}
					break;

				case IDC_LOADCODE:
					memset(&szFileName,  0, sizeof(szFileName));
                    memset(&szFileTitle, 0, sizeof(szFileTitle));
                    memset(&szFilter,    0, sizeof(szFilter));

                    strcpy(szFilter, _("PCSX Cheat Code Files"));
					strcatz(szFilter, "*.*");

					ofn.lStructSize			= sizeof(OPENFILENAME);
					ofn.hwndOwner			= hW;
					ofn.lpstrFilter			= szFilter;
					ofn.lpstrCustomFilter	= NULL;
					ofn.nMaxCustFilter		= 0;
					ofn.nFilterIndex		= 1;
					ofn.lpstrFile			= szFileName;
					ofn.nMaxFile			= 256;
					ofn.lpstrInitialDir		= ".\\Cheats";
					ofn.lpstrFileTitle		= szFileTitle;
					ofn.nMaxFileTitle		= 256;
					ofn.lpstrTitle			= NULL;
					ofn.lpstrDefExt			= "CHT";
					ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

					if (GetOpenFileName((LPOPENFILENAME)&ofn)) {
						LoadCheats(szFileName);
						UpdateCheatDlg(hW);
					}
					break;

				case IDC_SAVECODE:
					memset(&szFileName,  0, sizeof(szFileName));
                    memset(&szFileTitle, 0, sizeof(szFileTitle));
                    memset(&szFilter,    0, sizeof(szFilter));

                    strcpy(szFilter, _("PCSX Cheat Code Files"));
					strcatz(szFilter, "*.*");

					ofn.lStructSize			= sizeof(OPENFILENAME);
					ofn.hwndOwner			= hW;
					ofn.lpstrFilter			= szFilter;
					ofn.lpstrCustomFilter	= NULL;
					ofn.nMaxCustFilter		= 0;
					ofn.nFilterIndex		= 1;
					ofn.lpstrFile			= szFileName;
					ofn.nMaxFile			= 256;
					ofn.lpstrInitialDir		= ".\\Cheats";
					ofn.lpstrFileTitle		= szFileTitle;
					ofn.nMaxFileTitle		= 256;
					ofn.lpstrTitle			= NULL;
					ofn.lpstrDefExt			= "CHT";
					ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;

					if (GetOpenFileName((LPOPENFILENAME)&ofn)) {
						SaveCheats(szFileName);
					}
					break;
			}
			break;

		case WM_NOTIFY:
			switch (LOWORD(wParam)) {
				case IDC_CODELIST:
					List = GetDlgItem(hW, IDC_CODELIST);
					i = ListView_GetSelectionMark(List);

					if (i != -1) {
						Button_Enable(GetDlgItem(hW, IDC_EDITCODE), TRUE);
						Button_Enable(GetDlgItem(hW, IDC_REMOVECODE), TRUE);
						Button_Enable(GetDlgItem(hW, IDC_TOGGLECODE), TRUE);
					}
					else {
						Button_Enable(GetDlgItem(hW, IDC_EDITCODE), FALSE);
						Button_Enable(GetDlgItem(hW, IDC_REMOVECODE), FALSE);
						Button_Enable(GetDlgItem(hW, IDC_TOGGLECODE), FALSE);
					}

					Button_Enable(GetDlgItem(hW, IDC_SAVECODE), (NumCheats > 0));
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hW, TRUE);
			return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

#define SEARCH_EQUALVAL				0
#define SEARCH_NOTEQUALVAL			1
#define SEARCH_RANGE				2
#define SEARCH_INCBY				3
#define SEARCH_DECBY				4
#define SEARCH_INC					5
#define SEARCH_DEC					6
#define SEARCH_DIFFERENT			7
#define SEARCH_NOCHANGE				8

#define SEARCHTYPE_8BIT				0
#define SEARCHTYPE_16BIT			1
#define SEARCHTYPE_32BIT			2

#define SEARCHBASE_DEC				0
#define SEARCHBASE_HEX				1

static char current_search			= SEARCH_EQUALVAL;
static char current_searchtype		= SEARCHTYPE_8BIT;
static char current_searchbase		= SEARCHBASE_DEC;
static uint32_t current_valuefrom	= 0;
static uint32_t current_valueto		= 0;

static void UpdateCheatSearchDlg(HWND hW) {
}

LRESULT WINAPI CheatSearchDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
			SetWindowText(hW, _("Cheat Search"));

			Static_SetText(GetDlgItem(hW, IDC_LABEL_SEARCHFOR), _("Search For:"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_DATATYPE), _("Data Type:"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_VALUE), _("Value:"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_DATABASE), _("Data Base:"));
			Static_SetText(GetDlgItem(hW, IDC_LABEL_TO), _("To:"));
			Button_SetText(GetDlgItem(hW, IDC_FREEZE), _("&Freeze"));
			Button_SetText(GetDlgItem(hW, IDC_MODIFY), _("&Modify"));
			Button_SetText(GetDlgItem(hW, IDC_COPY), _("&Copy"));
			Button_SetText(GetDlgItem(hW, IDC_SEARCH), _("&Search"));
			Button_SetText(GetDlgItem(hW, IDC_NEWSEARCH), _("&New Search"));
			Button_SetText(GetDlgItem(hW, IDCANCEL), _("C&lose"));

			UpdateCheatSearchDlg(hW);
			break;

		case WM_COMMAND:
			switch (wParam) {
				case IDCANCEL:
					EndDialog(hW, TRUE);
					return TRUE;

				case IDC_FREEZE:
					break;

				case IDC_MODIFY:
					break;

				case IDC_COPY:
					break;

				case IDC_SEARCH:
					break;

				case IDC_NEWSEARCH:
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hW, TRUE);
			return TRUE;
	}

	return FALSE;
}
