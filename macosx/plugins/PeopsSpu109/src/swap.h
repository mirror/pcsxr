
// byteswappings

#define SWAP16(x) (((x)>>8 & 0xff) | ((x)<<8 & 0xff00))
#define SWAP32(x) (((x)>>24 & 0xfful) | ((x)>>8 & 0xff00ul) | ((x)<<8 & 0xff0000ul) | ((x)<<24 & 0xff000000ul))

#ifdef __POWERPC__

#define HOST2LE16(x) SWAP16(x)
#define HOST2BE16(x) (x)
#define LE2HOST16(x) SWAP16(x)
#define BE2HOST16(x) (x)

#else

#define HOST2LE16(x) (x)
#define HOST2BE16(x) SWAP16(x)
#define LE2HOST16(x) (x)
#define BE2HOST16(x) SWAP16(x)

#endif
