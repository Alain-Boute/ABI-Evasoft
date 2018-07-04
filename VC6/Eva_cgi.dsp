# Microsoft Developer Studio Project File - Name="Eva_cgi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Eva_cgi - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Eva_cgi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Eva_cgi.mak" CFG="Eva_cgi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Eva_cgi - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Eva_cgi - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Eva_cgi - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /O2 /I "c:\mysql\include" /I "C:\OpenSSL\include" /I "gd/include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fr /Fo"Debug/" /Fd"Debug/" /FD /c
# ADD BASE RSC /l 0x40c /d "NDEBUG"
# ADD RSC /l 0x40c /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 gd/lib/bgd.lib libeay32MDd.lib mysqlclient.lib kernel32.lib user32.lib uuid.lib wsock32.lib advapi32.lib version.lib LIBCMT.LIB oldnames.lib /nologo /subsystem:console /pdb:none /machine:I386 /nodefaultlib /out:"D:\travail\ABI\ApacheWeb\cgi\eva_cgi.cgi" /libpath:"C:\mysql\lib\opt" /libpath:"C:\OpenSSL\lib\VC\\"
# SUBTRACT LINK32 /verbose /debug

!ELSEIF  "$(CFG)" == "Eva_cgi - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MTd /W4 /Gm /Gi /GX /Zi /Od /I "c:\mysql\include" /I "C:\OpenSSL\include" /I "gd\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /FAs /FR /YX /FD /c
# ADD BASE RSC /l 0x40c /d "_DEBUG"
# ADD RSC /l 0x40c /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gd/lib/bgd.lib libeay32MDd.lib mysqlclient.lib kernel32.lib user32.lib wsock32.lib version.lib oldnames.lib advapi32.lib LIBCMT.LIB /nologo /subsystem:console /incremental:no /debug /machine:I386 /nodefaultlib /out:"D:\travail\ABI\ApacheWeb\cgi\eva_cgi.cgi" /pdbtype:sept /libpath:"C:\mysql\lib\opt" /libpath:"C:\OpenSSL\lib\VC"
# SUBTRACT LINK32 /map

!ENDIF 

# Begin Target

# Name "Eva_cgi - Win32 Release"
# Name "Eva_cgi - Win32 Debug"
# Begin Source File

SOURCE=.\action_create.c
# End Source File
# Begin Source File

SOURCE=.\action_email.c
# End Source File
# Begin Source File

SOURCE=.\action_export.c
# End Source File
# Begin Source File

SOURCE=.\action_import.c
# End Source File
# Begin Source File

SOURCE=.\action_launchproc.c
# End Source File
# Begin Source File

SOURCE=.\action_misc.c
# End Source File
# Begin Source File

SOURCE=.\action_paysite.c
# End Source File
# Begin Source File

SOURCE=.\action_sequence.c
# End Source File
# Begin Source File

SOURCE=.\action_sql_script.c
# End Source File
# Begin Source File

SOURCE=.\action_update.c
# End Source File
# Begin Source File

SOURCE=.\action_utils.c
# End Source File
# Begin Source File

SOURCE=.\cgi_utils.c
# End Source File
# Begin Source File

SOURCE=.\check_login.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_access.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_attr.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_butn.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_cgi.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_contain.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_exprval.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_hdlr.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_image.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_basic.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_chkbox.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_color.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_date.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_email.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_field.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_fileupload.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_hour.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_httplink.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_image.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_list.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_rel.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_input_tel.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_misc.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_option.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_output_calendar.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_output_catalog.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_output_list.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_output_planning.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_output_stats.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_title.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_tree.c
# End Source File
# Begin Source File

SOURCE=.\ctrl_utils.c
# End Source File
# Begin Source File

SOURCE=.\date_utils.c
# End Source File
# Begin Source File

SOURCE=.\dlg_openedforms.c
# End Source File
# Begin Source File

SOURCE=.\dll_adsea01.c
# End Source File
# Begin Source File

SOURCE=.\dll_fap.c
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

SOURCE=.\eva_cgi.c
# End Source File
# Begin Source File

SOURCE=.\file_image_utils.c
# End Source File
# Begin Source File

SOURCE=.\file_links.c
# End Source File
# Begin Source File

SOURCE=.\file_utils.c
# End Source File
# Begin Source File

SOURCE=.\form_html.c
# End Source File
# Begin Source File

SOURCE=.\form_io.c
# End Source File
# Begin Source File

SOURCE=.\form_mode_list.c
# End Source File
# Begin Source File

SOURCE=.\form_mode_search.c
# End Source File
# Begin Source File

SOURCE=.\form_mode_values.c
# End Source File
# Begin Source File

SOURCE=.\form_save.c
# End Source File
# Begin Source File

SOURCE=.\form_utils.c
# End Source File
# Begin Source File

SOURCE=.\html_basic.c
# End Source File
# Begin Source File

SOURCE=.\mem_utils.c
# End Source File
# Begin Source File

SOURCE=.\obj_utils.c
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

SOURCE=.\qry_obj_cache.c
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

SOURCE=.\table_utils.c
# End Source File
# Begin Source File

SOURCE=.\value_format.c
# End Source File
# End Target
# End Project
