/*
 * cb2_crypto.c -- Functions for decrypting/encrypting cheat codes for
 * CodeBreaker PS2 (V1 + V7)
 *
 * Copyright (C) 2006-2007 misfire
 * Copyright (C) 2008 Pyriel
 * All rights reserved.
 *
 * This file is part of Omniconvert.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* 
 * Misfire created most of the code in this file, and the heart of the
 * crypt routines.  I (Pyriel) added some wrappers for batch decryption.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "abbtypes.h"
#include "arcfour.h"
#include "big_int_full.h"
#include "cb2_crypto.h"


/**
 * The old, crappy CB V1 encryption...
 */

// V1 seed tables
static const u32 seedtable[3][16] = {
	{
		0x0A0B8D9B, 0x0A0133F8, 0x0AF733EC, 0x0A15C574,
		0x0A50AC20, 0x0A920FB9, 0x0A599F0B, 0x0A4AA0E3,
		0x0A21C012, 0x0A906254, 0x0A31FD54, 0x0A091C0E,
		0x0A372B38, 0x0A6F266C, 0x0A61DD4A, 0x0A0DBF92
	},
	{
		0x00288596, 0x0037DD28, 0x003BEEF1, 0x000BC822,
		0x00BC935D, 0x00A139F2, 0x00E9BBF8, 0x00F57F7B,
		0x0090D704, 0x001814D4, 0x00C5848E, 0x005B83E7,
		0x00108CF7, 0x0046CE5A, 0x003A5BF4, 0x006FAFFC
	},
	{
		0x1DD9A10A, 0xB95AB9B0, 0x5CF5D328, 0x95FE7F10,
		0x8E2D6303, 0x16BB6286, 0xE389324C, 0x07AC6EA8,
		0xAA4811D8, 0x76CE4E18, 0xFE447516, 0xF9CD94D0,
		0x4C24DEDB, 0x68275C4E, 0x72494382, 0xC8AA88E8
	}
};

/*
 * Encrypts a V1 code.
 */
void CB1EncryptCode(u32 *addr, u32 *val)
{
	u32 tmp;
	u8 cmd = *addr >> 28;

	tmp = *addr & 0xFF000000;
	*addr = ((*addr & 0xFF) << 16) | ((*addr >> 8) & 0xFFFF);
	*addr = (tmp | ((*addr + seedtable[1][cmd]) & 0x00FFFFFF)) ^ seedtable[0][cmd];

	if (cmd > 2) *val = *addr ^ (*val + seedtable[2][cmd]);
}

/*
 * Decrypts a V1 code.
 */
void CB1DecryptCode(u32 *addr, u32 *val)
{
	u32 tmp;
	u8 cmd = *addr >> 28;

	if (cmd > 2) *val = (*addr ^ *val) - seedtable[2][cmd];

	tmp = *addr ^ seedtable[0][cmd];
	*addr = tmp - seedtable[1][cmd];
	*addr = (tmp & 0xFF000000) | ((*addr & 0xFFFF) << 8) | ((*addr >> 16) & 0xFF);
}


/**
 * The new CB V7 encryption...
 */

