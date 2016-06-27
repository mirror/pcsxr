#include "pgxp_value.h"


void MakeValid(PGXP_value *pV, u32 psxV)
{
	psx_value psx;
	psx.d = psxV;
	if (!pV->valid)
	{
		pV->x = psx.sw.l;
		pV->y = psx.sw.h;
		pV->z = 1.f;
		pV->valid = 1;
		pV->value = psx.d;
	}
}

void Validate(PGXP_value *pV, u32 psxV)
{
	// assume pV is not NULL
	pV->valid = (pV->valid) && (pV->value == psxV);
}

void MaskValidate(PGXP_value *pV, u32 psxV, u32 mask)
{
	// assume pV is not NULL
	pV->valid = (pV->valid) && ((pV->value & mask) == (psxV & mask));
}