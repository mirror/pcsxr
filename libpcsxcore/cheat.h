/*  Cheat Support for PCSX-Reloaded
 *
 *  Copyright (C) 2009, Wei Mingzhi <whistler@openoffice.org>.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA
 */

#ifndef CHEAT_H
#define CHEAT_H

typedef struct {
	uint32_t	Addr;
	uint16_t	Val;
} CheatCode;

typedef struct {
	char		*Descr;
	int			First;		// index of the first cheat code
	int			n;			// number of cheat codes for this cheat
	int			Enabled;
} Cheat;

void ClearAllCheats();

void LoadCheats(const char *filename);
void SaveCheats(const char *filename);

void ApplyCheats();

int AddCheat(const char *descr, char *code);
void RemoveCheat(int index);
int EditCheat(int index, const char *descr, char *code);

extern Cheat *Cheats;
extern CheatCode *CheatCodes;
extern int NumCheats;
extern int NumCodes;

#endif
