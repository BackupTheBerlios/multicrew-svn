# Microsoft Developer Studio Project File - Name="multicrewgauge" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=multicrewgauge - Win32 Debug
!MESSAGE Dies ist kein g�ltiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und f�hren Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "multicrewgauge.mak".
!MESSAGE 
!MESSAGE Sie k�nnen beim Ausf�hren von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "multicrewgauge.mak" CFG="multicrewgauge - Win32 Debug"
!MESSAGE 
!MESSAGE F�r die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "multicrewgauge - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "multicrewgauge - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "multicrewgauge - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MULTICREWGAUGE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MULTICREWGAUGE_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib gdiplus.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "multicrewgauge - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "multicrewgauge___Win32_Debug"
# PROP BASE Intermediate_Dir "multicrewgauge___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MULTICREWGAUGE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "MULTICREWGAUGE_EXPORTS" /D "_CRTDBG_MAP_ALLOC" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib gdiplus.lib /nologo /dll /map /debug /debugtype:both /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "multicrewgauge - Win32 Release"
# Name "multicrewgauge - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\apihijack.cpp
# End Source File
# Begin Source File

SOURCE=.\elements.cpp
# End Source File
# Begin Source File

SOURCE=.\gauge.cpp
# End Source File
# Begin Source File

SOURCE=.\gaugemodule.cpp
# End Source File
# Begin Source File

SOURCE=.\iconelement.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\metafile.cpp
# End Source File
# Begin Source File

SOURCE=.\needleelement.cpp
# End Source File
# Begin Source File

SOURCE=.\staticelement.cpp
# End Source File
# Begin Source File

SOURCE=.\stringelement.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\apihijack.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\GAUGES.H
# End Source File
# Begin Source File

SOURCE=.\metafile.h
# End Source File
# Begin Source File

SOURCE=.\multicrewgauge.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\big.bmp
# End Source File
# Begin Source File

SOURCE=.\multicrewgauge.rc
# End Source File
# End Group
# Begin Group "zdelta"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\zdelta\adler32.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\deflate.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\deflate.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\infblock.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\infblock.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\infcodes.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\inffast.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\inffast.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\inflate.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\inftrees.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\infutil.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\infutil.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\tailor.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\trees.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\trees.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\zd_mem.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\zd_mem.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\zdconf.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\zdelta.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\zdlib.h
# End Source File
# Begin Source File

SOURCE=.\zdelta\zutil.c
# End Source File
# Begin Source File

SOURCE=.\zdelta\zutil.h
# End Source File
# End Group
# End Target
# End Project
