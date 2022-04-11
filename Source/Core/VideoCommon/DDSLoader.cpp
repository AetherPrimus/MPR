// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.
// Added for Ishiiruka By Tino

// Copyright 2014 Rodolfo Bogado
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the owner nor the names of its contributors may
//       be used to endorse or promote products derived from this software
//       without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "VideoCommon/Util/DDSLoader.h"

#include "Common/Common.h"
#include "VideoCommon/ImageLoader.h"
#include "VideoCommon/ImageWrite.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/Util/Aether.h"


static_assert(sizeof(_DDSHeader) == 128, "DDS Header size mismatch");
static_assert(sizeof(_DDSHeader_DXT10) == 20, "DDS DX10 Extended Header size mismatch");
#pragma optimize("", off)
bool ImageLoader::ReadDDS(ImageLoaderParams& loader_params)
{
  DDSHeader ddsd;
  size_t header_size = 0;
  bool addsd = false;

  File::IFile* file;
  if (StringEndsWith(loader_params.Path, ".adds")) {
    file = new Aether::AFile();
    addsd = true;
  }
  else {
    file = new File::IOFile();
  }

  if (!file->Open(loader_params.Path, "rb")) {
    return false;
  }

  u32 block_size = 8;

  if (!addsd) {
    // Get the surface descriptor
    if (!file->ReadBytes(&ddsd, sizeof(ddsd)) || ddsd.dwSignature != DDS_SIGNARURE ||
      ddsd.dwSize != 124)
    {
      return false;
    }
  } else {
    static_cast<Aether::AFile*>(file)->RetrieveHeader(&ddsd);
    if (ddsd.dwSignature != ADDS_SIGNARURE || ddsd.dwSize != 124)
    {
      return false;
    }
  }
  
  header_size += sizeof(ddsd);
  // Check for a valid Header
  u32 flag = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
  if ((ddsd.dwFlags & flag) != flag)
  {
    return false;
  }

  // Image should be 2D.
  if (ddsd.dwFlags & DDSD_DEPTH)
    return false;

  flag = DDPF_FOURCC | DDPF_RGB;
  if ((ddsd.ddpfPixelFormat.dwFlags & flag) == 0)
  {
    return false;
  }
  if (ddsd.ddpfPixelFormat.dwSize != 32)
  {
    return false;
  }
  // Handle DX10 extension header.
  u32 dxt10_format = 0;
  if (ddsd.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', '1', '0'))
  {
    _DDSHeader_DXT10 dxt10_header;
    if (!file->ReadBytes(&dxt10_header, sizeof(dxt10_header)))
      return false;
    // Can't handle array textures here. Doesn't make sense to use them, anyway.
    if (dxt10_header.resourceDimension != DDS_DIMENSION_TEXTURE2D || dxt10_header.arraySize != 1)
      return false;

    header_size += sizeof(dxt10_header);
    dxt10_format = dxt10_header.dxgiFormat;
  }

  //
  // Suport only Basic DDS compresion Formats
  //
  u32 FourCC = ddsd.ddpfPixelFormat.dwFourCC;
  if ((FourCC == FOURCC_DXT1 || dxt10_format == 71) &&
      g_ActiveConfig.backend_info.bSupportedFormats[HostTextureFormat::PC_TEX_FMT_DXT1])
  {
    block_size = 8;
  }
  else if ((FourCC == FOURCC_DXT3 || dxt10_format == 74) &&
           g_ActiveConfig.backend_info.bSupportedFormats[HostTextureFormat::PC_TEX_FMT_DXT3])
  {
    block_size = 16;
  }
  else if ((FourCC == FOURCC_DXT5 || dxt10_format == 77) &&
           g_ActiveConfig.backend_info.bSupportedFormats[HostTextureFormat::PC_TEX_FMT_DXT5])
  {
    block_size = 16;
  }
  else if (dxt10_format == 98 &&
           g_ActiveConfig.backend_info.bSupportedFormats[HostTextureFormat::PC_TEX_FMT_BPTC])
  {
    block_size = 16;
  }
  else
  {
    // unsupported format
    return false;
  }

  //
  // How big will the buffer need to be to load all of the pixel data
  // including mip-maps?
  ddsd.dwLinearSize = ((ddsd.dwWidth + 3) >> 2) * ((ddsd.dwHeight + 3) >> 2) * block_size;
  bool mipmapspresent = ddsd.dwMipMapCount > 1 && ddsd.dwFlags & DDSD_MIPMAPCOUNT;
  loader_params.data_size = ddsd.dwLinearSize;
  size_t remaining = file->GetSize() - header_size;
  if (mipmapspresent)
  {
    // calculate mipmaps size
    u32 w = ddsd.dwWidth;
    u32 h = ddsd.dwHeight;
    u32 level = 1;
    while ((w > 1 || h > 1))
    {
      w = std::max(w >> 1, 1u);
      h = std::max(h >> 1, 1u);
      u32 required = loader_params.data_size + ((w + 3) >> 2) * ((h + 3) >> 2) * block_size;
      if (required <= remaining)
      {
        loader_params.data_size = required;
        level++;
      }
      else
      {
        break;
      }
    }
    ddsd.dwMipMapCount = level;
  }
  else
  {
    ddsd.dwMipMapCount = 0;
  }
  if (remaining < loader_params.data_size)
  {
    loader_params.data_size = ddsd.dwLinearSize;
    ddsd.dwMipMapCount = 0;
    mipmapspresent = false;
    if (remaining < loader_params.data_size)
    {
      // Invalid File size
      return false;
    }
  }

  // Check available buffer size
  loader_params.dst =
      loader_params.request_buffer_delegate(loader_params.data_size, mipmapspresent);
  if (loader_params.dst == nullptr)
  {
    return false;
  }

  if (!file->ReadBytes(loader_params.dst, loader_params.data_size))
  {
    // Unable to read file
    return false;
  }

  loader_params.Width = ddsd.dwWidth;
  loader_params.Height = ddsd.dwHeight;
  if (FourCC == FOURCC_DXT1 || dxt10_format == 71)
  {
    loader_params.resultTex = HostTextureFormat::PC_TEX_FMT_DXT1;
    loader_params.Width = ((ddsd.dwWidth + 3) >> 2) << 2;
    loader_params.Height = ((ddsd.dwHeight + 3) >> 2) << 2;
  }
  else if (FourCC == FOURCC_DXT3 || dxt10_format == 74)
  {
    loader_params.resultTex = HostTextureFormat::PC_TEX_FMT_DXT3;
  }
  else if (FourCC == FOURCC_DXT5 || dxt10_format == 77)
  {
    loader_params.resultTex = HostTextureFormat::PC_TEX_FMT_DXT5;
  }
  else
  {
    loader_params.resultTex = HostTextureFormat::PC_TEX_FMT_BPTC;
  }  
  // loader_params should get the number of mipmaps, not counting the first level
  loader_params.nummipmaps = (ddsd.dwMipMapCount != 0) ? (ddsd.dwMipMapCount - 1) : 0;
  return true;
}
#pragma optimize("", on)

bool TextureToDDS(const u8* data, int row_stride, const std::string& filename, int width,
                  int height, DDSCompression format)
{
  DDSHeader header = {0};
  header.dwSignature = DDS_SIGNARURE;
  header.dwSize = 124;
  header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
  header.ddpfPixelFormat.dwFlags = DDPF_FOURCC | DDPF_RGB;
  header.ddpfPixelFormat.dwSize = 32;
  header.dwWidth = width;
  header.dwHeight = height;
  switch (format)
  {
  case DDSC_DXT1:
    header.ddpfPixelFormat.dwFourCC = FOURCC_DXT1;
    break;
  case DDSC_DXT3:
    header.ddpfPixelFormat.dwFourCC = FOURCC_DXT3;
    break;
  case DDSC_DXT5:
    header.ddpfPixelFormat.dwFourCC = FOURCC_DXT5;
    break;
  default:
    break;
  }
  header.dwLinearSize = ((header.dwWidth + 3) >> 2) * ((header.dwHeight + 3) >> 2) * 16;
  header.dwMipMapCount = 1;
  File::IOFile fp(filename, "wb");
  if (!fp.IsOpen())
  {
    PanicAlertT("Screenshot failed: Could not open file %s %d", filename.c_str(), errno);
    return false;
  }
  fp.WriteBytes(&header, sizeof(DDSHeader));
  u32 ddstride = ((header.dwWidth + 3) >> 2) * 16;
  u32 lines = ((header.dwHeight + 3) >> 2);
  for (size_t i = 0; i < lines; i++)
  {
    fp.WriteBytes(data, ddstride);
    data += row_stride;
  }
  return true;
}