// Default seed tables (1280 bytes total)
static const u8 defseeds[5][256] = {
	{
		0x84, 0x01, 0x21, 0xA4, 0xFA, 0x4D, 0x50, 0x8D, 0x75, 0x33, 0xC5, 0xF7, 0x4A, 0x6D, 0x7C, 0xA6,
		0x1C, 0xF8, 0x40, 0x18, 0xA1, 0xB3, 0xA2, 0xF9, 0x6A, 0x19, 0x63, 0x66, 0x29, 0xAE, 0x10, 0x75,
		0x84, 0x7D, 0xEC, 0x6A, 0xF9, 0x2D, 0x8E, 0x33, 0x44, 0x5C, 0x33, 0x6D, 0x78, 0x3E, 0x1B, 0x6C,
		0x02, 0xE0, 0x7D, 0x77, 0x1D, 0xB1, 0x61, 0x2A, 0xCD, 0xC1, 0x38, 0x53, 0x1F, 0xA1, 0x6E, 0x3D,
		0x03, 0x0D, 0x05, 0xDC, 0x50, 0x19, 0x85, 0x89, 0x9B, 0xF1, 0x8A, 0xC2, 0xD1, 0x5C, 0x22, 0xC4,
		0x11, 0x29, 0xF6, 0x13, 0xEC, 0x06, 0xE4, 0xBD, 0x08, 0x9E, 0xB7, 0x8D, 0x72, 0x92, 0x10, 0x3C,
		0x41, 0x4E, 0x81, 0x55, 0x08, 0x9C, 0xA3, 0xBC, 0xA1, 0x79, 0xB0, 0x7A, 0x94, 0x3A, 0x39, 0x95,
		0x7A, 0xC6, 0x96, 0x21, 0xB0, 0x07, 0x17, 0x5E, 0x53, 0x54, 0x08, 0xCF, 0x85, 0x6C, 0x4B, 0xBE,
		0x30, 0x82, 0xDD, 0x1D, 0x3A, 0x24, 0x3C, 0xB2, 0x67, 0x0C, 0x36, 0x03, 0x51, 0x60, 0x3F, 0x67,
		0xF1, 0xB2, 0x77, 0xDC, 0x12, 0x9D, 0x7B, 0xCE, 0x65, 0xF8, 0x75, 0xEA, 0x23, 0x63, 0x99, 0x54,
		0x37, 0xC0, 0x3C, 0x42, 0x77, 0x12, 0xB7, 0xCA, 0x54, 0xF1, 0x26, 0x1D, 0x1E, 0xD1, 0xAB, 0x2C,
		0xAF, 0xB6, 0x91, 0x2E, 0xBD, 0x84, 0x0B, 0xF2, 0x1A, 0x1E, 0x26, 0x1E, 0x00, 0x12, 0xB7, 0x77,
		0xD6, 0x61, 0x1C, 0xCE, 0xA9, 0x10, 0x19, 0xAA, 0x88, 0xE6, 0x35, 0x29, 0x32, 0x5F, 0x57, 0xA7,
		0x94, 0x93, 0xA1, 0x2B, 0xEB, 0x9B, 0x17, 0x2A, 0xAA, 0x60, 0xD5, 0x19, 0xB2, 0x4E, 0x5A, 0xE2,
		0xC9, 0x4A, 0x00, 0x68, 0x6E, 0x59, 0x36, 0xA6, 0xA0, 0xF9, 0x19, 0xA2, 0xC7, 0xC9, 0xD4, 0x29,
		0x5C, 0x99, 0x3C, 0x5C, 0xE2, 0xCB, 0x94, 0x40, 0x8B, 0xF4, 0x3B, 0xD2, 0x38, 0x7D, 0xBF, 0xD0
	},
	{
		0xCC, 0x6D, 0x5D, 0x0B, 0x70, 0x25, 0x5D, 0x68, 0xFE, 0xBE, 0x6C, 0x3F, 0xA4, 0xD9, 0x95, 0x5F,
		0x30, 0xAE, 0x34, 0x39, 0x00, 0x89, 0xDC, 0x5A, 0xC8, 0x82, 0x24, 0x3A, 0xFC, 0xDA, 0x3C, 0x1F,
		0x73, 0x3F, 0x63, 0xAA, 0x53, 0xBD, 0x4E, 0xB5, 0x33, 0x48, 0x59, 0xC1, 0xB7, 0xE0, 0x0C, 0x99,
		0xEC, 0x3B, 0x32, 0x26, 0xB3, 0xB1, 0xE2, 0x8E, 0x54, 0x41, 0x55, 0xDB, 0x1D, 0x90, 0x0B, 0x48,
		0xF3, 0x3F, 0xCA, 0x1F, 0x19, 0xEB, 0x7F, 0x56, 0x52, 0xD7, 0x20, 0x67, 0x59, 0x4F, 0x4E, 0xDC,
		0xBB, 0x6A, 0x8E, 0x45, 0x88, 0x0B, 0x93, 0xAC, 0xCD, 0x0E, 0x29, 0x18, 0x7A, 0x16, 0x8D, 0x8D,
		0xC2, 0x88, 0x6A, 0x9D, 0x39, 0xF4, 0x93, 0x14, 0xCD, 0xE0, 0x6B, 0xC7, 0x28, 0x21, 0x5C, 0x97,
		0x70, 0x7C, 0xAB, 0x53, 0x46, 0x33, 0x03, 0x18, 0xDF, 0x91, 0xFE, 0x06, 0xC0, 0xFF, 0xA2, 0x58,
		0xF3, 0xB0, 0x6B, 0x9B, 0x71, 0x91, 0x23, 0xDA, 0x92, 0x67, 0x14, 0x34, 0x9F, 0xA5, 0xAF, 0x65,
		0x62, 0xE8, 0x7F, 0x79, 0x35, 0x32, 0x29, 0x3E, 0x4F, 0xDC, 0xC7, 0x8E, 0xF1, 0x21, 0x9D, 0x3B,
		0x61, 0xFC, 0x0B, 0x02, 0xEC, 0xE4, 0xA7, 0xEA, 0x77, 0xE7, 0x21, 0x63, 0x97, 0x7F, 0x23, 0x8A,
		0x8B, 0xBE, 0x4E, 0x90, 0xC0, 0x89, 0x04, 0x44, 0x90, 0x57, 0x41, 0xB5, 0x74, 0xAD, 0xB1, 0xE9,
		0xF3, 0x91, 0xC7, 0x27, 0x3E, 0x00, 0x81, 0x99, 0xEE, 0x38, 0xF5, 0x32, 0x4F, 0x27, 0x4F, 0x64,
		0x39, 0x3D, 0xD3, 0x0B, 0x99, 0xD5, 0x99, 0xD6, 0x10, 0x4B, 0x43, 0x17, 0x38, 0x34, 0x54, 0x63,
		0x19, 0x36, 0xBD, 0x15, 0xB1, 0x06, 0x1E, 0xDE, 0x1B, 0xAF, 0xEB, 0xFA, 0x56, 0xB8, 0x8D, 0x9D,
		0x14, 0x1A, 0xA6, 0x49, 0x56, 0x19, 0xCA, 0xC1, 0x40, 0x6D, 0x71, 0xDE, 0x68, 0xC1, 0xC3, 0x4A
	},
	{
		0x69, 0x31, 0x5C, 0xAB, 0x7F, 0x5B, 0xE9, 0x81, 0x32, 0x58, 0x32, 0x0A, 0x97, 0xF3, 0xC7, 0xCF,
		0xBB, 0x1D, 0xCF, 0x0E, 0x83, 0x35, 0x4C, 0x58, 0xCE, 0xF7, 0x8A, 0xE4, 0xB0, 0xE4, 0x83, 0x48,
		0x81, 0x77, 0x7C, 0x3F, 0xBC, 0x27, 0x3A, 0x1B, 0xA4, 0xE9, 0x06, 0xA4, 0x15, 0xAB, 0x90, 0x10,
		0x7D, 0x74, 0xDA, 0xFC, 0x36, 0x09, 0xCC, 0xF7, 0x12, 0xB6, 0xF4, 0x94, 0xE9, 0x8B, 0x6A, 0x3B,
		0x5E, 0x71, 0x46, 0x3E, 0x0B, 0x78, 0xAD, 0x3B, 0x94, 0x5B, 0x89, 0x85, 0xA3, 0xE0, 0x01, 0xEB,
		0x84, 0x41, 0xAA, 0xD7, 0xB3, 0x17, 0x16, 0xC3, 0x6C, 0xB1, 0x81, 0x73, 0xEC, 0xE4, 0x6E, 0x09,
		0x56, 0xEE, 0x7A, 0xF6, 0x75, 0x6A, 0x73, 0x95, 0x8D, 0xDA, 0x51, 0x63, 0x8B, 0xBB, 0xE0, 0x4D,
		0xF8, 0xA0, 0x27, 0xF2, 0x9F, 0xC8, 0x15, 0x5A, 0x23, 0x85, 0x58, 0x04, 0x4A, 0x57, 0x28, 0x20,
		0x6D, 0x9D, 0x85, 0x83, 0x3C, 0xBF, 0x02, 0xB0, 0x96, 0xE8, 0x73, 0x6F, 0x20, 0x6E, 0xB0, 0xE4,
		0xC6, 0xFA, 0x71, 0xA6, 0x5D, 0xC5, 0xA0, 0xA3, 0xF8, 0x5C, 0x99, 0xCB, 0x9C, 0x04, 0x3A, 0xB2,
		0x04, 0x8D, 0xA2, 0x9D, 0x32, 0xF0, 0xBD, 0xAA, 0xEA, 0x81, 0x79, 0xE2, 0xA1, 0xBA, 0x89, 0x12,
		0xD5, 0x9F, 0x81, 0xEB, 0x63, 0xE7, 0xE5, 0xD4, 0xE9, 0x0E, 0x30, 0xBC, 0xCB, 0x70, 0xDD, 0x51,
		0x77, 0xC0, 0x80, 0xB3, 0x49, 0x03, 0x9A, 0xB8, 0x8C, 0xA7, 0x63, 0x62, 0x8F, 0x72, 0x5C, 0xA6,
		0xA0, 0xCF, 0x4F, 0xB4, 0x86, 0xFD, 0x49, 0xFA, 0x4A, 0x85, 0xDB, 0xFE, 0x61, 0xB7, 0x3A, 0xD7,
		0x83, 0x70, 0x57, 0x49, 0x83, 0xA7, 0x10, 0x73, 0x74, 0x37, 0x87, 0xFD, 0x6B, 0x28, 0xB7, 0x31,
		0x1E, 0x54, 0x1C, 0xE9, 0xD0, 0xB1, 0xCA, 0x76, 0x3B, 0x21, 0xF7, 0x67, 0xBB, 0x48, 0x69, 0x39
	},
	{
		0x8D, 0xD1, 0x8C, 0x7B, 0x83, 0x8C, 0xA8, 0x18, 0xA7, 0x4A, 0x14, 0x03, 0x88, 0xB3, 0xCE, 0x74,
		0xBF, 0x5B, 0x87, 0x67, 0xA7, 0x85, 0x6B, 0x62, 0x96, 0x7C, 0xA9, 0xA6, 0xF6, 0x9E, 0xF4, 0x73,
		0xC5, 0xC4, 0xB0, 0x2B, 0x73, 0x2E, 0x36, 0x77, 0xDF, 0xBA, 0x57, 0xFF, 0x7F, 0xE9, 0x84, 0xE1,
		0x8D, 0x7B, 0xA2, 0xEF, 0x4F, 0x10, 0xF3, 0xD3, 0xE8, 0xB4, 0xBA, 0x20, 0x28, 0x79, 0x18, 0xD6,
		0x0F, 0x1C, 0xAA, 0xBD, 0x0E, 0x45, 0xF7, 0x6C, 0x68, 0xB9, 0x29, 0x40, 0x1A, 0xCF, 0xB6, 0x0A,
		0x13, 0xF8, 0xC0, 0x9C, 0x87, 0x10, 0x36, 0x14, 0x73, 0xA1, 0x75, 0x27, 0x14, 0x55, 0xAF, 0x78,
		0x9A, 0x08, 0xC9, 0x05, 0xF2, 0xEC, 0x24, 0x1B, 0x07, 0x4A, 0xDC, 0xF6, 0x48, 0xC6, 0x25, 0xCD,
		0x12, 0x1D, 0xAF, 0x51, 0x8F, 0xE9, 0xCA, 0x2C, 0x80, 0x57, 0x78, 0xB7, 0x96, 0x07, 0x19, 0x77,
		0x6E, 0x16, 0x45, 0x47, 0x8E, 0x9C, 0x18, 0x55, 0xF1, 0x72, 0xB3, 0x8A, 0xEA, 0x4E, 0x8D, 0x90,
		0x2E, 0xBC, 0x08, 0xAC, 0xF6, 0xA0, 0x5C, 0x16, 0xE3, 0x7A, 0xEE, 0x67, 0xB8, 0x58, 0xDC, 0x16,
		0x40, 0xED, 0xF9, 0x18, 0xB3, 0x0E, 0xD8, 0xEE, 0xE1, 0xFA, 0xC3, 0x9F, 0x82, 0x99, 0x32, 0x41,
		0x34, 0xBE, 0xC9, 0x50, 0x36, 0xE5, 0x66, 0xAA, 0x0D, 0x43, 0xF0, 0x3F, 0x26, 0x7C, 0xF3, 0x87,
		0x26, 0xA4, 0xF5, 0xF8, 0xA0, 0x32, 0x46, 0x74, 0x2E, 0x5A, 0xE2, 0xE7, 0x6B, 0x02, 0xA8, 0xD0,
		0xCF, 0xB8, 0x33, 0x15, 0x3B, 0x4F, 0xC7, 0x7A, 0xE8, 0x3D, 0x75, 0xD2, 0xFE, 0x42, 0x22, 0x22,
		0xA8, 0x21, 0x33, 0xFB, 0xB0, 0x87, 0x92, 0x99, 0xCA, 0xD7, 0xD7, 0x88, 0xAC, 0xE4, 0x75, 0x83,
		0x56, 0xBF, 0xCE, 0xED, 0x4F, 0xF6, 0x22, 0x07, 0xCA, 0xBC, 0xD2, 0xEF, 0x1B, 0x75, 0xD6, 0x2D
	},
	{
		0xD2, 0x4F, 0x76, 0x51, 0xEB, 0xA1, 0xAD, 0x84, 0xD6, 0x19, 0xE6, 0x97, 0xD9, 0xD3, 0x58, 0x6B,
		0xFB, 0xB8, 0x20, 0xFD, 0x49, 0x56, 0x1B, 0x50, 0x61, 0x10, 0x57, 0xB8, 0x78, 0x07, 0xC1, 0x4A,
		0xA2, 0xEA, 0x47, 0x80, 0x00, 0x4A, 0xB3, 0x4E, 0x6F, 0x1A, 0xC1, 0xD5, 0x22, 0xF8, 0x54, 0x2F,
		0x33, 0xE5, 0x7F, 0xB4, 0x13, 0x02, 0xA3, 0xA1, 0x8B, 0x1C, 0x6F, 0x19, 0xD6, 0x42, 0xB3, 0x24,
		0x4B, 0x04, 0x30, 0x10, 0x02, 0x23, 0x6F, 0x10, 0x03, 0x4B, 0x0E, 0x33, 0x55, 0x22, 0xA4, 0x78,
		0xEC, 0xD2, 0x4A, 0x11, 0x8B, 0xFC, 0xFF, 0x14, 0x7A, 0xED, 0x06, 0x47, 0x86, 0xFC, 0xF0, 0x03,
		0x0F, 0x75, 0x07, 0xE4, 0x9A, 0xD3, 0xBB, 0x0D, 0x97, 0x1F, 0x6F, 0x80, 0x62, 0xA6, 0x9E, 0xC6,
		0xB1, 0x10, 0x81, 0xA1, 0x6D, 0x55, 0x0F, 0x9E, 0x1B, 0xB7, 0xF5, 0xDC, 0x62, 0xA8, 0x63, 0x58,
		0xCF, 0x2F, 0x6A, 0xAD, 0x5E, 0xD3, 0x3F, 0xBD, 0x8D, 0x9B, 0x2A, 0x8B, 0xDF, 0x60, 0xB9, 0xAF,
		0xAA, 0x70, 0xB4, 0xA8, 0x17, 0x99, 0x72, 0xB9, 0x88, 0x9D, 0x3D, 0x2A, 0x11, 0x87, 0x1E, 0xF3,
		0x9D, 0x33, 0x8D, 0xED, 0x52, 0x60, 0x36, 0x71, 0xFF, 0x7B, 0x37, 0x84, 0x3D, 0x27, 0x9E, 0xD9,
		0xDF, 0x58, 0xF7, 0xC2, 0x58, 0x0C, 0x9D, 0x5E, 0xEE, 0x23, 0x83, 0x70, 0x3F, 0x95, 0xBC, 0xF5,
		0x42, 0x86, 0x91, 0x5B, 0x3F, 0x77, 0x31, 0xD2, 0xB7, 0x09, 0x59, 0x53, 0xF5, 0xF2, 0xE5, 0xF1,
		0xDC, 0x92, 0x83, 0x14, 0xC1, 0xA2, 0x25, 0x62, 0x13, 0xFD, 0xD4, 0xC5, 0x54, 0x9D, 0x9C, 0x27,
		0x6C, 0xC2, 0x75, 0x8B, 0xBC, 0xC7, 0x4E, 0x0A, 0xF6, 0x5C, 0x2F, 0x12, 0x8E, 0x25, 0xBB, 0xF2,
		0x5F, 0x89, 0xAA, 0xEA, 0xD9, 0xCD, 0x05, 0x74, 0x20, 0xD6, 0x17, 0xED, 0xF0, 0x66, 0x6C, 0x7B
	}
};

