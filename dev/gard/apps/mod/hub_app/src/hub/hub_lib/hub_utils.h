/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <cjson/cJSON.h>

#include "gard_info.h"

/*
	Various utility functions used by HUB
*/

/**
 * Structure containing the context of a file
 * HUB uses this for various file operations
 * such as reading a file into memory, writing
 * memory buffers to a file on disk, etc.
 *
 * Contains:
 * 1. The full name of the said file
 * 2. A pointer to a buffer containing file content
 * 3. The size of the file / buffer in bytes
 */
struct hub_file_ctx {
	char *p_file_name;
	char *p_file_content;
	long  file_size;
};

/* Read a given file into memory */
enum hub_ret_code hub_read_file_into_memory(struct hub_file_ctx *p_file);

/* Parse a given JSON file using the cJSON library */
enum hub_ret_code hub_parse_json(struct hub_file_ctx *p_file,
								 cJSON              **pp_json_parsed);

#endif /* __UTILS_H__ */