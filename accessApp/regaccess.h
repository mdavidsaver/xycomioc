
#ifndef RACCESS_H_INC
#define RACCESS_H_INC

/*
 * Register Access Macros
 *
 * This assumes that registers are defined with names like
 * U16_name.  This serves to check that registers are always
 * access with the correct width.  Registers which actually
 * need to be access with different widths would need multiple
 * definitions with different prefixes.
 *
 * When defined as macros this would look like
 *
 * #define U16_DEVCSR 0x4
 * #define  U8_REGB   0x6
 * #define U32_DATA_BASE 0x100
 * #define U32_DATA(N) (U32_DATA_BASE + 4*(N))
 *
 * Note: The units for these macros depend on the type of the
 *       base pointer.  By convention this is a pointer to
 *       volatile unsigned char* so that all addresses are
 *       in bytes.
 */

/*
 * Accessors which do no endian conversion.
 *
 * Use these with buses which do this conversion implicitly (VME)
 */

#define WRITE32(base, addr, val) ( *(volatile epicsUInt32*)((base)+ (U32_ ## addr)) = (val)
#define WRITE16(base, addr, val) ( *(volatile epicsUInt16*)((base)+ (U16_ ## addr)) = (val) )
#define WRITE8(base, addr, val) ( *(volatile epicsUInt8*)((base)+ (U8_ ## addr)) = (val) )

#define READ32(base, addr) ( *(volatile epicsUInt32*)((base)+ (U32_ ## addr)) )
#define READ16(base, addr) ( *(volatile epicsUInt16*)((base)+ (U16_ ## addr)) )
#define READ8(base, addr) ( *(volatile epicsUInt8*)((base)+ (U8_ ## addr)) )


#endif /* RACCESS_H_INC */
