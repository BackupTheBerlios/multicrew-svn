# Microsoft Developer Studio Project File - Name="LibStatic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=LibStatic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LibStatic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LibStatic.mak" CFG="LibStatic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LibStatic - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "LibStatic - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LibStatic - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /D "_WIN32_WINNT=0x0410" /D "_MBCS" /Gm PRECOMP_VC7_TOBEREMOVED /GZ /c /GX 
# ADD CPP /nologo /MTd /ZI /W3 /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /D "_WIN32_WINNT=0x0410" /D "_MBCS" /Gm PRECOMP_VC7_TOBEREMOVED /GZ /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Debug\..\..\RakNetLibStaticDebug.lib" 
# ADD LIB32 /nologo /out:"Debug\..\..\RakNetLibStaticDebug.lib" 

!ELSEIF  "$(CFG)" == "LibStatic - Win32 Release"

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
# ADD BASE CPP /nologo /MT /Zi /W3 /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_WIN32_WINNT=0x0410" /D "_MBCS" PRECOMP_VC7_TOBEREMOVED /c /GX 
# ADD CPP /nologo /MT /Zi /W3 /D "WIN32" /D "NDEBUG" /D "_LIB" /D "_WIN32_WINNT=0x0410" /D "_MBCS" PRECOMP_VC7_TOBEREMOVED /c /GX 
# ADD BASE MTL /nologo /win32 
# ADD MTL /nologo /win32 
# ADD BASE RSC /l 1033 
# ADD RSC /l 1033 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"Release\..\..\RakNetLibStatic.lib" 
# ADD LIB32 /nologo /out:"Release\..\..\RakNetLibStatic.lib" 

!ENDIF

# Begin Target

# Name "LibStatic - Win32 Debug"
# Name "LibStatic - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;def;odl;idl;hpj;bat;asm;asmx"
# Begin Source File

SOURCE=..\..\Source\AES128.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\AsynchronousFileIO.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\BitStream.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\CheckSum.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\DataBlockEncryptor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectManager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectStub.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\EncodeClassName.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\ExtendedOverlappedPool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\GetTime.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTreeFactory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\InternalPacketPool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\NetworkObject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\NetworkTypes.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\PacketPool.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakClient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakNetStatistics.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakNetworkFactory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakPeer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakServer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\rand.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\ReliabilityLayer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\RPCNode.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\SHA1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\SimpleMutex.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\SocketLayer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\StringCompressor.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;inc;xsd"
# Begin Source File

SOURCE=..\..\Source\AES128.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\ArrayList.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\AsynchronousFileIO.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\BigTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\BinarySearchTree.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\BitStream.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\CheckSum.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\ClientContextStruct.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\DataBlockEncryptor.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObject.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectHeader.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectManager.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectStub.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\EncodeClassName.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\ExtendedOverlappedPool.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\GetTime.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTree.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTreeFactory.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTreeNode.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\InternalPacket.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\InternalPacketPool.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\LinkedList.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\MTUSize.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Multiplayer.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\NetworkObject.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\NetworkTypes.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\PacketEnumerations.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\PacketPool.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\PacketPriority.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Queue.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\QueueLinkedList.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakClient.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakClientInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakNetStatistics.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakNetworkFactory.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakPeer.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakPeerInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakServer.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakServerInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Rand.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\ReliabilityLayer.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RPCNode.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\RSACrypt.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\SHA1.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\SimpleMutex.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\SocketLayer.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\StringCompressor.h
# End Source File
# Begin Source File

SOURCE=..\..\Source\Types.h
# End Source File
# End Group
# End Target
# End Project
