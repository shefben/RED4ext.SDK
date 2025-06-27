#ifndef ZSTD_H
#define ZSTD_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
    size_t ZSTD_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize, int level);
    size_t ZSTD_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize);
    unsigned ZSTD_isError(size_t code);
    size_t ZSTD_compressBound(size_t srcSize);
#ifdef __cplusplus
}
#endif
#endif
