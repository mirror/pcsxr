# Microsoft Developer Studio Project File - Name="DFSound" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=DFSound - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DFSound.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DFSound.mak" CFG="DFSound - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DFSound - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "DFSound - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DFSound - Win32 Release"

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
# ADD CPP /nologo /G5 /MD /W3 /GX /O2 /I ".\\" /I ".\winsrc" /I "..\..\..\plugins\dfsound" /I "..\..\glue" /I "..\..\..\libpcsxcore" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 dsound.lib winmm.lib user32.lib gdi32.lib advapi32.lib /nologo /subsystem:windows /dll /machine:I386

!ELSEIF  "$(CFG)" == "DFSound - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I ".\\" /I ".\winsrc" /I "..\..\..\plugins\dfsound" /I "..\..\glue" /I "..\..\..\libpcsxcore" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 dsound.lib winmm.lib user32.lib gdi32.lib advapi32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "DFSound - Win32 Release"
# Name "DFSound - Win32 Debug"
# Begin Group "winsrc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\winsrc\cfg.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\debug.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\debug.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\dsound.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\dsound.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\psemu.c
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
# Begin Group "dfsound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\adsr.c

!IF  "$(CFG)" == "DFSound - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DFSound - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\adsr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\dma.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\dma.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\dsoundoss.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\externals.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\freeze.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\gauss_i.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\psemuxa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\registers.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\registers.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\regs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\reverb.c

!IF  "$(CFG)" == "DFSound - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DFSound - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\reverb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\spu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\spu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\xa.c

!IF  "$(CFG)" == "DFSound - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "DFSound - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\dfsound\xa.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\DFSound.def
# End Source File
# Begin Source File

SOURCE=.\DFSound.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Target
# End Project
