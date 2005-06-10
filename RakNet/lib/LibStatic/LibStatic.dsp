# Microsoft Developer Studio Project File - Name="LibStatic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=LibStatic - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "LibStatic.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "LibStatic.mak" CFG="LibStatic - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "LibStatic - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "LibStatic - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
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
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /D _WIN32_WINNT=0x0410 /D "_MBCS" /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_LIB" /D _WIN32_WINNT=0x0410 /D "_MBCS" /GZ PRECOMP_VC7_TOBEREMOVED /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
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
MTL=midl.exe
# ADD BASE MTL /nologo /win32
# ADD MTL /nologo /win32
# ADD BASE CPP /nologo /MT /W3 /GX /Zi /D "WIN32" /D "NDEBUG" /D "_LIB" /D _WIN32_WINNT=0x0410 /D "_MBCS" PRECOMP_VC7_TOBEREMOVED /c
# ADD CPP /nologo /MD /W3 /GX /Zi /D "WIN32" /D "NDEBUG" /D "_LIB" /D _WIN32_WINNT=0x0410 /D "_MBCS" PRECOMP_VC7_TOBEREMOVED /c
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409
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
DEP_CPP_AES12=\
	"..\..\Source\AES128.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\AsynchronousFileIO.cpp
DEP_CPP_ASYNC=\
	"..\..\Source\AsynchronousFileIO.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\ExtendedOverlappedPool.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\SimpleMutex.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\BitStream.cpp
DEP_CPP_BITST=\
	"..\..\Source\BitStream.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\CheckSum.cpp
DEP_CPP_CHECK=\
	"..\..\Source\CheckSum.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\DataBlockEncryptor.cpp
DEP_CPP_DATAB=\
	"..\..\Source\AES128.h"\
	"..\..\Source\CheckSum.h"\
	"..\..\Source\DataBlockEncryptor.h"\
	"..\..\Source\GetTime.h"\
	"..\..\Source\Rand.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObject.cpp
DEP_CPP_DISTR=\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\DistributedNetworkObject.h"\
	"..\..\Source\DistributedNetworkObjectManager.h"\
	"..\..\Source\EncodeClassName.h"\
	"..\..\Source\GetTime.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\NetworkObject.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketEnumerations.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakClientInterface.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\RakServerInterface.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectManager.cpp
DEP_CPP_DISTRI=\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\DistributedNetworkObject.h"\
	"..\..\Source\DistributedNetworkObjectManager.h"\
	"..\..\Source\DistributedNetworkObjectStub.h"\
	"..\..\Source\EncodeClassName.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\NetworkObject.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketEnumerations.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakClientInterface.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\RakServerInterface.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\DistributedNetworkObjectStub.cpp
DEP_CPP_DISTRIB=\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\DistributedNetworkObjectManager.h"\
	"..\..\Source\DistributedNetworkObjectStub.h"\
	"..\..\Source\EncodeClassName.h"\
	"..\..\Source\NetworkTypes.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\EncodeClassName.cpp
DEP_CPP_ENCOD=\
	"..\..\Source\BitStream.h"\
	"..\..\Source\EncodeClassName.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\ExtendedOverlappedPool.cpp
DEP_CPP_EXTEN=\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\ExtendedOverlappedPool.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\SimpleMutex.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\GetTime.cpp
DEP_CPP_GETTI=\
	"..\..\Source\GetTime.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTree.cpp
DEP_CPP_HUFFM=\
	"..\..\Source\BitStream.h"\
	"..\..\Source\HuffmanEncodingTree.h"\
	"..\..\Source\HuffmanEncodingTreeNode.h"\
	"..\..\Source\LinkedList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\HuffmanEncodingTreeFactory.cpp
DEP_CPP_HUFFMA=\
	"..\..\Source\BitStream.h"\
	"..\..\Source\HuffmanEncodingTree.h"\
	"..\..\Source\HuffmanEncodingTreeFactory.h"\
	"..\..\Source\HuffmanEncodingTreeNode.h"\
	"..\..\Source\LinkedList.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\InternalPacketPool.cpp
DEP_CPP_INTER=\
	"..\..\Source\InternalPacket.h"\
	"..\..\Source\InternalPacketPool.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\SHA1.h"\
	"..\..\Source\SimpleMutex.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\NetworkObject.cpp
DEP_CPP_NETWO=\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\DistributedNetworkObjectManager.h"\
	"..\..\Source\EncodeClassName.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\NetworkObject.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakClientInterface.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\RakServerInterface.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\NetworkTypes.cpp
DEP_CPP_NETWOR=\
	"..\..\Source\NetworkTypes.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\PacketPool.cpp
DEP_CPP_PACKE=\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketPool.h"\
	"..\..\Source\SimpleMutex.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakClient.cpp
