/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// byteswap.c
//

#include "shared.h"

static qBool	bs_inited = qFalse;
static qBool	bs_hostLittle = qTrue;

/*
===============
_FloatSwap
===============
*/
static float _FloatSwap (float f)
{
	union {
		byte	b[4];
		float	f;
	} in, out;
	
	in.f = f;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];
	
	return out.f;
}


/*
===============
_FloatNoSwap
===============
*/
static float _FloatNoSwap (float f)
{
	return f;
}


/*
===============
_LongSwap
===============
*/
static int _LongSwap (int l)
{
	union {
		byte	b[4];
		int		l;
	} in, out;

	in.l = l;

	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.l;
}


/*
===============
_LongNoSwap
===============
*/
static int _LongNoSwap (int l)
{
	return l;
}


/*
===============
_ShortSwap
===============
*/
static int16 _ShortSwap (int16 s)
{
	union {
		byte	b[2];
		int16	s;
	} in, out;

	in.s = s;

	out.b[0] = in.b[1];
	out.b[1] = in.b[0];

	return out.s;
}


/*
===============
_ShortNoSwap
===============
*/
static int16 _ShortNoSwap (int16 s)
{
	return s;
}

static inline void BS_EnsureInit (void)
{
	if (!bs_inited)
		Swap_Init ();
}

float LittleFloat (float f)
{
	BS_EnsureInit ();
	return bs_hostLittle ? _FloatNoSwap (f) : _FloatSwap (f);
}

int LittleLong (int l)
{
	BS_EnsureInit ();
	return bs_hostLittle ? _LongNoSwap (l) : _LongSwap (l);
}

int16 LittleShort (int16 s)
{
	BS_EnsureInit ();
	return bs_hostLittle ? _ShortNoSwap (s) : _ShortSwap (s);
}

float BigFloat (float f)
{
	BS_EnsureInit ();
	return bs_hostLittle ? _FloatSwap (f) : _FloatNoSwap (f);
}

int BigLong (int l)
{
	BS_EnsureInit ();
	return bs_hostLittle ? _LongSwap (l) : _LongNoSwap (l);
}

int16 BigShort (int16 s)
{
	BS_EnsureInit ();
	return bs_hostLittle ? _ShortSwap (s) : _ShortNoSwap (s);
}


/*
===============
Swap_Init
===============
*/
void Swap_Init (void)
{
	byte	swapTest[2] = { 1, 0 };

	bs_hostLittle = (*(int16 *)swapTest == 1) ? qTrue : qFalse;
	bs_inited = qTrue;
}
