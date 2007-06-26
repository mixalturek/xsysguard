/* endian.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>

/******************************************************************************/

static int am_big_endian(void) {
	long one = 1;
	return !(*((char *)(&one)));
}

int main(int argc, char **argv) {
	printf("#define __LITTLE_ENDIAN 1234\n");
	printf("#define __BIG_ENDIAN    4321\n");
	printf("#define __BYTE_ORDER __%s_ENDIAN\n", am_big_endian() ? "BIG" : "LITTLE");
	return 0;
}

