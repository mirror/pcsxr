# Microsoft Developer Studio Project File - Name="gpuPeopsOpenGL" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=gpuPeopsOpenGL - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gpuPeopsOpenGL.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gpuPeopsOpenGL.mak" CFG="gpuPeopsOpenGL - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gpuPeopsOpenGL - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gpuPeopsOpenGL - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gpuPeopsOpenGL - Win32 Release"

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
# ADD CPP /nologo /G5 /MD /W3 /GX /O2 /I ".\\" /I ".\winsrc" /I "..\..\glue" /I "..\..\..\plugins\peopsxgl" /I "..\..\..\libpcsxcore" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 user32.lib gdi32.lib opengl32.lib winmm.lib advapi32.lib /nologo /subsystem:windows /dll /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=rem copy release\*.dll d:\emus\epsxe\plugins
# End Special Build Tool

!ELSEIF  "$(CFG)" == "gpuPeopsOpenGL - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\winsrc" /I "..\..\glue" /I "..\..\..\plugins\peopsxgl" /I "..\..\..\libpcsxcore" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "gpuPeopsOpenGL - Win32 Release"
# Name "gpuPeopsOpenGL - Win32 Debug"
# Begin Group "winsrc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\winsrc\cfg.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\fps.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\key.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\ssave.c
# End Source File
# Begin Source File

SOURCE=.\winsrc\ssave.h
# End Source File
# Begin Source File

SOURCE=.\winsrc\winmain.c
# End Source File
# End Group
# Begin Group "peopsxgl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\cfg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\draw.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\draw.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\externals.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\fps.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\gl_ext.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\gpu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\gpu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\key.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\menu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\menu.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\prim.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\prim.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\soft.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\soft.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\texture.c
# End Source File
# Begin Source File

SOURCE=..\..\..\plugins\peopsxgl\texture.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\gpuPeopsOpenGL.def
# End Source File
# Begin Source File

SOURCE=.\gpuPeopsOpenGL.rc
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Target
# End Project
