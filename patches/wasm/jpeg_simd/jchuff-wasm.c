/*
 * jchuff-wasm.c - baseline Huffman entropy encoding (WebAssembly SIMD128)
 *
 * Port of jsimd_huff_encode_one_block_neon() from
 * simd/arm/aarch64/jchuff-neon.c, producing a bit-identical output stream.
 * The SIMD part computes, for all 64 coefficients at once: the zig-zag
 * reorder (4-way swizzle emulating vqtbl4q_s8), the nonzero bitmap
 * (wasm_i8x16_bitmask), and nbits/diff for every coefficient. Only the
 * data-dependent bit-packing loop stays scalar, as on every other
 * architecture.
 *
 * Differences from the NEON implementation, chosen for the wasm instruction
 * set (none of them change the output):
 *  - The nonzero bitmap is built LSB-first (bit k = coefficient k) and the
 *    emission loop uses count-trailing-zeros instead of the NEON MSB-first
 *    bitmap with count-leading-zeros.
 *  - wasm has no per-lane leading-zero count, so nbits is computed as the
 *    per-byte popcount of the bit-smeared absolute value, which also yields
 *    the (1 << nbits) - 1 mask that NEON derives via variable shifts.
 *  - jchuff.c uses a 64-bit SIMD bit buffer on non-Arm architectures (wasm
 *    included), so the 64-bit flavors of PUT_BITS()/FLUSH() are used, as in
 *    the x86-64 implementation.
 */

#define JPEG_INTERNALS
#include "../../src/jinclude.h"
#include "../../src/jpeglib.h"
#include "../../src/jsimd.h"
#include "../../src/jdct.h"
#include "../../src/jsimddct.h"
#include "../jsimd.h"

#include <string.h>

#include <wasm_simd128.h>

/* These structs mirror savable_state/working_state in src/jchuff.c (with
 * WITH_SIMD defined). The put_buffer union member used by the SIMD encoder
 * is unsigned long long on wasm, so the union storage is a single u64.
 */
typedef struct {
  unsigned long long put_buffer;        /* current bit accumulation buffer */
  int free_bits;                        /* # of bits available in it */
  int last_dc_val[MAX_COMPS_IN_SCAN];   /* last DC coef for each component */
} savable_state;

typedef struct {
  JOCTET *next_output_byte;     /* => next byte to write in buffer */
  size_t free_in_buffer;        /* # of byte spaces remaining in buffer */
  savable_state cur;            /* Current bit buffer & DC state */
  j_compress_ptr cinfo;         /* dump_buffer needs access to this */
  int simd;
} working_state;

#define BIT_BUF_SIZE  64

/* Output byte b and, speculatively, an additional 0 byte. 0xFF must be
 * encoded as 0xFF 0x00, so the output buffer pointer is advanced by 2 if the
 * byte is 0xFF. Otherwise, the output buffer pointer is advanced by 1, and
 * the speculative 0 byte will be overwritten by the next byte.
 */
#define EMIT_BYTE(b) { \
  buffer[0] = (JOCTET)(b); \
  buffer[1] = 0; \
  buffer -= -2 + ((JOCTET)(b) < 0xFF); \
}

/* Output the entire bit buffer. If there are no 0xFF bytes in it, then write
 * directly to the output buffer. Otherwise, use the EMIT_BYTE() macro to
 * encode 0xFF as 0xFF 0x00. (memcpy compiles to a single unaligned 64-bit
 * store on wasm.)
 */
#define FLUSH() { \
  if (put_buffer & 0x8080808080808080ULL & \
      ~(put_buffer + 0x0101010101010101ULL)) { \
    EMIT_BYTE(put_buffer >> 56) \
    EMIT_BYTE(put_buffer >> 48) \
    EMIT_BYTE(put_buffer >> 40) \
    EMIT_BYTE(put_buffer >> 32) \
    EMIT_BYTE(put_buffer >> 24) \
    EMIT_BYTE(put_buffer >> 16) \
    EMIT_BYTE(put_buffer >>  8) \
    EMIT_BYTE(put_buffer      ) \
  } else { \
    unsigned long long swapped = __builtin_bswap64(put_buffer); \
    memcpy(buffer, &swapped, 8); \
    buffer += 8; \
  } \
}

