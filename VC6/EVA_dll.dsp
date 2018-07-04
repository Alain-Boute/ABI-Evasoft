# Microsoft Developer Studio Project File - Name="EVA_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=EVA_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "EVA_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EVA_dll.mak" CFG="EVA_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EVA_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EVA_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EVA_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "EVA_dll_"
# PROP BASE Intermediate_Dir "EVA_dll_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "c:\mysql\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "NO_HANDLERS" /D "_EVA_DLL" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 th32.lib mysqlclient.lib kernel32.lib user32.lib uuid.lib wsock32.lib advapi32.lib version.lib LIBCMT.LIB oldnames.lib /nologo /subsystem:windows /dll /machine:I386 /out:"C:\Windows\EVA_dll.dll" /libpath:"C:\mysql\lib\opt"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "EVA_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "EVA_dll0"
# PROP BASE Intermediate_Dir "EVA_dll0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "c:\mysql\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_EVA_DLL" /FAs /FR /Fp"Debug/Eva_cgi.pch" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 th32.lib mysqlclient.lib kernel32.lib user32.lib wsock32.lib advapi32.lib version.lib LIBCMT.LIB oldnames.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib /out:"C:\Windows\EVA_dll.dll" /pdbtype:sept /libpath:"C:\mysql\lib\opt"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "EVA_dll - Win32 Release"
# Name "EVA_dll - Win32 Debug"
# Begin Source File

SOURCE=.\action_utils.c
# End Source File
# Begin Source File

SOURCE=.\cgi_utils.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_attr.c
# End Source File
# Begin Source File

SOURCE=.\date_utils.c
# End Source File
# Begin Source File

SOURCE=.\dynbuf.c
# End Source File
# Begin Source File

SOURCE=.\dynbuf_sr.c
# End Source File
# Begin Source File

SOURCE=.\dyntab.c
# End Source File
# Begin Source File

SOURCE=.\dyntab_field.c
# End Source File
# Begin Source File

SOURCE=.\dyntab_sort.c
# End Source File
# Begin Source File

SOURCE=.\err_utils.c
# End Source File
# Begin Source File

SOURCE=.\EVA_dll.c
# End Source File
# Begin Source File

SOURCE=.\EVA_dll.def
# End Source File
# Begin Source File

SOURCE=.\file_utils.c
# End Source File
# Begin Source File

SOURCE=.\form_utils.c
# End Source File
# Begin Source File

SOURCE=.\mem_utils.c
# End Source File
# Begin Source File

SOURCE=.\pivottable.c
# End Source File
# Begin Source File

SOURCE=.\qry_build.c
# End Source File
# Begin Source File

SOURCE=.\qry_build_to_sql.c
# End Source File
# Begin Source File

SOURCE=.\qry_obj_label.c
# End Source File
# Begin Source File

SOURCE=.\qry_utils.c
# End Source File
# Begin Source File

SOURCE=.\sql_dbif.c
# End Source File
# Begin Source File

SOURCE=.\sql_values.c
# End Source File
# Begin Source File

SOURCE=.\table_export.c
# End Source File
# Begin Source File

SOURCE=.\table_search.c
# End Source File
# Begin Source File

SOURCE=.\value_format.c
# End Source File
# End Target
# End Project
