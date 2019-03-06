#define COMBINE_EXT          0x8570
#define COMBINE_RGB_EXT      0x8571
#define COMBINE_ALPHA_EXT    0x8572
#define SOURCE0_RGB_EXT      0x8580
#define SOURCE1_RGB_EXT      0x8581
#define SOURCE2_RGB_EXT      0x8582
#define SOURCE0_ALPHA_EXT    0x8588
#define SOURCE1_ALPHA_EXT    0x8589
#define SOURCE2_ALPHA_EXT    0x858A
#define OPERAND0_RGB_EXT     0x8590
#define OPERAND1_RGB_EXT     0x8591
#define OPERAND2_RGB_EXT     0x8592
#define OPERAND0_ALPHA_EXT   0x8598
#define OPERAND1_ALPHA_EXT   0x8599
#define OPERAND2_ALPHA_EXT   0x859A
#define RGB_SCALE_EXT        0x8573
#define ADD_SIGNED_EXT       0x8574
#define INTERPOLATE_EXT      0x8575
#define CONSTANT_EXT         0x8576
#define PRIMARY_COLOR_EXT    0x8577
#define PREVIOUS_EXT         0x8578

#define FUNC_ADD_EXT             0x8006
#define FUNC_REVERSESUBTRACT_EXT 0x800B

#ifdef _WINDOWS
typedef void (APIENTRY * PFNGLBLENDEQU) (GLenum mode);
typedef void (APIENTRY * PFNGLCOLORTABLEEXT)
    (GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
     GLenum type, const GLvoid *data);
typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int iV);
#else
typedef void (* PFNGLBLENDEQU) (GLenum mode);
typedef void (* PFNGLCOLORTABLEEXT)
    (GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
     GLenum type, const GLvoid *data);
#endif

#define GL_UNSIGNED_SHORT_4_4_4_4_EXT       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT       0x8034

#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE

#ifndef GL_BGR_EXT
#define GL_BGR_EXT                        0x80E0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT                       0x80E1
#endif

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT               0x80E5
#endif

//GL_ALPHA_SCALE