// Default ARCFOUR key (20 bytes)
static const u32 defkey[5] = {
	0xD0DBA9D7, 0x13A0A96C, 0x80410DF0, 0x2CCDBE1F, 0xE570A86B
};

// RSA parameters
static const u64 rsa_modulus = 18446744073709551605ULL; // = 0xFFFFFFFFFFFFFFF5
static const u64 rsa_dec_key = 11;
/*
 * This is how I calculated the encryption key e from d:
 * (some number theory)
 *
 *	d = 11
 *	n = 18446744073709551605
 *	e = d^(-1) mod phi(n)
 *
 *	n factored:
 *	n = 5 * 2551 * 1446236305269271
 *	  = p*q*r, only single prime factors
 *
 *	phi(n) = phi(p*q*r)
 *	       = phi(p) * phi(q) * phi(r), phi(p) = p - 1
 *	       = (p-1)*(q-1)*(r-1)
 *	       = (5-1) * (2551-1) * (1446236305269271-1)
 *	       = 4 * 2550 * 1446236305269270
 *	       = 14751610313746554000
 *
 *	e = 11^(-1) mod 14751610313746554000
 *	e = 2682110966135737091
 */
static const u64 rsa_enc_key = 2682110966135737091ULL;

static u8 seeds[5][256];	// Current set of seeds
static u32 key[5];		// Current ARCFOUR key
static u32 oldkey[5];		// Backup of ARCFOUR key
static ARC4_CTX ctx;		// ARCFOUR context
static int v7enc;		// Flag: Use V7 encryption?
static int v7init;		// Flag: V7 encryption already initialized?
static int beefcodf;		// Flag: Is it BEEFC0DF?
//u32 unkwn;

