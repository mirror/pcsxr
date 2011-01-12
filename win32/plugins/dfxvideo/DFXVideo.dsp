# Microsoft Developer Studio Project File - Name="DFXVideo" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DFXVideo - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DFXVideo.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DFXVideo.mak" CFG="DFXVideo - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DFXVideo - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DFXVideo - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DFXVideo - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G5 /MD /W3 /GX /O2 /I ".\\" /I ".\winsrc" /I "..\.." /I "..\..\glue" /I "..\..\..\plugins\dfxvideo" /I "..\..\..\libpcsxcore" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "__i386__" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 user32.lib gdi32.lib winmm.lib advapi32.lib vfw32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=rem copy release\DFXVideo.dll  d:\emus\epsxe\plugins	rem copy release\DFXVideo.dll d:\emus\zinc\renderer.znc
# End Special Build Tool

!ELSEIF  "$(CFG)" == "DFXVideo - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".\\" /I ".\winsrc" /I "..\..\glue" /I "..\..\..\plugins\dfxvideo" /I "..\..\..\libpcsxcore" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib winmm.lib advapi32.lib vfw32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "DFXVideo - Win32 Release"
# Name "DFXVideo - Win32 Debug"
# Begin Group "winsrc"

# PROP Default_Filter ""
# Begin Group "directx"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\winsrc\d3d.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\d3dcaps.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\d3dtypes.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\ddraw.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\dxguid.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\winsrc\cfg.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\draw.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\fps.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\key.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\record.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\record.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\winmain.c
# End Source File
# End Group
# Begin Group "dfxvideo"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\cfg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\draw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\externals.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\fps.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\gpu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\gpu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\hq2x.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\hq3x.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\interp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\key.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\menu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\menu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\prim.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\prim.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\soft.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\soft.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\swap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfxvideo\zn.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\DFXVideo.def
# End Source File
# Begin Source File

SOURCE=.\DFXVideo.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Target
# End Project