DEP_CPP_RAKCL=\
	"..\..\Source\AES128.h"\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BigTypes.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\DataBlockEncryptor.h"\
	"..\..\Source\GetTime.h"\
	"..\..\Source\InternalPacket.h"\
	"..\..\Source\InternalPacketPool.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketEnumerations.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakClient.h"\
	"..\..\Source\RakClientInterface.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakPeer.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\Rand.h"\
	"..\..\Source\ReliabilityLayer.h"\
	"..\..\Source\RPCNode.h"\
	"..\..\Source\RSACrypt.h"\
	"..\..\Source\SHA1.h"\
	"..\..\Source\SimpleMutex.h"\
	"..\..\Source\SocketLayer.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakNetStatistics.cpp
DEP_CPP_RAKNE=\
	"..\..\Source\BitStream.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\RakNetStatistics.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakNetworkFactory.cpp
DEP_CPP_RAKNET=\
	"..\..\Source\AES128.h"\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BigTypes.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\DataBlockEncryptor.h"\
	"..\..\Source\InternalPacket.h"\
	"..\..\Source\InternalPacketPool.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakClient.h"\
	"..\..\Source\RakClientInterface.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakNetworkFactory.h"\
	"..\..\Source\RakPeer.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\RakServer.h"\
	"..\..\Source\RakServerInterface.h"\
	"..\..\Source\Rand.h"\
	"..\..\Source\ReliabilityLayer.h"\
	"..\..\Source\RPCNode.h"\
	"..\..\Source\RSACrypt.h"\
	"..\..\Source\SHA1.h"\
	"..\..\Source\SimpleMutex.h"\
	"..\..\Source\SocketLayer.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakPeer.cpp
DEP_CPP_RAKPE=\
	"..\..\Source\AES128.h"\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\AsynchronousFileIO.h"\
	"..\..\Source\BigTypes.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\DataBlockEncryptor.h"\
	"..\..\Source\GetTime.h"\
	"..\..\Source\HuffmanEncodingTree.h"\
	"..\..\Source\HuffmanEncodingTreeNode.h"\
	"..\..\Source\InternalPacket.h"\
	"..\..\Source\InternalPacketPool.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketEnumerations.h"\
	"..\..\Source\PacketPool.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakPeer.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\Rand.h"\
	"..\..\Source\ReliabilityLayer.h"\
	"..\..\Source\RPCNode.h"\
	"..\..\Source\RSACrypt.h"\
	"..\..\Source\SHA1.h"\
	"..\..\Source\SimpleMutex.h"\
	"..\..\Source\SocketLayer.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\RakServer.cpp
DEP_CPP_RAKSE=\
	"..\..\Source\AES128.h"\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BigTypes.h"\
	"..\..\Source\BinarySearchTree.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\DataBlockEncryptor.h"\
	"..\..\Source\GetTime.h"\
	"..\..\Source\InternalPacket.h"\
	"..\..\Source\InternalPacketPool.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketEnumerations.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\QueueLinkedList.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\RakPeer.h"\
	"..\..\Source\RakPeerInterface.h"\
	"..\..\Source\RakServer.h"\
	"..\..\Source\RakServerInterface.h"\
	"..\..\Source\Rand.h"\
	"..\..\Source\ReliabilityLayer.h"\
	"..\..\Source\RPCNode.h"\
	"..\..\Source\RSACrypt.h"\
	"..\..\Source\SHA1.h"\
	"..\..\Source\SimpleMutex.h"\
	"..\..\Source\SocketLayer.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\rand.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Source\ReliabilityLayer.cpp
DEP_CPP_RELIA=\
	"..\..\Source\AES128.h"\
	"..\..\Source\ArrayList.h"\
	"..\..\Source\BitStream.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\DataBlockEncryptor.h"\
	"..\..\Source\GetTime.h"\
	"..\..\Source\InternalPacket.h"\
	"..\..\Source\InternalPacketPool.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\PacketPriority.h"\
	"..\..\Source\RakNetStatistics.h"\
	"..\..\Source\ReliabilityLayer.h"\
	"..\..\Source\SHA1.h"\
	"..\..\Source\SimpleMutex.h"\
	"..\..\Source\SocketLayer.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\RPCNode.cpp
DEP_CPP_RPCNO=\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\RPCNode.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\SHA1.cpp
DEP_CPP_SHA1_=\
	"..\..\Source\SHA1.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\SimpleMutex.cpp
DEP_CPP_SIMPL=\
	"..\..\Source\SimpleMutex.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\SocketLayer.cpp
DEP_CPP_SOCKE=\
	"..\..\Source\AsynchronousFileIO.h"\
	"..\..\Source\ClientContextStruct.h"\
	"..\..\Source\ExtendedOverlappedPool.h"\
	"..\..\Source\MTUSize.h"\
	"..\..\Source\NetworkTypes.h"\
	"..\..\Source\SimpleMutex.h"\
	"..\..\Source\SocketLayer.h"\
	
# End Source File
# Begin Source File

SOURCE=..\..\Source\StringCompressor.cpp
DEP_CPP_STRIN=\
	"..\..\Source\BitStream.h"\
	"..\..\Source\HuffmanEncodingTree.h"\
	"..\..\Source\HuffmanEncodingTreeNode.h"\
	"..\..\Source\LinkedList.h"\
	"..\..\Source\StringCompressor.h"\
	
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
