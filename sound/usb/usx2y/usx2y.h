/*
 * Driver for Tascam US-X2Y USB soundcards
 *
 * Copyright (c) 2003 by Karsten Wiese <annabellesgarden@yahoo.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef __SOUND_USX2Y_COMMON_H
#define __SOUND_USX2Y_COMMON_H


#define USX2Y_DRIVER_VERSION	0x0100	/*       */


/*                 */
#define SND_USX2Y_LOADER_ID		"USX2Y Loader"
#define SND_USX2Y_USBPCM_ID		"USX2Y USBPCM"

/*               */
enum {
	USX2Y_TYPE_122,
	USX2Y_TYPE_224,
	USX2Y_TYPE_428,
	USX2Y_TYPE_NUMS
};

#define USB_ID_US122 0x8007
#define USB_ID_US224 0x8005
#define USB_ID_US428 0x8001

/*             */
enum {
	USX2Y_STAT_CHIP_INIT	=	(1 << 0),	/*                 */
	USX2Y_STAT_CHIP_MMAP_PCM_URBS = (1 << 1),	/*                                */
	USX2Y_STAT_CHIP_HUP	=	(1 << 31),	/*                 */
};

#endif /*                        */
