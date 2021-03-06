/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file HuffmanEncodingTreeNode.h
 * 
 * This file is part of RakNet Copyright 2003, 2004 Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the shareware license found at http://www.rakkarsoft.com/shareWareLicense.html which you agreed to upon purchase of a "Shareware license"
 * "Commercial" Licensees with Rakkarsoft LLC are subject to the commercial license found at http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed to upon purchase of a "Commercial license"
 * All other users are subject to the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
 *
 * Refer to the appropriate license agreement for distribution, modification, and warranty rights.
 */

#ifndef __HUFFMAN_ENCODING_TREE_NODE
#define __HUFFMAN_ENCODING_TREE_NODE

/**
 * @todo BasicDataStructure namespace maybe 
 * This structure define a node eof an Huffman Tree 
 * 
 */

struct HuffmanEncodingTreeNode
{
	/**
	 * Node value 
	 */
	unsigned char value;
	/**
	 * Node weight 
	 */
	unsigned weight;
	/**
	 * Left Child Node
	 */
	HuffmanEncodingTreeNode *left;
	/**
	 * Right Child Node 
	 */
	HuffmanEncodingTreeNode *right;
	/**
	 * Parent Node 
	 */
	HuffmanEncodingTreeNode *parent;
};

#endif
