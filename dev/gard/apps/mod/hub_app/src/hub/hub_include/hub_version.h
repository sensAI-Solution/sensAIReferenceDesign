/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_VERSION_H__
#define __HUB_VERSION_H__

#define TO_STR(x)          #x
#define STR(x)             TO_STR(x)

/* Make changes for versioning here - do not change any other lines */
#define HUB_VERSION_MAJOR  1
#define HUB_VERSION_MINOR  5
#define HUB_VERSION_BUGFIX 0
/* Make changes for versioning here - do not change any other lines */

#define STR_MAJOR          STR(HUB_VERSION_MAJOR)
#define STR_MINOR          STR(HUB_VERSION_MINOR)
#define STR_BUGFIX         STR(HUB_VERSION_BUGFIX)

#define JOIN_SYMBOL(_1, _2, _3, symbol)                                        \
	_1 STR(symbol)                                                             \
	_2 STR(symbol) _3

#define HUB_VERSION_STRING JOIN_SYMBOL(STR_MAJOR, STR_MINOR, STR_BUGFIX, .)

#endif /* __HUB_VERSION_H__ */