# Microsoft Developer Studio Project File - Name="stlplus" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=stlplus - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "stlplus.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "stlplus.mak" CFG="stlplus - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "stlplus - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "stlplus - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "stlplus - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fr /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "stlplus - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fr /FD /I /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "stlplus - Win32 Release"
# Name "stlplus - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cli_parser.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\dprintf.cpp
# End Source File
# Begin Source File

SOURCE=.\error_handler.cpp
# End Source File
# Begin Source File

SOURCE=.\exceptions.cpp
# End Source File
# Begin Source File

SOURCE=.\file_system.cpp
# End Source File
# Begin Source File

SOURCE=.\fileio.cpp
# End Source File
# Begin Source File

SOURCE=.\inf.cpp
# End Source File
# Begin Source File

SOURCE=.\ini_manager.cpp
# End Source File
# Begin Source File

SOURCE=.\iostreamio.cpp
# End Source File
# Begin Source File

SOURCE=.\library_manager.cpp
# End Source File
# Begin Source File

SOURCE=.\multiio.cpp
# End Source File
# Begin Source File

SOURCE=.\os_fixes.cpp
# End Source File
# Begin Source File

SOURCE=.\persistent.cpp
# End Source File
# Begin Source File

SOURCE=.\persistent_exceptions.cpp
# End Source File
# Begin Source File

SOURCE=.\stlplus.cpp
# End Source File
# Begin Source File

SOURCE=.\string_arithmetic.cpp
# End Source File
# Begin Source File

SOURCE=.\string_utilities.cpp
# End Source File
# Begin Source File

SOURCE=.\string_vectorio.cpp
# End Source File
# Begin Source File

SOURCE=.\stringio.cpp
# End Source File
# Begin Source File

SOURCE=.\subprocesses.cpp
# End Source File
# Begin Source File

SOURCE=.\tcp.cpp
# End Source File
# Begin Source File

SOURCE=.\textio.cpp
# End Source File
# Begin Source File

SOURCE=.\time.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cli_parser.hpp
# End Source File
# Begin Source File

SOURCE=.\clonable.hpp
# End Source File
# Begin Source File

SOURCE=.\debug.hpp
# End Source File
# Begin Source File

SOURCE=.\digraph.hpp
# End Source File
# Begin Source File

SOURCE=.\dprintf.hpp
# End Source File
# Begin Source File

SOURCE=.\error_handler.hpp
# End Source File
# Begin Source File

SOURCE=.\exceptions.hpp
# End Source File
# Begin Source File

SOURCE=.\file_system.hpp
# End Source File
# Begin Source File

SOURCE=.\fileio.hpp
# End Source File
# Begin Source File

SOURCE=.\hash.hpp
# End Source File
# Begin Source File

SOURCE=.\inf.hpp
# End Source File
# Begin Source File

SOURCE=.\ini_manager.hpp
# End Source File
# Begin Source File

SOURCE=.\iostreamio.hpp
# End Source File
# Begin Source File

SOURCE=.\library_manager.hpp
# End Source File
# Begin Source File

SOURCE=.\matrix.hpp
# End Source File
# Begin Source File

SOURCE=.\multiio.hpp
# End Source File
# Begin Source File

SOURCE=.\ntree.hpp
# End Source File
# Begin Source File

SOURCE=.\os_fixes.hpp
# End Source File
# Begin Source File

SOURCE=.\persistent.hpp
# End Source File
# Begin Source File

SOURCE=.\persistent_exceptions.hpp
# End Source File
# Begin Source File

SOURCE=.\smart_ptr.hpp
# End Source File
# Begin Source File

SOURCE=.\stlplus.hpp
# End Source File
# Begin Source File

SOURCE=.\string_arithmetic.hpp
# End Source File
# Begin Source File

SOURCE=.\string_utilities.hpp
# End Source File
# Begin Source File

SOURCE=.\string_vectorio.hpp
# End Source File
# Begin Source File

SOURCE=.\stringio.hpp
# End Source File
# Begin Source File

SOURCE=.\subprocesses.hpp
# End Source File
# Begin Source File

SOURCE=.\tcp.hpp
# End Source File
# Begin Source File

SOURCE=.\textio.hpp
# End Source File
# Begin Source File

SOURCE=.\time.hpp
# End Source File
# Begin Source File

SOURCE=.\timer.hpp
# End Source File
# Begin Source File

SOURCE=.\triple.hpp
# End Source File
# End Group
# Begin Group "Template Implementations"

# PROP Default_Filter "tpp"
# Begin Source File

SOURCE=.\digraph.tpp
# End Source File
# Begin Source File

SOURCE=.\hash.tpp
# End Source File
# Begin Source File

SOURCE=.\matrix.tpp
# End Source File
# Begin Source File

SOURCE=.\ntree.tpp
# End Source File
# Begin Source File

SOURCE=.\persistent.tpp
# End Source File
# Begin Source File

SOURCE=.\smart_ptr.tpp
# End Source File
# Begin Source File

SOURCE=.\string_utilities.tpp
# End Source File
# Begin Source File

SOURCE=.\triple.tpp
# End Source File
# End Group
# End Target
# End Project
