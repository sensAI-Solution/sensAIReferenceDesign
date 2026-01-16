/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "assert.h"

/**
 * gard_assert_failed() will print the user message to the UART and LOG.
 * The function will wait in an endless loop since the assumption is that
 * the system cannot continue further in a sane manner.
 *
 * @param assert_expr 	contains the failed assertion expression as a string
 * @param filename 		points to the name of the file from where this function
 *                 		is called
 * @param lineno 		is the line number in the file from where this function
 *               		is called
 *
 * @return Nothing
 */
void gard_assert_failed(const i8 *assert_expr,
						const i8 *filename,
						const u32 lineno,
						...)
{
	/**
	 * <TBD-SRP> Currently this function is implementedas an endless while loop.
	 * In future we may implement this to write a backtrace to the Flash for
	 * post-mortem analysis.
	 */
	(void)assert_expr;
	(void)filename;
	(void)lineno;

	while (true) {
		/* Endless loop */
	}
}
