/* -*- mode: c++; c-file-style: raknet; tab-always-indent: nil; -*- */
/**
 * @file Rand.h 
 * @brief Random Number Generator 
 *
 * This file is part of RakNet Copyright 2003, 2004
 * Rakkarsoft LLC and Kevin Jenkins.
 *
 * Usage of Raknet is subject to the appropriate licence agreement.
 * "Shareware" Licensees with Rakkarsoft LLC are subject to the
 * shareware license found at
 * http://www.rakkarsoft.com/shareWareLicense.html which you agreed to
 * upon purchase of a "Shareware license" "Commercial" Licensees with
 * Rakkarsoft LLC are subject to the commercial license found at
 * http://www.rakkarsoft.com/sourceCodeLicense.html which you agreed
 * to upon purchase of a "Commercial license" All other users are
 * subject to the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Refer to the appropriate license agreement for distribution,
 * modification, and warranty rights.
 */

#ifndef __RAND_H
#define __RAND_H 
/**
 * Initialise seed for Random Generator
 * @param seed The initial value of the pseudo random suite. 
 */
extern void seedMT( unsigned long seed ); // Defined in cokus_c.c

/**
 * @todo Document this function 
 */
extern unsigned long reloadMT( void );

/**
 * Get next random value from the generator 
 * @return an integer random value. 
 */
extern unsigned long randomMT( void );

/**
 * Get next random value form the generator 
 * @return a real random value. 
 */
extern float frandomMT( void );

#endif