u32 MulInverse(u32 word);
u32 MulEncrypt(u32 a, u32 b);
u32 MulDecrypt(u32 a, u32 b);
void RSACrypt(u32 *addr, u32 *val, u64 rsakey);
void CB7Beefcode(int init, u32 val);
void CB7EncryptCode(u32 *addr, u32 *val);
void CB7DecryptCode(u32 *addr, u32 *val);
int CB7SelfTest(void);
void CBReset(void);
void CBEncryptCode(u32 *addr, u32 *val);
void CBDecryptCode(u32 *addr, u32 *val);

/*
 * Computes the multiplicative inverse of @word, modulo (2^32).
 * I think this is borrowed from IDEA. ;)
 */
u32 MulInverse(u32 word)
{
	// Original MIPS R5900 coding converted to C
	u32 a0, a1, a2, a3;
	u32 v0, v1;
	u32 t1;

	if (word == 1) return 1;

	a2 = (0 - word) % word;
	if (!a2) return 1;

	t1 = 1;
	a3 = word;
	a0 = 0 - (0xFFFFFFFF / word);

	do {
		v0 = a3 / a2;
		v1 = a3 % a2;
		a1 = a2;
		a3 = a1;
		a1 = a0;
		a2 = v1;
		v0 = v0 * a1;
		a0 = t1 - v0;
		t1 = a1;
	} while (a2);

	return t1;
}

/*
 * Multiplication, modulo (2^32)
 */
u32 MulEncrypt(u32 a, u32 b)
{
	return (a * (b | 1));
}

/*
 * Multiplication with multiplicative inverse, modulo (2^32)
 */
u32 MulDecrypt(u32 a, u32 b)
{
	return (a * MulInverse(b | 1));
}

/*
 * RSA encryption/decryption
 *
 * NOTE: Uses the excellent BIG_INT library by
 * Alexander Valyalkin (valyala@gmail.com)
 */
void RSACrypt(u32 *addr, u32 *val, u64 rsakey)
{
	big_int *code, *exp, *mod;
	int cmp_flag;

	// Setup big_int number for code
	code = big_int_create(2);
	code->len = 2;
	code->num[0] = *val;
	code->num[1] = *addr;

	// Setup modulus
	mod = big_int_create(2);
	mod->len = 2;
	*(u64*)mod->num = rsa_modulus;

	// Exponentiation is only invertible if code < modulus
	big_int_cmp(code, mod, &cmp_flag);
	if (cmp_flag < 0) {
		// Setup exponent
		exp = big_int_create(2);
		exp->len = 2;
		*(u64*)exp->num = rsakey;

		// Encryption: c = m^e mod n
		// Decryption: m = c^d mod n
		big_int_powmod(code, exp, mod, code);

		// Save result
		*addr = code->num[1];
		*val  = code->num[0];

		// Deallocate big_int number
		big_int_destroy(exp);
	}

	big_int_destroy(code);
	big_int_destroy(mod);
}

