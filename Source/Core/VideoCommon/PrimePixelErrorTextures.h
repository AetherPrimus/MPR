#include <array>
#include <string_view>

// This contains the texture names for each culprit causing the infamous pixel glitch.
// These textures will be made transparent when detected in the TextureCacheBase.

constexpr uint64_t PRIME1_PIXEL_HASH = 0x79d1ddfd254bc6d1L; // tex1_128x32_b2f411682a7de4a5_14
constexpr uint64_t PRIME2_PIXEL_HASH = 0x037fa2c1eb31f00bL; // tex1_32x32_m_e0267b6c99e16dd4_14
// Prime 3 uses the same texture as Prime 2
