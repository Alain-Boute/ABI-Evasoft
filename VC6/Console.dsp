# Microsoft Developer Studio Project File - Name="Console" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Console - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Console.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Console.mak" CFG="Console - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Console - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Console - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Console - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Console_"
# PROP BASE Intermediate_Dir "Console_"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "c:\mysql\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fr /Fo"Debug/" /Fd"Debug/" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 th32.lib mysqlclient.lib kernel32.lib user32.lib uuid.lib wsock32.lib advapi32.lib version.lib LIBCMT.LIB oldnames.lib psapi.lib /nologo /subsystem:console /pdb:none /machine:I386 /nodefaultlib /out:"D:\travail\ABI\ApacheWeb\cgi\console.cgi" /libpath:"C:\mysql\lib\opt"

!ELSEIF  "$(CFG)" == "Console - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Console0"
# PROP BASE Intermediate_Dir "Console0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "c:\mysql\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FAs /FR /Fp"Debug/Eva_cgi.pch" /YX /FD /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 th32.lib mysqlclient.lib kernel32.lib user32.lib wsock32.lib version.lib LIBCMT.LIB oldnames.lib advapi32.lib psapi.lib /nologo /subsystem:console /pdb:"Debug/evasoft.pdb" /debug /machine:I386 /nodefaultlib /out:"D:\travail\ABI\ApacheWeb\cgi\console.cgi" /pdbtype:sept /libpath:"C:\mysql\lib\opt"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Console - Win32 Release"
# Name "Console - Win32 Debug"
# Begin Source File

SOURCE=.\action_utils.c
# End Source File
# Begin Source File

SOURCE=.\cgi_utils.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_graph.c
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

SOURCE=.\file_utils.c
# End Source File
# Begin Source File

SOURCE=.\form_utils.c
# End Source File
# Begin Source File

SOURCE=.\html_basic.c
# End Source File
# Begin Source File

SOURCE=.\html_console.c
# End Source File
# Begin Source File

SOURCE=.\mem_utils.c
# End Source File
# Begin Source File

SOURCE=.\qry_build.c
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
# End Target
# End Project