/*
 * Used to generate/change the encryption key and seeds.
 * "Beefcode" is the new V7+ seed code:
 * BEEFC0DE VVVVVVVV, where VVVVVVVV = val.
 */
void CB7Beefcode(int init, u32 val)
{
	int i;
	u8 *p = (u8*)&val; // Easy access to all bytes of val

	// Setup key and seeds
	if (init) {
		beefcodf = 0;
		//unkwn = 0;
		memcpy(key, defkey, sizeof(defkey));

		if (val) {
			memcpy(seeds, defseeds, sizeof(defseeds));
			key[0] = (seeds[3][p[3]] << 24) | (seeds[2][p[2]] << 16) | (seeds[1][p[1]] << 8) | seeds[0][p[0]];
			key[1] = (seeds[0][p[3]] << 24) | (seeds[3][p[2]] << 16) | (seeds[2][p[1]] << 8) | seeds[1][p[0]];
			key[2] = (seeds[1][p[3]] << 24) | (seeds[0][p[2]] << 16) | (seeds[3][p[1]] << 8) | seeds[2][p[0]];
			key[3] = (seeds[2][p[3]] << 24) | (seeds[1][p[2]] << 16) | (seeds[0][p[1]] << 8) | seeds[3][p[0]];
		} else {
			memset(seeds, 0, sizeof(seeds));
		}
	} else {
		if (val) {
			key[0] = (seeds[3][p[3]] << 24) | (seeds[2][p[2]] << 16) | (seeds[1][p[1]] << 8) | seeds[0][p[0]];
			key[1] = (seeds[0][p[3]] << 24) | (seeds[3][p[2]] << 16) | (seeds[2][p[1]] << 8) | seeds[1][p[0]];
			key[2] = (seeds[1][p[3]] << 24) | (seeds[0][p[2]] << 16) | (seeds[3][p[1]] << 8) | seeds[2][p[0]];
			key[3] = (seeds[2][p[3]] << 24) | (seeds[1][p[2]] << 16) | (seeds[0][p[1]] << 8) | seeds[3][p[0]];
		} else {
			memset(seeds, 0, sizeof(seeds));
			key[0] = 0;
			key[1] = 0;
			key[2] = 0;
			key[3] = 0;
		}
	}

	// Use key to encrypt seeds with ARCFOUR algorithm
	for (i = 0; i < 5; i++) {
		// Setup ARCFOUR context with key
		ARC4Init(&ctx, (u8*)key, 20);
		// Encrypt seeds
		ARC4Crypt(&ctx, &seeds[i][0], 256);
		// Encrypt original key for next round
		ARC4Crypt(&ctx, (u8*)key, 20);
	}

	// Backup key
	memcpy(oldkey, key, sizeof(key));
}

/*
 * Encrypts a V7+ code.
 */
void CB7EncryptCode(u32 *addr, u32 *val)
{
	int i;
	u32 code[2];
	u32 oldaddr, oldval;

	oldaddr = *addr;
	oldval  = *val;

	// Step 1: Multiplication, modulo (2^32)
	*addr = MulEncrypt(*addr, oldkey[0] - oldkey[1]);
	*val  = MulEncrypt(*val,  oldkey[2] + oldkey[3]);

	// Step 2: ARCFOUR
	code[0] = *addr;
	code[1] = *val;
	ARC4Init(&ctx, (u8*)key, 20);
	ARC4Crypt(&ctx, (u8*)code, 8);
	*addr = code[0];
	*val  = code[1];

	// Step 3: RSA
	RSACrypt(addr, val, rsa_enc_key);

	// Step 4: Encryption loop of 64 cycles, using the generated seeds
	for (i = 0; i <= 63; i++) {
		*addr = ((*addr + ((u32*)seeds[2])[i]) ^ ((u32*)seeds[0])[i]) - (*val ^ ((u32*)seeds[4])[i]);
		*val  = ((*val - ((u32*)seeds[3])[i]) ^ ((u32*)seeds[1])[i]) + (*addr ^ ((u32*)seeds[4])[i]);
	}

	// BEEFC0DE
	if ((oldaddr & 0xFFFFFFFE) == 0xBEEFC0DE) {
		CB7Beefcode(0, oldval);
		//beefcodf = 1;
		return;
	}

	// BEEFC0DF
	if (beefcodf) {
		code[0] = oldaddr;
		code[1] = oldval;
		ARC4Init(&ctx, (u8*)code, 8);
		ARC4Crypt(&ctx, (u8*)seeds, sizeof(seeds));
		beefcodf = 0;
		//unkwn = 0;
		return;
	}
}

/*
 * Decrypts a V7+ code.
 */
void CB7DecryptCode(u32 *addr, u32 *val)
{
	int i;
	u32 code[2];

	// Step 1: Decryption loop of 64 cycles, using the generated seeds
	for (i = 63; i >= 0; i--) {
		*val  = ((*val - (*addr ^ ((u32*)seeds[4])[i])) ^ ((u32*)seeds[1])[i]) + ((u32*)seeds[3])[i];
		*addr = ((*addr + (*val ^ ((u32*)seeds[4])[i])) ^ ((u32*)seeds[0])[i]) - ((u32*)seeds[2])[i];
	}

	// Step 2: RSA
	RSACrypt(addr, val, rsa_dec_key);

	// Step 3: ARCFOUR
	code[0] = *addr;
	code[1] = *val;
	ARC4Init(&ctx, (u8*)key, 20);
	ARC4Crypt(&ctx, (u8*)code, 8);
	*addr = code[0];
	*val  = code[1];

	// Step 4: Multiplication with multiplicative inverse, modulo (2^32)
	*addr = MulDecrypt(*addr, oldkey[0] - oldkey[1]);
	*val  = MulDecrypt(*val,  oldkey[2] + oldkey[3]);

	// BEEFC0DF
	if (beefcodf) {
		code[0] = *addr;
		code[1] = *val;
		ARC4Init(&ctx, (u8*)code, 8);
		ARC4Crypt(&ctx, (u8*)seeds, sizeof(seeds));
		beefcodf = 0;
		//unkwn = 0;
		return;
	}

	// BEEFC0DE
	if ((*addr & 0xFFFFFFFE) == 0xBEEFC0DE) {
		CB7Beefcode(0, *val);
		//beefcodf = 1;
		return;
	}
/*
	if (unkwn) {
		key[0] = *addr ^ oldkey[0];
		key[1] = *val  ^ oldkey[1];
		key[2] = *addr - oldkey[2];
		key[3] = *val  + oldkey[3];
		key[4] = (*addr - *val) ^ oldkey[4];
	}
*/
}