/* Fill the bit buffer to capacity with the leading bits from code, then
 * output the bit buffer and put the remaining bits from code into the bit
 * buffer.
 */
#define PUT_AND_FLUSH(code, size) { \
  put_buffer = (put_buffer << (size + free_bits)) | (code >> -free_bits); \
  FLUSH() \
  free_bits += BIT_BUF_SIZE; \
  put_buffer = code; \
}

/* Insert code into the bit buffer and output the bit buffer if needed.
 * NOTE: We can't flush with free_bits == 0, since the left shift in
 * PUT_AND_FLUSH() would have undefined behavior.
 */
#define PUT_BITS(code, size) { \
  free_bits -= size; \
  if (free_bits < 0) \
    PUT_AND_FLUSH(code, size) \
  else \
    put_buffer = (put_buffer << size) | code; \
}

#define PUT_CODE(code, size, diff) { \
  diff |= code << nbits; \
  nbits += size; \
  PUT_BITS(diff, nbits) \
}

/* Byte indices into the 64-byte coefficient table for the zig-zag reorder,
 * identical to jsimd_huff_encode_one_block_consts in the NEON version. Rows
 * 0, 1, 3 index rows 0-3 of the block; rows 4, 6 index block rows 4-7; rows
 * 2, 5 index block rows 2-5; row 7 indexes block rows 5-7. 255 marks lanes
 * filled in separately.
 */
static const uint8_t jsimd_huff_encode_one_block_consts[] = {
    0,   1,   2,   3,  16,  17,  32,  33,
   18,  19,   4,   5,   6,   7,  20,  21,
   34,  35,  48,  49, 255, 255,  50,  51,
   36,  37,  22,  23,   8,   9,  10,  11,
  255, 255,   6,   7,  20,  21,  34,  35,
   48,  49, 255, 255,  50,  51,  36,  37,
   54,  55,  40,  41,  26,  27,  12,  13,
   14,  15,  28,  29,  42,  43,  56,  57,
    6,   7,  20,  21,  34,  35,  48,  49,
   50,  51,  36,  37,  22,  23,   8,   9,
   26,  27,  12,  13, 255, 255,  14,  15,
   28,  29,  42,  43,  56,  57, 255, 255,
   52,  53,  54,  55,  40,  41,  26,  27,
   12,  13, 255, 255,  14,  15,  28,  29,
   26,  27,  40,  41,  42,  43,  28,  29,
   14,  15,  30,  31,  44,  45,  46,  47
};

/* vqtbl4q_s8: look up bytes in a 64-byte table held in four vectors.
 * wasm_i8x16_swizzle returns 0 for out-of-range indices, so out-of-table
 * lanes (index 255) come out as 0, matching TBL. */
static inline v128_t tbl4(v128_t t0, v128_t t1, v128_t t2, v128_t t3,
                          v128_t idx)
{
  v128_t r = wasm_i8x16_swizzle(t0, idx);
  r = wasm_v128_or(r, wasm_i8x16_swizzle(t1, wasm_i8x16_sub(idx, wasm_i8x16_splat(16))));
  r = wasm_v128_or(r, wasm_i8x16_swizzle(t2, wasm_i8x16_sub(idx, wasm_i8x16_splat(32))));
  r = wasm_v128_or(r, wasm_i8x16_swizzle(t3, wasm_i8x16_sub(idx, wasm_i8x16_splat(48))));
  return r;
}

static inline v128_t tbl3(v128_t t0, v128_t t1, v128_t t2, v128_t idx)
{
  v128_t r = wasm_i8x16_swizzle(t0, idx);
  r = wasm_v128_or(r, wasm_i8x16_swizzle(t1, wasm_i8x16_sub(idx, wasm_i8x16_splat(16))));
  r = wasm_v128_or(r, wasm_i8x16_swizzle(t2, wasm_i8x16_sub(idx, wasm_i8x16_splat(32))));
  return r;
}

