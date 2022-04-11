#pragma once

#include "Common/File.h"

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                                                             \
  ((u32)(u8)(ch0) | ((u32)(u8)(ch1) << 8) | ((u32)(u8)(ch2) << 16) | ((u32)(u8)(ch3) << 24))
/*
 * FOURCC codes for DX compressed-texture pixel formats
 */
#define DDS_SIGNARURE (MAKEFOURCC('D', 'D', 'S', ' '))
#define ADDS_SIGNARURE (MAKEFOURCC('A', 'D', 'D', 'S'))
#define FOURCC_DXT1 (MAKEFOURCC('D', 'X', 'T', '1'))
#define FOURCC_DXT3 (MAKEFOURCC('D', 'X', 'T', '3'))
#define FOURCC_DXT5 (MAKEFOURCC('D', 'X', 'T', '5'))
 /*
  * Flags
  */
#define DDSD_CAPS 0x00000001
#define DDSD_HEIGHT 0x00000002
#define DDSD_WIDTH 0x00000004
#define DDSD_PITCH 0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE 0x00080000
#define DDSD_DEPTH 0x00800000

  /*
   * Pixel Fomat flags
   */
#define DDPF_FOURCC 0x00000004      // DDPF_FOURCC
#define DDPF_RGB 0x00000040         // DDPF_RGB
#define DDPF_RGBA 0x00000041        // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDPF_LUMINANCE 0x00020000   // DDPF_LUMINANCE
#define DDPF_LUMINANCEA 0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDPF_ALPHA 0x00000002       // DDPF_ALPHA
#define DDPF_PAL8 0x00000020        // DDPF_PALETTEINDEXED8
#define DDPF_PAL8A 0x00000021       // DDPF_PALETTEINDEXED8 | DDPF_ALPHAPIXELS
#define DDPF_BUMPDUDV 0x00080000    // DDPF_BUMPDUDV

   // Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
enum DDS_RESOURCE_DIMENSION
{
  DDS_DIMENSION_TEXTURE1D = 2,
  DDS_DIMENSION_TEXTURE2D = 3,
  DDS_DIMENSION_TEXTURE3D = 4,
};
#pragma pack(push, 1)
typedef struct _DDPIXELFORMAT
{
  u32 dwSize;    // size of structure
  u32 dwFlags;   // pixel format flags
  u32 dwFourCC;  // (FOURCC code)
  union
  {
    u32 dwRGBBitCount;        // how many bits per pixel
    u32 dwYUVBitCount;        // how many bits per pixel
    u32 dwZBufferBitDepth;    // how many total bits/pixel in z buffer (including any stencil bits)
    u32 dwAlphaBitDepth;      // how many bits for alpha channels
    u32 dwLuminanceBitCount;  // how many bits per pixel
    u32 dwBumpBitCount;       // how many bits per "buxel", total
    u32 dwPrivateFormatBitCount;  // Bits per pixel of private driver formats. Only valid in texture
    // format list and if DDPF_D3DFORMAT is set
  };
  union
  {
    u32 dwRBitMask;          // mask for red bit
    u32 dwYBitMask;          // mask for Y bits
    u32 dwStencilBitDepth;   // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is
                             // total Z-only bits)
    u32 dwLuminanceBitMask;  // mask for luminance bits
    u32 dwBumpDuBitMask;     // mask for bump map U delta bits
    u32 dwOperations;        // DDPF_D3DFORMAT Operations
  };
  union
  {
    u32 dwGBitMask;       // mask for green bits
    u32 dwUBitMask;       // mask for U bits
    u32 dwZBitMask;       // mask for Z bits
    u32 dwBumpDvBitMask;  // mask for bump map V delta bits
    struct
    {
      u16 wFlipMSTypes;  // Multisample methods supported via flip for this D3DFORMAT
      u16 wBltMSTypes;   // Multisample methods supported via blt for this D3DFORMAT
    } MultiSampleCaps;
  };
  union
  {
    u32 dwBBitMask;              // mask for blue bits
    u32 dwVBitMask;              // mask for V bits
    u32 dwStencilBitMask;        // mask for stencil bits
    u32 dwBumpLuminanceBitMask;  // mask for luminance in bump map
  };
  union
  {
    u32 dwRGBAlphaBitMask;        // mask for alpha channel
    u32 dwYUVAlphaBitMask;        // mask for alpha channel
    u32 dwLuminanceAlphaBitMask;  // mask for alpha channel
    u32 dwRGBZBitMask;            // mask for Z channel
    u32 dwYUVZBitMask;            // mask for Z channel
  };
} DDPIXELFORMAT;

typedef struct _DDCOLORKEY
{
  u32 dwColorSpaceLowValue;  // low boundary of color space that is to
  // be treated as Color Key, inclusive
  u32 dwColorSpaceHighValue;  // high boundary of color space that is
  // to be treated as Color Key, inclusive
} DDCOLORKEY;

typedef struct _DDSCAPS2
{
  u32 dwCaps;  // capabilities of surface wanted
  u32 dwCaps2;
  u32 dwCaps3;
  union
  {
    u32 dwCaps4;
    u32 dwVolumeDepth;
  };
} DDSCAPS2;

typedef struct _DDSHeader
{
  u32 dwSignature;
  u32 dwSize;
  u32 dwFlags;
  u32 dwHeight;
  u32 dwWidth;
  union
  {
    s32 lPitch;
    u32 dwLinearSize;
  };
  union
  {
    u32 dwBackBufferCount;
    u32 dwDepth;
  };
  union
  {
    u32 dwMipMapCount;
    u32 dwRefreshRate;
    u32 dwSrcVBHandle;
  };
  u32 dwAlphaBitDepth;
  u32 dwReserved;
  u32 lpSurface;
  union
  {
    DDCOLORKEY ddckCKDestOverlay;
    u32 dwEmptyFaceColor;
  };
  DDCOLORKEY ddckCKDestBlt;
  DDCOLORKEY ddckCKSrcOverlay;
  DDCOLORKEY ddckCKSrcBlt;
  union
  {
    DDPIXELFORMAT ddpfPixelFormat;
    u32 dwFVF;
  };
  DDSCAPS2 ddsCaps;
  u32 dwTextureStage;
} DDSHeader;

typedef struct _ADDSHeader
{
  u32 dwFlags;
  u32 dwHeight;
  u32 dwWidth;
  u32 dwSignature;
  u32 dwSize;
  union
  {
    s32 lPitch;
    u32 dwLinearSize;
  };
  union
  {
    u32 dwBackBufferCount;
    u32 dwDepth;
  };
  union
  {
    u32 dwMipMapCount;
    u32 dwRefreshRate;
    u32 dwSrcVBHandle;
  };
  u32 dwAlphaBitDepth;
  u32 dwReserved;
  u32 lpSurface;
  union
  {
    DDCOLORKEY ddckCKDestOverlay;
    u32 dwEmptyFaceColor;
  };
  DDCOLORKEY ddckCKDestBlt;
  DDCOLORKEY ddckCKSrcOverlay;
  DDCOLORKEY ddckCKSrcBlt;
  union
  {
    DDPIXELFORMAT ddpfPixelFormat;
    u32 dwFVF;
  };
  u32 dwTextureStage;
  DDSCAPS2 ddsCaps;
} ADDSHeader;

struct _DDSHeader_DXT10
{
  uint32_t dxgiFormat;
  uint32_t resourceDimension;
  uint32_t miscFlag;  // see DDS_RESOURCE_MISC_FLAG
  uint32_t arraySize;
  uint32_t miscFlags2;  // see DDS_MISC_FLAGS2
};

#pragma pack(pop)