/*
 * Checks if V7 encryption/decryption works properly.
 */
#define NUM_TESTCODES	3

int CB7SelfTest(void)
{
	static const u32 testcodes[NUM_TESTCODES*2] = {
		0x000FFFFE, 0x0000007D,
		0x90175B28, 0x0C061A24,
		0x20323260, 0xFFFFFFFF
	};
	u32 addr, val;
	int i;

	// Generate some random seeds
	CB7Beefcode(1, (u32)rand());
	CB7Beefcode(0, (u32)rand());

	// Check if D(E(M)) = M
	for (i = 0; i < NUM_TESTCODES; i++) {
		addr = testcodes[i*2];
		val  = testcodes[i*2+1];

		CB7EncryptCode(&addr, &val);
		CB7DecryptCode(&addr, &val);

		if ((addr != testcodes[i*2]) || (val != testcodes[i*2+1]))
			return -1;
	}

	return 0;
}


/**
 * Common functions for both V1 and V7.
 */

/*
 * Resets the CB encryption.  Must be called before processing
 * a code list using CBEncryptCode() or CBDecryptCode()!
 */
void CBReset(void)
{
	// Clear flags
	v7enc = 0;
	v7init = 0;
	beefcodf = 0;
}

/*
 * Set common CB V7 encryption (B4336FA9 4DFEFB79) which is used by CMGSCCC.com
 */
void CBSetCommonV7(void)
{
	v7enc = 1;
	CB7Beefcode(1, 0);
	v7init = 1;
	beefcodf = 0;
}

/*
 * Used to encrypt a list of CB codes (V1 + V7).
 */
void CBEncryptCode(u32 *addr, u32 *val)
{
	u32 oldaddr, oldval;

	oldaddr = *addr;
	oldval  = *val;

	if (v7enc)
		CB7EncryptCode(addr, val);
	else
		CB1EncryptCode(addr, val);

	if ((oldaddr & 0xFFFFFFFE) == 0xBEEFC0DE) {
		if (!v7init) {
			// Init seeds
			CB7Beefcode(1, oldval);
			v7init = 1;
		} else {
			// Change original seeds
			CB7Beefcode(0, oldval);
		}
		v7enc = 1;
		beefcodf = oldaddr & 1;
	}
}

/*
 * Used to decrypt a list of CB codes (V1 + V7).
 */
void CBDecryptCode(u32 *addr, u32 *val)
{
	if (v7enc)
		CB7DecryptCode(addr, val);
	else
		CB1DecryptCode(addr, val);

	if ((*addr & 0xFFFFFFFE) == 0xBEEFC0DE) {
		if (!v7init) {
			// Init seeds
			CB7Beefcode(1, *val);
			v7init = 1;
		} else {
			// Change original seeds
			CB7Beefcode(0, *val);
		}
		v7enc = 1;
		beefcodf = *addr & 1;
	}
}

/*
 * Wrapper functions for encrypting and decrypting batches of code lines,
 * i.e. an entire cheat.
 */
void CBBatchDecrypt(cheat_t *cheat) {
	int i, was_beefcode = 0, num = 0;
	u32 *code = cheat->code;
	for(i = 0; i < cheat->codecnt; i+=2) {
		CBDecryptCode(&code[i], &code[i+1]);
		if((code[i] & 0xFFFFFFFE) == 0xBEEFC0DE) { 
			was_beefcode = 1;
			cheatRemoveOctets(cheat, i + 1, 2);
			num = 0;
			i-=2;
		} else if (was_beefcode) {
			if(code[i] == 0x000FFFFE || code[i] == 0x000FFFFF) {
				num++;
				cheatRemoveOctets(cheat, i + 1, 2);
				i-=2;
			} else was_beefcode = 0;
			if(num >= 2) was_beefcode = 0;
		}
	}
}

void CBBatchEncrypt(cheat_t *cheat) {
	int i, num = cheat->codecnt;
	u32 *code = cheat->code;
	for(i = 0; i < num; i+=2) {
		CBEncryptCode(&code[i], &code[i+1]);
	}
}

/**
 * CBC/PCB file functions
 */

