#ifndef CONSTHASH_H_
#define CONSTHASH_H_

#include <common.h>
#include <string.h>

#define FNV_OFFSET (u32)2166136261u
#define FNV_PRIME (u32)16777619u
#define FNV1A_HASH_STEP(h, c) (((u8)(c) ^ (u32)(h)) * FNV_PRIME)

#define HASH_STR_1(s) FNV1A_HASH_STEP(FNV_OFFSET, s[0])
#define HASH_STR_2(s) FNV1A_HASH_STEP(HASH_STR_1(s), s[1])
#define HASH_STR_3(s) FNV1A_HASH_STEP(HASH_STR_2(s), s[2])
#define HASH_STR_4(s) FNV1A_HASH_STEP(HASH_STR_3(s), s[3])
#define HASH_STR_5(s) FNV1A_HASH_STEP(HASH_STR_4(s), s[4])
#define HASH_STR_6(s) FNV1A_HASH_STEP(HASH_STR_5(s), s[5])
#define HASH_STR_7(s) FNV1A_HASH_STEP(HASH_STR_6(s), s[6])
#define HASH_STR_8(s) FNV1A_HASH_STEP(HASH_STR_7(s), s[7])
#define HASH_STR_9(s) FNV1A_HASH_STEP(HASH_STR_8(s), s[8])
#define HASH_STR_10(s) FNV1A_HASH_STEP(HASH_STR_9(s), s[9])
#define HASH_STR_11(s) FNV1A_HASH_STEP(HASH_STR_10(s), s[10])
#define HASH_STR_12(s) FNV1A_HASH_STEP(HASH_STR_11(s), s[11])
#define HASH_STR_13(s) FNV1A_HASH_STEP(HASH_STR_12(s), s[12])
#define HASH_STR_14(s) FNV1A_HASH_STEP(HASH_STR_13(s), s[13])
#define HASH_STR_15(s) FNV1A_HASH_STEP(HASH_STR_14(s), s[14])
#define HASH_STR_16(s) FNV1A_HASH_STEP(HASH_STR_15(s), s[15])
#define HASH_STR_17(s) FNV1A_HASH_STEP(HASH_STR_16(s), s[16])
#define HASH_STR_18(s) FNV1A_HASH_STEP(HASH_STR_17(s), s[17])
#define HASH_STR_19(s) FNV1A_HASH_STEP(HASH_STR_18(s), s[18])
#define HASH_STR_20(s) FNV1A_HASH_STEP(HASH_STR_19(s), s[19])
#define HASH_STR_21(s) FNV1A_HASH_STEP(HASH_STR_20(s), s[20])
#define HASH_STR_22(s) FNV1A_HASH_STEP(HASH_STR_21(s), s[21])
#define HASH_STR_23(s) FNV1A_HASH_STEP(HASH_STR_22(s), s[22])
#define HASH_STR_24(s) FNV1A_HASH_STEP(HASH_STR_23(s), s[23])
#define HASH_STR_25(s) FNV1A_HASH_STEP(HASH_STR_24(s), s[24])
#define HASH_STR_26(s) FNV1A_HASH_STEP(HASH_STR_25(s), s[25])
#define HASH_STR_27(s) FNV1A_HASH_STEP(HASH_STR_26(s), s[26])
#define HASH_STR_28(s) FNV1A_HASH_STEP(HASH_STR_27(s), s[27])
#define HASH_STR_29(s) FNV1A_HASH_STEP(HASH_STR_28(s), s[28])
#define HASH_STR_30(s) FNV1A_HASH_STEP(HASH_STR_29(s), s[29])
#define HASH_STR_31(s) FNV1A_HASH_STEP(HASH_STR_30(s), s[30])
#define HASH_STR_32(s) FNV1A_HASH_STEP(HASH_STR_31(s), s[31])

/* hash with FNV1A32_CSTR_LEN("my string", 9) where 9 is strlen("my string") */
#define FNV1A32_CSTR_LEN(s, len) HASH_STR_##len(s)

u32 fnv32_hash(const char *_str) {
  const usize len = strnlen_s(_str, 128);
  u32 hash = FNV_OFFSET;
  for (usize i = 0; i != len; i++) {
    hash = ((u8)_str[i] ^ hash) * FNV_PRIME;
  }
  return hash;
}

#endif // CONSTHASH_H_
