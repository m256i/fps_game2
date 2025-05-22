#include <renderer/internal/stb_image.h>
#include <renderer/internal/stb_image_resize.h>
#include <renderer/internal/stb_dxt.h>
#include <renderer/internal/load_image_texture.h>
#include <util/dbg/alloctrack.h>
#include <common.h>

/*
call this on RGB textures with no alpha
*/
u0 compress_rgba_dxt1(
  u8 *const __restrict _dst,
  const u8 *const __restrict _src,
  usize width,
  usize height
) {
  const static usize block_size = 8; /* 8 bytes per block for DXT1 */
  const usize        blocks_x   = (width + 3) / 4;
  const usize        blocks_y   = (height + 3) / 4;

  for (usize y = 0; y < blocks_y; ++y) {
    for (usize x = 0; x < blocks_x; ++x) {
      u8 *dest_block = _dst + (y * blocks_x + x) * block_size;
      u8  block[16 * 4]; /* 16 pixels * 4 bytes per pixel (RGBA) */

      for (usize by = 0; by < 4; ++by) {
        for (usize bx = 0; bx < 4; ++bx) {
          const usize src_x = x * 4 + bx;
          const usize src_y = y * 4 + by;

          if (src_x < width && src_y < height) {
            usize src_index   = (src_y * width + src_x) * 4;
            usize block_index = (by * 4 + bx) * 4;

            block[block_index + 0] = _src[src_index + 0];
            block[block_index + 1] = _src[src_index + 1];
            block[block_index + 2] = _src[src_index + 2];
            block[block_index + 3] = _src[src_index + 3];

          } else {
            i32 block_index        = (by * 4 + bx) * 4;
            block[block_index + 0] = 0;
            block[block_index + 1] = 0;
            block[block_index + 2] = 0;
            block[block_index + 3] = 0;
          }
        }
      }

      stb_compress_dxt_block(dest_block, block, 0, STB_DXT_NORMAL);
    }
  }
}
/*
call this on RGBA textures WITH alpha
*/
u0 compress_rgba_dxt5(
  u8 *const __restrict _dst,
  const u8 *const __restrict _src,
  usize width,
  usize height
) {
  const static usize block_size = 16;
  const usize        blocks_x   = (width + 3) / 4;
  const usize        blocks_y   = (height + 3) / 4;

  for (usize y = 0; y < blocks_y; ++y) {
    for (usize x = 0; x < blocks_x; ++x) {
      u8 *dest_block = _dst + (y * blocks_x + x) * block_size;
      u8  block[16 * 4];
      for (usize by = 0; by < 4; ++by) {
        for (usize bx = 0; bx < 4; ++bx) {
          const usize src_x = x * 4 + bx;
          const usize src_y = y * 4 + by;

          if (src_x < width && src_y < height) {
            usize src_index   = (src_y * width + src_x) * 4;
            usize block_index = (by * 4 + bx) * 4;

            block[block_index + 0] = _src[src_index + 0];
            block[block_index + 1] = _src[src_index + 1];
            block[block_index + 2] = _src[src_index + 2];
            block[block_index + 3] = _src[src_index + 3];

          } else {
            i32 block_index        = (by * 4 + bx) * 4;
            block[block_index + 0] = 0;
            block[block_index + 1] = 0;
            block[block_index + 2] = 0;
            block[block_index + 3] = 0;
          }
        }
      }

      stb_compress_dxt_block(dest_block, block, 1, STB_DXT_NORMAL);
      // stb_compress_dxt_block(dest_block, block, 1, 2);
      // stb_compress_dxt_block(dest_block + 8, block, 0, STB_DXT_NORMAL);
    }
  }
}

loaded_texture load_texture_from_file(
  char  *_path,
  bool   _compress,
  f32    _scale,
  GLenum _wrap_mode
) {
  i32 size_x, size_y, channel_count;
  u8 *data = stbi_load(_path, &size_x, &size_y, &channel_count, STBI_rgb_alpha);

  if (!data) {
    GAME_WARNF("failed to load texture from file '%s'", _path);
    return (loaded_texture){0};
  }

  const u32 new_size_x = (u32)((f32)size_x * _scale);
  const u32 new_size_y = (u32)((f32)size_y * _scale);

  loaded_texture out_texture_data;

  u8 *scaled_image_data =
    TRACKED_MALLOC(new_size_x * new_size_y * channel_count);

  if (!stbir_resize_uint8(
        data,
        size_x,
        size_y,
        0,
        scaled_image_data,
        new_size_x,
        new_size_y,
        0,
        channel_count
      )) {
    GAME_WARNF("error resizing image: '%s'", _path);
    exit(1);
  }

  out_texture_data.width  = new_size_x;
  out_texture_data.height = new_size_y;

  /* free here alrady as we have scaled_image_data now! */
  stbi_image_free(data);

  GLenum format = 0, internal_format;
  switch (channel_count) {
  case 1:
    format          = GL_RED;
    internal_format = GL_R8;
    break;
  case 3:
    format          = GL_RGB;
    internal_format = GL_RGB8;
    break;
  case 4:
    format          = GL_RGBA;
    internal_format = GL_RGBA8;
    break;
  default: {
    GAME_CRITICALF("unsupported file format on file '%s'", _path);
    exit(1);
    break;
  }
  }

  GLuint handle = 0;
  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_2D, handle);

  if (!_compress) goto no_compress;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  switch (format) {
  case GL_RGB: {
    const usize total_size = ((new_size_x + 3) / 4) * ((new_size_y + 3) / 4) *
                             8ull; /* dxt1 data size */
    u8 *compressed_data = TRACKED_MALLOC(total_size);
    compress_rgba_dxt1(
      compressed_data,
      scaled_image_data,
      new_size_x,
      new_size_y
    );
    glCompressedTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
      new_size_x,
      new_size_y,
      0,
      total_size,
      compressed_data
    );
    TRACKED_FREE(compressed_data);
    break;
  }
  case GL_RGBA: {
    const usize total_size = ((new_size_x + 3) / 4) * ((new_size_y + 3) / 4) *
                             16ull; /* dxt5 data size */
    u8 *compressed_data = TRACKED_MALLOC(total_size);
    compress_rgba_dxt5(
      compressed_data,
      scaled_image_data,
      new_size_x,
      new_size_y
    );
    glCompressedTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
      new_size_x,
      new_size_y,
      0,
      total_size,
      compressed_data
    );
    TRACKED_FREE(compressed_data);
    break;
  }
  no_compress:
  default: {
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      internal_format,
      new_size_x,
      new_size_y,
      0,
      format,
      GL_UNSIGNED_BYTE,
      scaled_image_data
    );
    break;
  }
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrap_mode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrap_mode);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  out_texture_data.internal_format = internal_format;
  out_texture_data.format          = format;
  out_texture_data.handle          = handle;

  /* data now on GPU, we can free */
  TRACKED_FREE(scaled_image_data);
  return out_texture_data;
}
