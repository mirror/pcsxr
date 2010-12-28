/***************************************************************************
                            cfg.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "stdafx.h"

#define _IN_CFG

#include "externals.h"

////////////////////////////////////////////////////////////////////////
// WINDOWS CONFIG/ABOUT HANDLING
////////////////////////////////////////////////////////////////////////

#include "resource.h"

////////////////////////////////////////////////////////////////////////
// simple about dlg handler
////////////////////////////////////////////////////////////////////////

BOOL CALLBACK AboutDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {case IDOK:  EndDialog(hW,TRUE);return TRUE;}
    }
  }
 return FALSE;
}

////////////////////////////////////////////////////////////////////////
// READ CONFIG: from win registry
////////////////////////////////////////////////////////////////////////

void ReadConfig(void)
{
 HKEY myKey;
 DWORD temp;
 DWORD type;
 DWORD size;

 iVolume=1;                                            // init vars
 iXAPitch=1;
 iUseTimer=2;
 iSPUIRQWait=1;
 iDebugMode=0;
 iRecordMode=0;
 iUseReverb=2;
 iUseInterpolation=2;
 iDisStereo=0;
 iFreqResponse=0;

 if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\SPU\\DFSound",0,KEY_ALL_ACCESS,&myKey)==ERROR_SUCCESS)
  {
   size = 4;
   if(RegQueryValueEx(myKey,"Volume",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iVolume=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"XAPitch",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iXAPitch=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseTimer",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseTimer=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"SPUIRQWait",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iSPUIRQWait=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"FreqResponse",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iFreqResponse=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"DebugMode",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iDebugMode=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"RecordMode",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iRecordMode=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseReverb",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseReverb=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"UseInterpolation",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iUseInterpolation=(int)temp;
   size = 4;
   if(RegQueryValueEx(myKey,"DisStereo",0,&type,(LPBYTE)&temp,&size)==ERROR_SUCCESS)
    iDisStereo=(int)temp;

   RegCloseKey(myKey);
  }

 if(iUseTimer>2) iUseTimer=2;              // some checks
 if(iVolume<1) iVolume=1;
 if(iVolume>5) iVolume=5;
}

////////////////////////////////////////////////////////////////////////
// WRITE CONFIG: in win registry
////////////////////////////////////////////////////////////////////////

void WriteConfig(void)
{
 HKEY myKey;
 DWORD myDisp;
 DWORD temp;

 RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\Vision Thing\\PSEmu Pro\\SPU\\DFSound",0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&myKey,&myDisp);
 temp=iVolume;
 RegSetValueEx(myKey,"Volume",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iXAPitch;
 RegSetValueEx(myKey,"XAPitch",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseTimer;
 RegSetValueEx(myKey,"UseTimer",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iSPUIRQWait;
 RegSetValueEx(myKey,"SPUIRQWait",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iFreqResponse;
 RegSetValueEx(myKey,"FreqResponse",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iDebugMode;
 RegSetValueEx(myKey,"DebugMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iRecordMode;
 RegSetValueEx(myKey,"RecordMode",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseReverb;
 RegSetValueEx(myKey,"UseReverb",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iUseInterpolation;
 RegSetValueEx(myKey,"UseInterpolation",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));
 temp=iDisStereo;
 RegSetValueEx(myKey,"DisStereo",0,REG_DWORD,(LPBYTE) &temp,sizeof(temp));

 RegCloseKey(myKey);
}

////////////////////////////////////////////////////////////////////////
// INIT WIN CFG DIALOG
////////////////////////////////////////////////////////////////////////

BOOL OnInitDSoundDialog(HWND hW) 
{
 HWND hWC;

 ReadConfig();
                
 if(iXAPitch)    CheckDlgButton(hW,IDC_XAPITCH,TRUE);

 hWC=GetDlgItem(hW,IDC_VOLUME);
 ComboBox_AddString(hWC, "None");
 ComboBox_AddString(hWC, "Low");
 ComboBox_AddString(hWC, "Medium");
 ComboBox_AddString(hWC, "Loud");
 ComboBox_AddString(hWC, "Loudest");
 ComboBox_SetCurSel(hWC,5-iVolume);

 if(iSPUIRQWait) CheckDlgButton(hW,IDC_IRQWAIT,TRUE);
 if(iDebugMode)  CheckDlgButton(hW,IDC_DEBUGMODE,TRUE);
 if(iRecordMode) CheckDlgButton(hW,IDC_RECORDMODE,TRUE);
 if(iDisStereo)  CheckDlgButton(hW,IDC_DISSTEREO,TRUE);
 if(iFreqResponse) CheckDlgButton(hW,IDC_FREQRESPONSE,TRUE);

 hWC=GetDlgItem(hW,IDC_USETIMER);
 ComboBox_AddString(hWC, "Fast mode (thread, less compatible spu timing)");
 ComboBox_AddString(hWC, "High compatibility mode (timer event, slower)");
 ComboBox_AddString(hWC, "Use SPUasync (must be supported by the emu)");
 ComboBox_SetCurSel(hWC,iUseTimer);

 hWC=GetDlgItem(hW,IDC_USEREVERB);
 ComboBox_AddString(hWC, "No reverb (fastest)");
 ComboBox_AddString(hWC, "Simple reverb (fakes the most common effects)");
 ComboBox_AddString(hWC, "PSX reverb (best quality)");
 ComboBox_SetCurSel(hWC,iUseReverb);

 hWC=GetDlgItem(hW,IDC_INTERPOL);
 ComboBox_AddString(hWC, "None (fastest)");
 ComboBox_AddString(hWC, "Simple interpolation");
 ComboBox_AddString(hWC, "Gaussian interpolation (good quality)");
 ComboBox_AddString(hWC, "Cubic interpolation (better treble)");
 ComboBox_SetCurSel(hWC,iUseInterpolation);

 return TRUE;
}

////////////////////////////////////////////////////////////////////////
// WIN CFG DLG OK
////////////////////////////////////////////////////////////////////////

void OnDSoundOK(HWND hW) 
{
 HWND hWC;

 if(IsDlgButtonChecked(hW,IDC_XAPITCH))
  iXAPitch=1; else iXAPitch=0;

 hWC=GetDlgItem(hW,IDC_VOLUME);
 iVolume=5-ComboBox_GetCurSel(hWC);

 hWC=GetDlgItem(hW,IDC_USETIMER);
 iUseTimer=ComboBox_GetCurSel(hWC);

 hWC=GetDlgItem(hW,IDC_USEREVERB);
 iUseReverb=ComboBox_GetCurSel(hWC);

 hWC=GetDlgItem(hW,IDC_INTERPOL);
 iUseInterpolation=ComboBox_GetCurSel(hWC);

 if(IsDlgButtonChecked(hW,IDC_IRQWAIT))
  iSPUIRQWait=1; else iSPUIRQWait=0;

 if(IsDlgButtonChecked(hW,IDC_DEBUGMODE))
  iDebugMode=1; else iDebugMode=0;

 if(IsDlgButtonChecked(hW,IDC_RECORDMODE))
  iRecordMode=1; else iRecordMode=0;

 if(IsDlgButtonChecked(hW,IDC_DISSTEREO))
  iDisStereo=1; else iDisStereo=0;

 if(IsDlgButtonChecked(hW,IDC_FREQRESPONSE))
  iFreqResponse=1; else iFreqResponse=0;

 WriteConfig();                                        // write registry

 EndDialog(hW,TRUE);
}

////////////////////////////////////////////////////////////////////////
// WIN CFG DLG CANCEL
////////////////////////////////////////////////////////////////////////

void OnDSoundCancel(HWND hW) 
{
 EndDialog(hW,FALSE);
}

////////////////////////////////////////////////////////////////////////
// WIN CFG PROC
////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DSoundDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
 switch(uMsg)
  {
   case WM_INITDIALOG:
     return OnInitDSoundDialog(hW);

   case WM_COMMAND:
    {
     switch(LOWORD(wParam))
      {
       case IDCANCEL:     OnDSoundCancel(hW);return TRUE;
       case IDOK:         OnDSoundOK(hW);   return TRUE;
      }
    }
  }
 return FALSE;
}