/* (1 << nbits) - 1 for every u16 lane: smear the highest set bit downwards. */
static inline v128_t bit_smear_u16(v128_t v)
{
  v = wasm_v128_or(v, wasm_u16x8_shr(v, 1));
  v = wasm_v128_or(v, wasm_u16x8_shr(v, 2));
  v = wasm_v128_or(v, wasm_u16x8_shr(v, 4));
  v = wasm_v128_or(v, wasm_u16x8_shr(v, 8));
  return v;
}

/* nbits (= 16 - clz16) for every u16 lane: popcount of the smeared value. */
static inline v128_t nbits_u16(v128_t smeared)
{
  return wasm_u16x8_extadd_pairwise_u8x16(wasm_i8x16_popcnt(smeared));
}

/* 16 "coefficient != 0" bits (LSB first) for two rows of 8 u16 values. */
static inline uint32_t ne0_bits16(v128_t rowa, v128_t rowb)
{
  const v128_t zero = wasm_i16x8_splat(0);
  return (uint32_t)wasm_i8x16_bitmask(wasm_i8x16_narrow_i16x8(
      wasm_v128_not(wasm_i16x8_eq(rowa, zero)),
      wasm_v128_not(wasm_i16x8_eq(rowb, zero))));
}

JOCTET *jsimd_huff_encode_one_block_wasm(void *state, JOCTET *buffer,
                                         JCOEFPTR block, int last_dc_val,
                                         c_derived_tbl *dctbl,
                                         c_derived_tbl *actbl)
{
  uint16_t block_diff[DCTSIZE2];

  /* Load the 8x8 block of DCT coefficients. */
  v128_t blk0 = wasm_v128_load(block + 0 * DCTSIZE);
  v128_t blk1 = wasm_v128_load(block + 1 * DCTSIZE);
  v128_t blk2 = wasm_v128_load(block + 2 * DCTSIZE);
  v128_t blk3 = wasm_v128_load(block + 3 * DCTSIZE);
  v128_t blk4 = wasm_v128_load(block + 4 * DCTSIZE);
  v128_t blk5 = wasm_v128_load(block + 5 * DCTSIZE);
  v128_t blk6 = wasm_v128_load(block + 6 * DCTSIZE);
  v128_t blk7 = wasm_v128_load(block + 7 * DCTSIZE);

  const uint8_t *consts = jsimd_huff_encode_one_block_consts;

  /* Shuffle coefficients into zig-zag order. */
  v128_t row0 = tbl4(blk0, blk1, blk2, blk3,
                     wasm_v128_load(consts + 0 * DCTSIZE));
  v128_t row1 = tbl4(blk0, blk1, blk2, blk3,
                     wasm_v128_load(consts + 2 * DCTSIZE));
  v128_t row2 = tbl4(blk2, blk3, blk4, blk5,
                     wasm_v128_load(consts + 4 * DCTSIZE));
  v128_t row3 = tbl4(blk0, blk1, blk2, blk3,
                     wasm_v128_load(consts + 6 * DCTSIZE));
  v128_t row4 = tbl4(blk4, blk5, blk6, blk7,
                     wasm_v128_load(consts + 8 * DCTSIZE));
  v128_t row5 = tbl4(blk2, blk3, blk4, blk5,
                     wasm_v128_load(consts + 10 * DCTSIZE));
  v128_t row6 = tbl4(blk4, blk5, blk6, blk7,
                     wasm_v128_load(consts + 12 * DCTSIZE));
  v128_t row7 = tbl3(blk5, blk6, blk7,
                     wasm_v128_load(consts + 14 * DCTSIZE));

  /* Compute DC coefficient difference value (F.1.1.5.1). */
  row0 = wasm_i16x8_replace_lane(row0, 0, (int16_t)(block[0] - last_dc_val));
  /* Initialize AC coefficient lanes not reachable by the lookup tables
   * (natural-order positions 32, 12, 48, 15, 51 and 31). */
  row1 = wasm_i16x8_replace_lane(row1, 2, block[32]);
  row2 = wasm_i16x8_replace_lane(row2, 0, block[12]);
  row2 = wasm_i16x8_replace_lane(row2, 5, block[48]);
  row5 = wasm_i16x8_replace_lane(row5, 2, block[15]);
  row5 = wasm_i16x8_replace_lane(row5, 7, block[51]);
  row6 = wasm_i16x8_replace_lane(row6, 5, block[31]);

  /* DCT block is now in zig-zag order; start Huffman encoding process. */

  /* Construct bitmap to accelerate encoding of AC coefficients. Bit k of
   * the bitmap is set if zig-zag coefficient k != 0 (LSB-first, walked with
   * count-trailing-zeros below).
   */
  uint64_t bitmap = (uint64_t)ne0_bits16(row0, row1) |
                    ((uint64_t)ne0_bits16(row2, row3) << 16) |
                    ((uint64_t)ne0_bits16(row4, row5) << 32) |
                    ((uint64_t)ne0_bits16(row6, row7) << 48);
  /* Remove the DC bit. */
  bitmap &= ~(uint64_t)1;
  /* Number of non-zero AC coefficients. */
  unsigned int non_zero_coefficients =
    (unsigned int)__builtin_popcountll(bitmap);

  /* Set up state and bit buffer for the output bitstream. */
  working_state *state_ptr = (working_state *)state;
  int free_bits = state_ptr->cur.free_bits;
  unsigned long long put_buffer = state_ptr->cur.put_buffer;

  /* Encode the DC coefficient. */

  const v128_t zero = wasm_i16x8_splat(0);
  /* For negative coeffs: diff = abs(coeff) - 1 = ~abs(coeff), i.e.
   * abs ^ ((1 << nbits) - 1). The smeared absolute value is that mask. */
  v128_t abs_row0 = wasm_i16x8_abs(row0);
  v128_t row0_smear = bit_smear_u16(abs_row0);
  v128_t row0_nbits = nbits_u16(row0_smear);
  v128_t row0_diff = wasm_v128_xor(
      abs_row0, wasm_v128_and(wasm_i16x8_lt(row0, zero), row0_smear));

  unsigned int nbits = (unsigned int)wasm_u16x8_extract_lane(row0_nbits, 0);
  unsigned int diff = (unsigned int)wasm_u16x8_extract_lane(row0_diff, 0);
  /* Emit Huffman-coded symbol and additional diff bits. */
  PUT_CODE(dctbl->ehufco[nbits], dctbl->ehufsi[nbits], diff)

  /* Encode the AC coefficients. */

  unsigned int r;      /* r = run length of zeros */
  unsigned int i = 1;  /* i = number of coefficients encoded */
  /* Code and size information for a run length of 16 zero coefficients */
  const unsigned int code_0xf0 = actbl->ehufco[0xf0];
  const unsigned int size_0xf0 = actbl->ehufsi[0xf0];

  /* The most efficient method of computing nbits and diff depends on the
   * number of non-zero coefficients: with more than 8, precompute everything
   * with SIMD; otherwise compute nbits/diff on demand in the scalar loop.
   */
  if (non_zero_coefficients > 8) {
    uint8_t block_nbits[DCTSIZE2];

    v128_t abs_row1 = wasm_i16x8_abs(row1);
    v128_t abs_row2 = wasm_i16x8_abs(row2);
    v128_t abs_row3 = wasm_i16x8_abs(row3);
    v128_t abs_row4 = wasm_i16x8_abs(row4);
    v128_t abs_row5 = wasm_i16x8_abs(row5);
    v128_t abs_row6 = wasm_i16x8_abs(row6);
    v128_t abs_row7 = wasm_i16x8_abs(row7);
    v128_t row1_smear = bit_smear_u16(abs_row1);
    v128_t row2_smear = bit_smear_u16(abs_row2);
    v128_t row3_smear = bit_smear_u16(abs_row3);
    v128_t row4_smear = bit_smear_u16(abs_row4);
    v128_t row5_smear = bit_smear_u16(abs_row5);
    v128_t row6_smear = bit_smear_u16(abs_row6);
    v128_t row7_smear = bit_smear_u16(abs_row7);

    /* Store nbits (values <= 16, so the saturating narrow is exact). */
    wasm_v128_store(block_nbits + 0 * DCTSIZE,
                    wasm_u8x16_narrow_i16x8(row0_nbits,
                                            nbits_u16(row1_smear)));
    wasm_v128_store(block_nbits + 2 * DCTSIZE,
                    wasm_u8x16_narrow_i16x8(nbits_u16(row2_smear),
                                            nbits_u16(row3_smear)));
    wasm_v128_store(block_nbits + 4 * DCTSIZE,
                    wasm_u8x16_narrow_i16x8(nbits_u16(row4_smear),
                                            nbits_u16(row5_smear)));
    wasm_v128_store(block_nbits + 6 * DCTSIZE,
                    wasm_u8x16_narrow_i16x8(nbits_u16(row6_smear),
                                            nbits_u16(row7_smear)));

    /* diff = abs(coeff) ^ ((1 << nbits) - 1) for negative coefficients
     * [no-op for positive ones]. */
    v128_t row1_diff = wasm_v128_xor(
        abs_row1, wasm_v128_and(wasm_i16x8_lt(row1, zero), row1_smear));
    v128_t row2_diff = wasm_v128_xor(
        abs_row2, wasm_v128_and(wasm_i16x8_lt(row2, zero), row2_smear));
    v128_t row3_diff = wasm_v128_xor(
        abs_row3, wasm_v128_and(wasm_i16x8_lt(row3, zero), row3_smear));
    v128_t row4_diff = wasm_v128_xor(
        abs_row4, wasm_v128_and(wasm_i16x8_lt(row4, zero), row4_smear));
    v128_t row5_diff = wasm_v128_xor(
        abs_row5, wasm_v128_and(wasm_i16x8_lt(row5, zero), row5_smear));
    v128_t row6_diff = wasm_v128_xor(
        abs_row6, wasm_v128_and(wasm_i16x8_lt(row6, zero), row6_smear));
    v128_t row7_diff = wasm_v128_xor(
        abs_row7, wasm_v128_and(wasm_i16x8_lt(row7, zero), row7_smear));

    /* Store diff bits. */
    wasm_v128_store(block_diff + 0 * DCTSIZE, row0_diff);
    wasm_v128_store(block_diff + 1 * DCTSIZE, row1_diff);
    wasm_v128_store(block_diff + 2 * DCTSIZE, row2_diff);
    wasm_v128_store(block_diff + 3 * DCTSIZE, row3_diff);
    wasm_v128_store(block_diff + 4 * DCTSIZE, row4_diff);
    wasm_v128_store(block_diff + 5 * DCTSIZE, row5_diff);
    wasm_v128_store(block_diff + 6 * DCTSIZE, row6_diff);
    wasm_v128_store(block_diff + 7 * DCTSIZE, row7_diff);

    while (bitmap != 0) {
      unsigned int next = (unsigned int)__builtin_ctzll(bitmap);
      r = next - i;
      i = next;
      nbits = block_nbits[i];
      diff = block_diff[i];
      while (r > 15) {
        /* If run length > 15, emit special run-length-16 codes. */
        PUT_BITS(code_0xf0, size_0xf0)
        r -= 16;
      }
      /* Emit Huffman symbol for run length / number of bits. (F.1.2.2.1) */
      unsigned int rs = (r << 4) + nbits;
      PUT_CODE(actbl->ehufco[rs], actbl->ehufsi[rs], diff)
      i++;
      bitmap &= bitmap - 1;
    }
  } else if (bitmap != 0) {
    uint16_t block_abs[DCTSIZE2];
    /* Compute and store the absolute value of the coefficients. */
    v128_t abs_row1 = wasm_i16x8_abs(row1);
    v128_t abs_row2 = wasm_i16x8_abs(row2);
    v128_t abs_row3 = wasm_i16x8_abs(row3);
    v128_t abs_row4 = wasm_i16x8_abs(row4);
    v128_t abs_row5 = wasm_i16x8_abs(row5);
    v128_t abs_row6 = wasm_i16x8_abs(row6);
    v128_t abs_row7 = wasm_i16x8_abs(row7);
    wasm_v128_store(block_abs + 0 * DCTSIZE, abs_row0);
    wasm_v128_store(block_abs + 1 * DCTSIZE, abs_row1);
    wasm_v128_store(block_abs + 2 * DCTSIZE, abs_row2);
    wasm_v128_store(block_abs + 3 * DCTSIZE, abs_row3);
    wasm_v128_store(block_abs + 4 * DCTSIZE, abs_row4);
    wasm_v128_store(block_abs + 5 * DCTSIZE, abs_row5);
    wasm_v128_store(block_abs + 6 * DCTSIZE, abs_row6);
    wasm_v128_store(block_abs + 7 * DCTSIZE, abs_row7);
    /* Compute diff bits (without the nbits mask) and store. */
    v128_t row1_diff = wasm_v128_xor(abs_row1, wasm_i16x8_lt(row1, zero));
    v128_t row2_diff = wasm_v128_xor(abs_row2, wasm_i16x8_lt(row2, zero));
    v128_t row3_diff = wasm_v128_xor(abs_row3, wasm_i16x8_lt(row3, zero));
    v128_t row4_diff = wasm_v128_xor(abs_row4, wasm_i16x8_lt(row4, zero));
    v128_t row5_diff = wasm_v128_xor(abs_row5, wasm_i16x8_lt(row5, zero));
    v128_t row6_diff = wasm_v128_xor(abs_row6, wasm_i16x8_lt(row6, zero));
    v128_t row7_diff = wasm_v128_xor(abs_row7, wasm_i16x8_lt(row7, zero));
    wasm_v128_store(block_diff + 0 * DCTSIZE, row0_diff);
    wasm_v128_store(block_diff + 1 * DCTSIZE, row1_diff);
    wasm_v128_store(block_diff + 2 * DCTSIZE, row2_diff);
    wasm_v128_store(block_diff + 3 * DCTSIZE, row3_diff);
    wasm_v128_store(block_diff + 4 * DCTSIZE, row4_diff);
    wasm_v128_store(block_diff + 5 * DCTSIZE, row5_diff);
    wasm_v128_store(block_diff + 6 * DCTSIZE, row6_diff);
    wasm_v128_store(block_diff + 7 * DCTSIZE, row7_diff);

    /* Same as above but must mask diff bits and compute nbits on demand. */
    while (bitmap != 0) {
      unsigned int next = (unsigned int)__builtin_ctzll(bitmap);
      r = next - i;
      i = next;
      unsigned int lz = (unsigned int)__builtin_clz(block_abs[i]);
      nbits = 32 - lz;
      diff = ((unsigned int)block_diff[i] << lz) >> lz;
      while (r > 15) {
        /* If run length > 15, emit special run-length-16 codes. */
        PUT_BITS(code_0xf0, size_0xf0)
        r -= 16;
      }
      /* Emit Huffman symbol for run length / number of bits. (F.1.2.2.1) */
      unsigned int rs = (r << 4) + nbits;
      PUT_CODE(actbl->ehufco[rs], actbl->ehufsi[rs], diff)
      i++;
      bitmap &= bitmap - 1;
    }
  }

  /* If the last coefficient(s) were zero, emit an end-of-block (EOB) code.
   * The value of RS for the EOB code is 0.
   */
  if (i != 64) {
    PUT_BITS(actbl->ehufco[0], actbl->ehufsi[0])
  }

  state_ptr->cur.put_buffer = put_buffer;
  state_ptr->cur.free_bits = free_bits;

  return buffer;
}
