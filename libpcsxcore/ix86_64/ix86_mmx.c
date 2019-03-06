// stop compiling if NORECBUILD build (only for Visual Studio)

#ifdef __x86_64__

#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include "ix86-64.h"

#include <assert.h>

/********************/
/* MMX instructions */
/********************/

// r64 = mm

/* movq m64 to r64 */
void MOVQMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x6F), true, to, from, 0);
}

/* movq r64 to m64 */
void MOVQRtoM( uptr to, x86MMXRegType from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x7F), true, from, to, 0);
}

/* pand r64 to r64 */
void PANDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xDB0F );
	ModRM( 3, to, from ); 
}

void PANDNRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xDF0F );
	ModRM( 3, to, from ); 
}

/* por r64 to r64 */
void PORRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xEB0F );
	ModRM( 3, to, from ); 
}

/* pxor r64 to r64 */
void PXORRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xEF0F );
	ModRM( 3, to, from ); 
}

/* psllq r64 to r64 */
void PSLLQRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xF30F );
	ModRM( 3, to, from ); 
}

/* psllq m64 to r64 */
void PSLLQMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xF3), true, to, from, 0);
}

/* psllq imm8 to r64 */
void PSLLQItoR( x86MMXRegType to, u8 from ) 
{
	RexB(0, to);
	write16( 0x730F ); 
	ModRM( 3, 6, to); 
	write8( from ); 
}

/* psrlq r64 to r64 */
void PSRLQRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xD30F ); 
	ModRM( 3, to, from ); 
}

/* psrlq m64 to r64 */
void PSRLQMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xD3), true, to, from, 0);
}

/* psrlq imm8 to r64 */
void PSRLQItoR( x86MMXRegType to, u8 from ) 
{
	RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 2, to); 
	write8( from ); 
}

/* paddusb r64 to r64 */
void PADDUSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xDC0F ); 
	ModRM( 3, to, from ); 
}

/* paddusb m64 to r64 */
void PADDUSBMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xDC), true, to, from, 0);
}

/* paddusw r64 to r64 */
void PADDUSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xDD0F ); 
	ModRM( 3, to, from ); 
}

/* paddusw m64 to r64 */
void PADDUSWMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xDD), true, to, from, 0);
}

/* paddb r64 to r64 */
void PADDBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xFC0F ); 
	ModRM( 3, to, from ); 
}

/* paddb m64 to r64 */
void PADDBMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xFC), true, to, from, 0);
}

/* paddw r64 to r64 */
void PADDWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xFD0F ); 
	ModRM( 3, to, from ); 
}

/* paddw m64 to r64 */
void PADDWMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xFD), true, to, from, 0);
}

/* paddd r64 to r64 */
void PADDDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xFE0F ); 
	ModRM( 3, to, from ); 
}

/* paddd m64 to r64 */
void PADDDMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xFE), true, to, from, 0);
}

/* emms */
void EMMS( void ) 
{
	write16( 0x770F );
}

void PADDSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xEC0F ); 
	ModRM( 3, to, from ); 
}

void PADDSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xED0F );
	ModRM( 3, to, from ); 
}

// paddq m64 to r64 (sse2 only?)
void PADDQMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xD4), true, to, from, 0);
}

// paddq r64 to r64 (sse2 only?)
void PADDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xD40F ); 
	ModRM( 3, to, from ); 
}

void PSUBSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xE80F ); 
	ModRM( 3, to, from ); 
}

void PSUBSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xE90F );
	ModRM( 3, to, from ); 
}


void PSUBBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xF80F ); 
	ModRM( 3, to, from ); 
}

void PSUBWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xF90F ); 
	ModRM( 3, to, from ); 
}

void PSUBDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xFA0F ); 
	ModRM( 3, to, from ); 
}

void PSUBDMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xFA), true, to, from, 0);
}

void PSUBUSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xD80F ); 
	ModRM( 3, to, from ); 
}

void PSUBUSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	RexRB(0, to, from);
	write16( 0xD90F ); 
	ModRM( 3, to, from ); 
}

// psubq m64 to r64 (sse2 only?)
void PSUBQMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xFB), true, to, from, 0);
}

// psubq r64 to r64 (sse2 only?)
void PSUBQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xFB0F ); 
	ModRM( 3, to, from ); 
}

// pmuludq m64 to r64 (sse2 only?)
void PMULUDQMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xF4), true, to, from, 0);
}

// pmuludq r64 to r64 (sse2 only?)
void PMULUDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xF40F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x740F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x750F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x760F ); 
	ModRM( 3, to, from ); 
}

void PCMPEQDMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x76), true, to, from, 0);
}

void PCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x640F ); 
	ModRM( 3, to, from ); 
}

void PCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x650F ); 
	ModRM( 3, to, from ); 
}

void PCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x660F ); 
	ModRM( 3, to, from ); 
}

void PCMPGTDMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x66), true, to, from, 0);
}

void PSRLWItoR( x86MMXRegType to, u8 from )
{
	RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 2 , to ); 
	write8( from );
}

void PSRLDItoR( x86MMXRegType to, u8 from )
{
	RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 2 , to ); 
	write8( from );
}

void PSRLDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xD20F );
	ModRM( 3, to, from ); 
}

void PSLLWItoR( x86MMXRegType to, u8 from )
{
	RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 6 , to ); 
	write8( from );
}

void PSLLDItoR( x86MMXRegType to, u8 from )
{
	RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 6 , to ); 
	write8( from );
}

void PSLLDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xF20F );
	ModRM( 3, to, from ); 
}

