/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub.h"
#include "hub_utils.h"

/**
 * Read a given file into memory
 *
 * On success, malloc's memory equal to the file size in p_file's
 * p_file_content variable
 *
 * This memory has to be freed by the caller when not needed.
 *
 * @param: p_file is the HUB file context variable
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_READ_FILE_INTO_MEM on failure
 */
enum hub_ret_code hub_read_file_into_memory(struct hub_file_ctx *p_file)
{
	int    retval;
	long   file_size;
	size_t file_size_read;
	FILE  *p_fp;

	p_fp = fopen(p_file->p_file_name, "r");
	if (NULL == p_fp) {
		hub_pr_err("Error opening file %s\n", p_file->p_file_name);
		goto read_file_err_1;
	}

	retval = fseek(p_fp, 0L, SEEK_END);
	(void)retval;
	file_size = ftell(p_fp);
	retval    = fseek(p_fp, 0L, SEEK_SET);
	(void)retval;

	p_file->p_file_content = (char *)malloc(file_size * sizeof(char));
	if (NULL == p_file->p_file_content) {
		hub_pr_err("Error allocating buffer for reading %s\n",
				   p_file->p_file_name);
		goto read_file_err_2;
	}

	file_size_read =
		fread(p_file->p_file_content, sizeof(char), file_size, p_fp);
	if (file_size_read != file_size) {
		hub_pr_err("Error reading file %s\n", p_file->p_file_name);
		goto read_file_err_3;
	}

	p_file->file_size = file_size;
	fclose(p_fp);

	return HUB_SUCCESS;

read_file_err_3:
	free(p_file->p_file_content);
read_file_err_2:
	fclose(p_fp);
read_file_err_1:
	return HUB_FAILURE_READ_FILE_INTO_MEM;
}

/**
 * Parse a given JSON file using the cJSON library
 *
 * Malloc's memory on pp_json_parsed
 *
 * Has to be freed by the called when not needed.
 *
 * @param: p_file is the json file to be parsed
 * @param: pp_json_parsed is the pointer a memory filled in after the parsing
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_PARSE_JSON on failure
 */
enum hub_ret_code hub_parse_json(struct hub_file_ctx *p_file,
								 cJSON              **pp_json_parsed)
{
	const char *p_json_err_ptr;

	*pp_json_parsed =
		cJSON_ParseWithLength(p_file->p_file_content, p_file->file_size);
	if (NULL == *pp_json_parsed) {
		hub_pr_err("Error parsing %s", p_file->p_file_name);

		/* JSON parse error, find and print the error point */
		p_json_err_ptr = cJSON_GetErrorPtr();
		if (NULL != p_json_err_ptr) {
			hub_pr_err(", before: %s\n", p_json_err_ptr);
		} else {
			hub_pr_err("\n");
		}
		return HUB_FAILURE_PARSE_JSON;
	}

	return HUB_SUCCESS;
}