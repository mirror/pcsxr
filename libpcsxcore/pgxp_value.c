#include "pgxp_value.h"


void MakeValid(PGXP_value *pV, u32 psxV)
{
	psx_value psx;
	psx.d = psxV;
	if (VALID_01 != (pV->flags & VALID_01))
	{
		pV->x = psx.sw.l;
		pV->y = psx.sw.h;
		pV->z = 1.f;
		pV->flags |= VALID_ALL;
		pV->value = psx.d;
	}
}

void Validate(PGXP_value *pV, u32 psxV)
{
	// assume pV is not NULL
	pV->flags &= pV->value == psxV ? ALL : INV_VALID_ALL;
}

void MaskValidate(PGXP_value *pV, u32 psxV, u32 mask, u32 validMask)
{
	// assume pV is not NULL
	pV->flags &= ((pV->value & mask) == (psxV & mask)) ? ALL : (ALL ^ (validMask));
}