void PSRAWItoR( x86MMXRegType to, u8 from )
{
	RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 4 , to ); 
	write8( from );
}

void PSRADItoR( x86MMXRegType to, u8 from )
{
	RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 4 , to ); 
	write8( from );
}

void PSRADRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0xE20F );
	ModRM( 3, to, from ); 
}

/* por m64 to r64 */
void PORMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xEB), true, to, from, 0);
}

/* pxor m64 to r64 */
void PXORMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xEF), true, to, from, 0);
}

/* pand m64 to r64 */
void PANDMtoR( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xDB), true, to, from, 0);
}

void PANDNMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0xDF), true, to, from, 0);
}

void PUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x6A0F );
	ModRM( 3, to, from );
}

void PUNPCKHDQMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x6A), true, to, from, 0);
}

void PUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x620F );
	ModRM( 3, to, from );
}

void PUNPCKLDQMtoR( x86MMXRegType to, uptr from )
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x62), true, to, from, 0);
}

void MOVQ64ItoR( x86MMXRegType reg, u64 i ) 
{
	RexR(0, reg);
	write16(0x6F0F);
	ModRM(0, reg, DISP32);
	write32(2);
	JMP8( 8 );
	write64( i );
}

void MOVQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	RexRB(0, to, from);
	write16( 0x6F0F );
	ModRM( 3, to, from );
}

void MOVQRmtoROffset( x86MMXRegType to, x86IntRegType from, u32 offset )
{
	RexRB(0, to, from);
	write16( 0x6F0F );

	if( offset < 128 ) {
		ModRM( 1, to, from );
		write8(offset);
	}
	else {
		ModRM( 2, to, from );
		write32(offset);
	}
}

void MOVQRtoRmOffset( x86IntRegType to, x86MMXRegType from, u32 offset )
{
	RexRB(0, from, to);
	write16( 0x7F0F );

	if( offset < 128 ) {
		ModRM( 1, from , to );
		write8(offset);
	}
	else {
		ModRM( 2, from, to );
		write32(offset);
	}
}

/* movd m32 to r64 */
void MOVDMtoMMX( x86MMXRegType to, uptr from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x6E), true, to, from, 0);
}

/* movd r64 to m32 */
void MOVDMMXtoM( uptr to, x86MMXRegType from ) 
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x7E), true, from, to, 0);
}

void MOVD32RtoMMX( x86MMXRegType to, x86IntRegType from )
{
	RexRB(0, to, from);
	write16( 0x6E0F );
	ModRM( 3, to, from );
}

void MOVD32RmtoMMX( x86MMXRegType to, x86IntRegType from )
{
	RexRB(0, to, from);
	write16( 0x6E0F );
	ModRM( 0, to, from );
}

void MOVD32RmOffsettoMMX( x86MMXRegType to, x86IntRegType from, u32 offset )
{
	RexRB(0, to, from);
	write16( 0x6E0F );

	if( offset < 128 ) {
		ModRM( 1, to, from );
		write8(offset);
	}
	else {
		ModRM( 2, to, from );
		write32(offset);
	}
}

void MOVD32MMXtoR( x86IntRegType to, x86MMXRegType from )
{
	RexRB(0, from, to);
	write16( 0x7E0F );
	ModRM( 3, from, to );
}

void MOVD32MMXtoRm( x86IntRegType to, x86MMXRegType from )
{
	RexRB(0, from, to);
	write16( 0x7E0F );
	ModRM( 0, from, to );
	if( to >= 4 ) {
		// no idea why
		assert( to == ESP );
		write8(0x24);
	}

}

void MOVD32MMXtoRmOffset( x86IntRegType to, x86MMXRegType from, u32 offset )
{
	RexRB(0, from, to);
	write16( 0x7E0F );

	if( offset < 128 ) {
		ModRM( 1, from, to );
		write8(offset);
	}
	else {
		ModRM( 2, from, to );
		write32(offset);
	}
}

///* movd r32 to r64 */
//void MOVD32MMXtoMMX( x86MMXRegType to, x86MMXRegType from ) 
//{
//	write16( 0x6E0F );
//	ModRM( 3, to, from );
//}
//
///* movq r64 to r32 */
//void MOVD64MMXtoMMX( x86MMXRegType to, x86MMXRegType from ) 
//{
//	write16( 0x7E0F );
//	ModRM( 3, from, to );
//}

// untested
void PACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	RexRB(0, to, from);
	write16( 0x630F );
	ModRM( 3, to, from ); 
}

void PACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	RexRB(0, to, from);
	write16( 0x6B0F );
	ModRM( 3, to, from ); 
}

void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from)
{
	RexRB(0, to, from);
	write16( 0xD70F ); 
	ModRM( 3, to, from );
}

void PINSRWRtoMMX( x86MMXRegType to, x86SSERegType from, u8 imm8 )
{
	RexRB(0, to, from);
	write16( 0xc40f );
	ModRM( 3, to, from );
	write8( imm8 );
}

void PSHUFWRtoR(x86MMXRegType to, x86MMXRegType from, u8 imm8)
{
	RexRB(0, to, from);
	write16(0x700f);
	ModRM( 3, to, from );
	write8(imm8);
}

void PSHUFWMtoR(x86MMXRegType to, uptr from, u8 imm8)
{
	MEMADDR_OP(0, VAROP2(0x0F, 0x70), true, to, from, 1 /* XXX was 0? */);
	write8(imm8);
}

void MASKMOVQRtoR(x86MMXRegType to, x86MMXRegType from)
{
	RexRB(0, to, from);
	write16(0xf70f);
	ModRM( 3, to, from );
}

#endif

#endif