/* 1024-byte ARCFOUR key used to encrypt/decrypt CBC and PCB files */
static const u8 filekey[1024] = {
	0x2B, 0xF3, 0x2C, 0x6A, 0x73, 0x33, 0xCC, 0xD6, 0x01, 0x8F, 0x28, 0x26, 0xF0, 0xD6, 0xAF, 0xBF,
	0xEB, 0x7C, 0xCF, 0x96, 0xAD, 0x40, 0x35, 0x16, 0xB1, 0x84, 0x8D, 0x29, 0x08, 0x86, 0x78, 0xE5,
	0x06, 0x86, 0x7D, 0xCC, 0xA5, 0x45, 0x9D, 0x26, 0xB5, 0x0E, 0x97, 0x87, 0xCB, 0x45, 0xEA, 0x61,
	0xC0, 0xC2, 0x13, 0xEB, 0x44, 0x34, 0xB8, 0xB7, 0x17, 0xFB, 0x2A, 0x06, 0xB4, 0x19, 0xB3, 0xAD,
	0x3F, 0x98, 0xEB, 0xCC, 0xF1, 0x21, 0xB5, 0x89, 0x82, 0x37, 0x52, 0xAF, 0x19, 0xF6, 0xF9, 0x55,
	0x25, 0x2E, 0xF0, 0x6B, 0xB2, 0x7A, 0x37, 0x24, 0xDA, 0x0F, 0xA6, 0x50, 0x9E, 0xF0, 0x13, 0x66,
	0xB9, 0x60, 0xB7, 0x31, 0x3D, 0x9F, 0x89, 0xE3, 0x10, 0x40, 0x04, 0xEA, 0x73, 0x87, 0x33, 0x5D,
	0xFA, 0xA8, 0x47, 0x6B, 0x5B, 0xF2, 0x7D, 0x64, 0x04, 0x2E, 0x82, 0x4E, 0x1C, 0x19, 0x7E, 0x42,
	0xC6, 0xE0, 0x0D, 0x5E, 0x1D, 0x2C, 0xBD, 0x39, 0xD5, 0x13, 0x5D, 0x3F, 0x58, 0xF1, 0x7C, 0xA1,
	0x27, 0x44, 0xE6, 0xD6, 0x34, 0xF3, 0x9F, 0x8D, 0x61, 0x12, 0x78, 0xFA, 0xD8, 0x3C, 0x04, 0xA8,
	0x50, 0x09, 0xEE, 0x59, 0xEC, 0x5A, 0xD4, 0x4F, 0xFB, 0x2B, 0xB4, 0xD5, 0xBE, 0x4E, 0xD2, 0x8D,
	0xD6, 0x07, 0x26, 0xAC, 0x24, 0x11, 0x1D, 0x5B, 0x7C, 0xD4, 0xEC, 0xD4, 0x1D, 0xBE, 0xB1, 0x6E,
	0x72, 0x93, 0x0E, 0x37, 0xCE, 0x97, 0xCD, 0x8D, 0xAA, 0xB4, 0xAA, 0x48, 0x0D, 0xED, 0xB2, 0x94,
	0x25, 0xDF, 0x70, 0x1E, 0xB2, 0x34, 0xFE, 0xDF, 0x2E, 0xF7, 0x8D, 0x4C, 0x08, 0x6F, 0xE0, 0xE8,
	/*
	 * The following bytes of the key in ASCII:
	 *
	 *	Copyright 2003 � Pelican Accessories, Inc., CMGSCCC.com & Smiley
	 *
	 *	  A BIG shoutout (/sarcasm) goes out to the losers who set us
	 *	back 9 months from kicking ass in the cheat market (you know who
	 *	you are). The sad part for you is that we are back now full steam
	 *	ahead, ready to kick ass and take names later. We are taking no
	 *	prisoners, so prepare to die slow. We already put one fish dead
	 *	in the water, whos next?
	 *	 -CMX
	 *
	 *	    A wiseman once said:
	 *
	 *	Will there ever be peace, or are we all just
	 *	headed for doom, still consumed by the beast
	 *	And I know - there'll never be peace
	 *	That's why I keep my pistol when I walk the streets
	 *	Cause there could never be peace
	 *
	 *	(The views expressed above are those of CMX and not the views of
	 *	Pelican Accessories, Inc. or www.codebreaker.com)
	 */
	0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x32, 0x30, 0x30, 0x33, 0x20, 0xA9,
	0x20, 0x50, 0x65, 0x6C, 0x69, 0x63, 0x61, 0x6E, 0x20, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x6F,
	0x72, 0x69, 0x65, 0x73, 0x2C, 0x20, 0x49, 0x6E, 0x63, 0x2E, 0x2C, 0x20, 0x43, 0x4D, 0x47, 0x53,
	0x43, 0x43, 0x43, 0x2E, 0x63, 0x6F, 0x6D, 0x20, 0x26, 0x20, 0x53, 0x6D, 0x69, 0x6C, 0x65, 0x79,
	0x0D, 0x0A, 0x0D, 0x0A, 0x20, 0x20, 0x41, 0x20, 0x42, 0x49, 0x47, 0x20, 0x73, 0x68, 0x6F, 0x75,
	0x74, 0x6F, 0x75, 0x74, 0x20, 0x28, 0x2F, 0x73, 0x61, 0x72, 0x63, 0x61, 0x73, 0x6D, 0x29, 0x20,
	0x67, 0x6F, 0x65, 0x73, 0x20, 0x6F, 0x75, 0x74, 0x20, 0x74, 0x6F, 0x20, 0x74, 0x68, 0x65, 0x20,
	0x6C, 0x6F, 0x73, 0x65, 0x72, 0x73, 0x20, 0x77, 0x68, 0x6F, 0x20, 0x73, 0x65, 0x74, 0x20, 0x75,
	0x73, 0x0D, 0x0A, 0x62, 0x61, 0x63, 0x6B, 0x20, 0x39, 0x20, 0x6D, 0x6F, 0x6E, 0x74, 0x68, 0x73,
	0x20, 0x66, 0x72, 0x6F, 0x6D, 0x20, 0x6B, 0x69, 0x63, 0x6B, 0x69, 0x6E, 0x67, 0x20, 0x61, 0x73,
	0x73, 0x20, 0x69, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x68, 0x65, 0x61, 0x74, 0x20, 0x6D,
	0x61, 0x72, 0x6B, 0x65, 0x74, 0x20, 0x28, 0x79, 0x6F, 0x75, 0x20, 0x6B, 0x6E, 0x6F, 0x77, 0x20,
	0x77, 0x68, 0x6F, 0x0D, 0x0A, 0x79, 0x6F, 0x75, 0x20, 0x61, 0x72, 0x65, 0x29, 0x2E, 0x20, 0x54,
	0x68, 0x65, 0x20, 0x73, 0x61, 0x64, 0x20, 0x70, 0x61, 0x72, 0x74, 0x20, 0x66, 0x6F, 0x72, 0x20,
	0x79, 0x6F, 0x75, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68, 0x61, 0x74, 0x20, 0x77, 0x65, 0x20, 0x61,
	0x72, 0x65, 0x20, 0x62, 0x61, 0x63, 0x6B, 0x20, 0x6E, 0x6F, 0x77, 0x20, 0x66, 0x75, 0x6C, 0x6C,
	0x20, 0x73, 0x74, 0x65, 0x61, 0x6D, 0x0D, 0x0A, 0x61, 0x68, 0x65, 0x61, 0x64, 0x2C, 0x20, 0x72,
	0x65, 0x61, 0x64, 0x79, 0x20, 0x74, 0x6F, 0x20, 0x6B, 0x69, 0x63, 0x6B, 0x20, 0x61, 0x73, 0x73,
	0x20, 0x61, 0x6E, 0x64, 0x20, 0x74, 0x61, 0x6B, 0x65, 0x20, 0x6E, 0x61, 0x6D, 0x65, 0x73, 0x20,
	0x6C, 0x61, 0x74, 0x65, 0x72, 0x2E, 0x20, 0x57, 0x65, 0x20, 0x61, 0x72, 0x65, 0x20, 0x74, 0x61,
	0x6B, 0x69, 0x6E, 0x67, 0x20, 0x6E, 0x6F, 0x0D, 0x0A, 0x70, 0x72, 0x69, 0x73, 0x6F, 0x6E, 0x65,
	0x72, 0x73, 0x2C, 0x20, 0x73, 0x6F, 0x20, 0x70, 0x72, 0x65, 0x70, 0x61, 0x72, 0x65, 0x20, 0x74,
	0x6F, 0x20, 0x64, 0x69, 0x65, 0x20, 0x73, 0x6C, 0x6F, 0x77, 0x2E, 0x20, 0x57, 0x65, 0x20, 0x61,
	0x6C, 0x72, 0x65, 0x61, 0x64, 0x79, 0x20, 0x70, 0x75, 0x74, 0x20, 0x6F, 0x6E, 0x65, 0x20, 0x66,
	0x69, 0x73, 0x68, 0x20, 0x64, 0x65, 0x61, 0x64, 0x0D, 0x0A, 0x69, 0x6E, 0x20, 0x74, 0x68, 0x65,
	0x20, 0x77, 0x61, 0x74, 0x65, 0x72, 0x2C, 0x20, 0x77, 0x68, 0x6F, 0x73, 0x20, 0x6E, 0x65, 0x78,
	0x74, 0x3F, 0x0D, 0x0A, 0x20, 0x2D, 0x43, 0x4D, 0x58, 0x0D, 0x0A, 0x0D, 0x0A, 0x20, 0x20, 0x20,
	0x20, 0x41, 0x20, 0x77, 0x69, 0x73, 0x65, 0x6D, 0x61, 0x6E, 0x20, 0x6F, 0x6E, 0x63, 0x65, 0x20,
	0x73, 0x61, 0x69, 0x64, 0x3A, 0x0D, 0x0A, 0x0D, 0x0A, 0x57, 0x69, 0x6C, 0x6C, 0x20, 0x74, 0x68,
	0x65, 0x72, 0x65, 0x20, 0x65, 0x76, 0x65, 0x72, 0x20, 0x62, 0x65, 0x20, 0x70, 0x65, 0x61, 0x63,
	0x65, 0x2C, 0x20, 0x6F, 0x72, 0x20, 0x61, 0x72, 0x65, 0x20, 0x77, 0x65, 0x20, 0x61, 0x6C, 0x6C,
	0x20, 0x6A, 0x75, 0x73, 0x74, 0x0D, 0x0A, 0x68, 0x65, 0x61, 0x64, 0x65, 0x64, 0x20, 0x66, 0x6F,
	0x72, 0x20, 0x0D, 0x64, 0x6F, 0x6F, 0x6D, 0x2C, 0x20, 0x73, 0x74, 0x69, 0x6C, 0x6C, 0x20, 0x63,
	0x6F, 0x6E, 0x73, 0x75, 0x6D, 0x65, 0x64, 0x20, 0x62, 0x79, 0x20, 0x74, 0x68, 0x65, 0x20, 0x62,
	0x65, 0x61, 0x73, 0x74, 0x0D, 0x0A, 0x41, 0x6E, 0x64, 0x20, 0x49, 0x20, 0x6B, 0x6E, 0x6F, 0x77,
	0x20, 0x2D, 0x20, 0x74, 0x68, 0x65, 0x72, 0x65, 0x27, 0x6C, 0x6C, 0x20, 0x6E, 0x65, 0x76, 0x65,
	0x72, 0x20, 0x62, 0x65, 0x20, 0x70, 0x65, 0x61, 0x63, 0x65, 0x0D, 0x0A, 0x54, 0x68, 0x61, 0x74,
	0x27, 0x73, 0x20, 0x77, 0x68, 0x79, 0x20, 0x49, 0x20, 0x6B, 0x65, 0x65, 0x70, 0x20, 0x6D, 0x79,
	0x20, 0x70, 0x69, 0x73, 0x74, 0x6F, 0x6C, 0x20, 0x77, 0x68, 0x65, 0x6E, 0x20, 0x49, 0x20, 0x77,
	0x61, 0x6C, 0x6B, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x74, 0x72, 0x65, 0x65, 0x74, 0x73, 0x0D,
	0x0A, 0x43, 0x61, 0x75, 0x73, 0x65, 0x20, 0x74, 0x68, 0x65, 0x72, 0x65, 0x20, 0x63, 0x6F, 0x75,
	0x6C, 0x64, 0x20, 0x6E, 0x65, 0x76, 0x65, 0x72, 0x20, 0x62, 0x65, 0x20, 0x70, 0x65, 0x61, 0x63,
	0x65, 0x0D, 0x0A, 0x0D, 0x0A, 0x28, 0x54, 0x68, 0x65, 0x20, 0x76, 0x69, 0x65, 0x77, 0x73, 0x20,
	0x65, 0x78, 0x70, 0x72, 0x65, 0x73, 0x73, 0x65, 0x64, 0x20, 0x61, 0x62, 0x6F, 0x76, 0x65, 0x20,
	0x61, 0x72, 0x65, 0x20, 0x74, 0x68, 0x6F, 0x73, 0x65, 0x20, 0x6F, 0x66, 0x20, 0x43, 0x4D, 0x58,
	0x20, 0x61, 0x6E, 0x64, 0x20, 0x6E, 0x6F, 0x74, 0x20, 0x74, 0x68, 0x65, 0x20, 0x76, 0x69, 0x65,
	0x77, 0x73, 0x20, 0x6F, 0x66, 0x0D, 0x0A, 0x50, 0x65, 0x6C, 0x69, 0x63, 0x61, 0x6E, 0x20, 0x41,
	0x63, 0x63, 0x65, 0x73, 0x73, 0x6F, 0x72, 0x69, 0x65, 0x73, 0x2C, 0x20, 0x49, 0x6E, 0x63, 0x2E,
	0x20, 0x6F, 0x72, 0x20, 0x77, 0x77, 0x77, 0x2E, 0x63, 0x6F, 0x64, 0x65, 0x62, 0x72, 0x65, 0x61,
	0x6B, 0x65, 0x72, 0x2E, 0x63, 0x6F, 0x6D, 0x29, 0x0A, 0x00, 0x00, 0x00, 0xBE, 0xEF, 0xBE, 0xEF
};

/* Encrypt/decrypt CBC/PCB file data */
void CBCryptFileData(u8 *data, int datasize)
{
	ARC4_CTX ctx;

	ARC4Init(&ctx, filekey, sizeof(filekey));
	ARC4Crypt(&ctx, data, datasize);
}
