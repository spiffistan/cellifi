#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <spu_intrinsics.h>
#include "cell_util.h"
#include "spu.h"
#include "spu_tables.h"

#define ISQRT2 0.70710678118654f
vector unsigned char trans_01 = (vector unsigned char){ 
    0x00, 0x01, 0x02, 0x03,
    0x10, 0x11, 0x12, 0x13,
    0x04, 0x05, 0x06, 0x07,
    0x14, 0x15, 0x16, 0x17
};

vector unsigned char trans_10 = (vector unsigned char){ 
    0x04, 0x05, 0x06, 0x07,
    0x14, 0x15, 0x16, 0x17,
    0x00, 0x01, 0x02, 0x03,
    0x10, 0x11, 0x12, 0x13
};

vector unsigned char trans_23 = (vector unsigned char){
    0x08, 0x09, 0x0A, 0x0B,
    0x18, 0x19, 0x1A, 0x1B,
    0x0C, 0x0D, 0x0E, 0x0F,
    0x1C, 0x1D, 0x1E, 0x1F
};
vector unsigned char trans_32 = (vector unsigned char){
    0x0C, 0x0D, 0x0E, 0x0F,
    0x1C, 0x1D, 0x1E, 0x1F,
    0x08, 0x09, 0x0A, 0x0B,
    0x18, 0x19, 0x1A, 0x1B
};

vector unsigned char trans_merge_even = (vector unsigned char){ 
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x18, 0x19, 0x1A, 0x1B,
    0x1C, 0x1D, 0x1E, 0x1F,
};
vector unsigned char trans_merge_odd = (vector unsigned char){ 
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17,
};

static void transpose_block(vector float *in_data, vector float *out_data)
{

    vector float tmp1;
    vector float tmp2;
    tmp1 = spu_shuffle(in_data[0], in_data[2], trans_01);
    tmp2 = spu_shuffle(in_data[4], in_data[6], trans_10);
    out_data[0] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[2] = spu_shuffle(tmp1, tmp2, trans_merge_odd);

    tmp1 = spu_shuffle(in_data[8], in_data[10], trans_01);
    tmp2 = spu_shuffle(in_data[12], in_data[14], trans_10);
    out_data[1] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[3] = spu_shuffle(tmp1, tmp2, trans_merge_odd);
    
    tmp1 = spu_shuffle(in_data[0], in_data[2], trans_23);
    tmp2 = spu_shuffle(in_data[4], in_data[6], trans_32);
    out_data[4] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[6] = spu_shuffle(tmp1, tmp2, trans_merge_odd);

    tmp1 = spu_shuffle(in_data[8], in_data[10], trans_23);
    tmp2 = spu_shuffle(in_data[12], in_data[14], trans_32);
    out_data[5] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[7] = spu_shuffle(tmp1, tmp2, trans_merge_odd);

    tmp1 = spu_shuffle(in_data[1], in_data[3], trans_01);
    tmp2 = spu_shuffle(in_data[5], in_data[7], trans_10);
    out_data[8] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[10] = spu_shuffle(tmp1, tmp2, trans_merge_odd);

    tmp1 = spu_shuffle(in_data[9], in_data[11], trans_01);
    tmp2 = spu_shuffle(in_data[13], in_data[15], trans_10);
    out_data[9] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[11] = spu_shuffle(tmp1, tmp2, trans_merge_odd);
    
    tmp1 = spu_shuffle(in_data[1], in_data[3], trans_23);
    tmp2 = spu_shuffle(in_data[5], in_data[7], trans_32);
    out_data[12] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[14] = spu_shuffle(tmp1, tmp2, trans_merge_odd);

    tmp1 = spu_shuffle(in_data[9], in_data[11], trans_23);
    tmp2 = spu_shuffle(in_data[13], in_data[15], trans_32);
    out_data[13] = spu_shuffle(tmp1, tmp2, trans_merge_even);
    out_data[15] = spu_shuffle(tmp1, tmp2, trans_merge_odd);
}

// void test_transpose_block() {
//     
//     float mat[64];
//     int i,j, k, counter = 0;
// 
//     vector float mat_v[16];
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 mat_v[i*2 +j] = spu_insert((float)counter++, mat_v[i*2 +j], k);
//             }
//         }
//     }
//     vector float out[16];
//     transpose_block(mat_v, out);
//     printf("\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 printf(", %.2f", spu_extract(out[i * 2 + j], k));
//             }
//         }
//         printf("\n");
//     }
//     exit(0);    
// }



static void dct_1d(vector float *in_data, vector float *out_data)
{
    int i,j;

    for (j=0; j<8; ++j)
    {
        vector float dct = spu_xor(*in_data, *in_data);

        for (i=0; i<2; ++i)
        {
            dct = spu_madd(in_data[i], dct_lookup_transposed[j][i], dct);
        }
        
        float d = spu_extract(dct, 0) + spu_extract(dct, 1) + spu_extract(dct,2) + spu_extract(dct,3);
        out_data[j/4] = spu_insert(d, out_data[j/4], j % 4);
    }
}

// static void pre_dct_1d(float *in_data, float *out_data)
// {
//     int i,j;
// 
//     for (j=0; j<8; ++j)
//     {
//         float dct = 0;
// 
//         for (i=0; i<8; ++i)
//         {
//             dct += in_data[i] * dctlookup_pre[i][j];
//         }
// 
//         out_data[j] = dct;
//     }
// }
// 
// void test_dct_1d() {
//     float in[64], out[64];
//     int i,j, k, counter = 0;
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             in[i*8+j] = (float) i * 8 + j;
//         }
//     }
// 
//     pre_dct_1d(in,out);
// 
//     vector float mat_v[16];
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 mat_v[i*2 +j] = spu_insert((float)counter++, mat_v[i*2 +j], k);
//             }
//         }
//     }
//     vector float out_v[16];
//     dct_1d(mat_v, out_v);
//     printf("\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 printf(", %.2f", spu_extract(out_v[i * 2 + j], k));
//             }
//         }
//         printf("\n");
//     }
//     printf("\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             printf(", %.2f", out[i*8+j]);
//         }
//         printf("\n");
//     }
//     fflush(stdout);
//     exit(0);    
// }



static void idct_1d(vector float *in_data, vector float *out_data)
{
    int i,j;

    for (j=0; j<8; ++j)
    {
        vector float idct = spu_xor(*in_data, *in_data);

        for (i=0; i<2; ++i)
        {
            idct = spu_madd(in_data[i] , dctlookup[j][i], idct);
        }
        float val = spu_extract(idct, 0) + spu_extract(idct,1) + spu_extract(idct,2) + spu_extract(idct, 3);
        out_data[j/4] = spu_insert(val, out_data[j/4], j % 4);
    }
}

// static void pre_idct_1d(float *in_data, float *out_data)
// {
//     int i,j;
// 
//     for (j=0; j<8; ++j)
//     {
//         float idct = 0;
// 
//         for (i=0; i<8; ++i)
//         {
//             idct += in_data[i] * dctlookup_pre[j][i];
//         }
// 
//         out_data[j] = idct;
//     }
// }
// void test_idct_1d() {
//     float in[64], out[64];
//     int i,j, k, counter = 0;
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             in[i*8+j] = (float) i * 8 + j;
//         }
//     }
// 
//     pre_idct_1d(in,out);
// 
//     vector float mat_v[16];
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 mat_v[i*2 +j] = spu_insert((float)counter++, mat_v[i*2 +j], k);
//             }
//         }
//     }
//     vector float out_v[16];
//     idct_1d(mat_v, out_v);
//     printf("\nCELL:\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 printf(", %.2f", spu_extract(out_v[i * 2 + j], k));
//             }
//         }
//         printf("\n");
//     }
//     printf("\nPRE:\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             printf(", %.2f", out[i*8+j]);
//         }
//         printf("\n");
//     }
//     fflush(stdout);
//     exit(0);    
// }

vector float scale[2*8]  = { 
    (vector float) { ISQRT2*ISQRT2, ISQRT2, ISQRT2,ISQRT2 },   (vector float) { ISQRT2, ISQRT2, ISQRT2,ISQRT2 }, 
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f },
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f },
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f },
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f },
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f },
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f },
    (vector float) { ISQRT2, 1.0f, 1.0f,1.0f },         (vector float) { 1.0f, 1.0f, 1.0f,1.0f }
    };
static void scale_block(vector float *in_data,vector float *out_data)
{
    int u,v;

    for (v=0; v<8; ++v)
    {
        for (u=0; u<2; ++u)
        {
            /* Scale according to normalizing function */
            out_data[v*2+u] = spu_mul(scale[v*2+u], in_data[v*2+u]);
        }
    }
}


static void quantize_block(vector float *in_data, vector float *out_data, uint8_t cc)
{
    int zigzag;
    for (zigzag=0; zigzag < 64; ++zigzag)
    {
        uint8_t u = zigzag_U[zigzag];
        uint8_t v = zigzag_V[zigzag];

        float dct = spu_extract(in_data[(v << 1)+(u >> 2)], (u & 3));
        dct = round((dct * 0.25) / q_table[cc][zigzag]);
        /* Zig-zag and quantize */
        out_data[zigzag >> 2] = spu_insert(dct, out_data[zigzag >> 2], zigzag & 3);
    }
}


// static void pre_quantize_block(float *in_data, float *out_data)
// {
//     int zigzag;
//     for (zigzag=0; zigzag < 64; ++zigzag)
//     {
//         uint8_t u = zigzag_U[zigzag];
//         uint8_t v = zigzag_V[zigzag];
// 
//         float dct = in_data[v*8+u];
// 
//         /* Zig-zag and quantize */
//         out_data[zigzag] = round((dct / 4.0) / q_table[0][zigzag]);
//     }
// }
// void test_quantize_block() {
//     float in[64], out[64];
//     int i,j, k, counter = 0;
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             in[i*8+j] = (float) i * 8 + j;
//         }
//     }
// 
//     pre_quantize_block(in,out);
// 
//     vector float mat_v[16];
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 mat_v[i*2 +j] = spu_insert((float)counter++, mat_v[i*2 +j], k);
//             }
//         }
//     }
//     vector float out_v[16];
//     quantize_block(mat_v, out_v, 0);
//     printf("\nCELL:\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 printf(", %.2f", spu_extract(out_v[i * 2 + j], k));
//             }
//         }
//         printf("\n");
//     }
//     printf("\nPRE:\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             printf(", %.2f", out[i*8+j]);
//         }
//         printf("\n");
//     }
//     fflush(stdout);
//     exit(0);    
// }

static void dequantize_block(vector float *in_data, vector float *out_data, uint8_t cc)
{
    int zigzag, i;
    for (zigzag=0; zigzag < 64; zigzag ++)
    {
        
        /* Zig-zag and de-quantize */
            uint8_t u = zigzag_U[zigzag];
            uint8_t v = zigzag_V[zigzag];
            float dct = spu_extract(in_data[zigzag>>2], zigzag & 3);
            
            dct = round(dct * q_table[cc][zigzag] * 0.25);
            
            out_data[v * 2 + (u >> 2)] = spu_insert(dct, out_data[v * 2 + (u >> 2)], (u & 3));
    }
}

// static void pre_dequantize_block(float *in_data, float *out_data)
// {
//     int zigzag;
//     for (zigzag=0; zigzag < 64; ++zigzag)
//     {
//         uint8_t u = zigzag_U[zigzag];
//         uint8_t v = zigzag_V[zigzag];
// 
//         float dct = in_data[zigzag];
// 
//         /* Zig-zag and de-quantize */
//         out_data[v*8+u] = round((dct * q_table[0][zigzag]) / 4.0);
//     }
// }
// 
// void test_dequantize_block() {
//     float in[64], out[64];
//     int i,j, k, counter = 0;
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             in[i*8+j] = (float) i * 8 + j;
//         }
//     }
// 
//     pre_dequantize_block(in,out);
// 
//     vector float mat_v[16];
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 mat_v[i*2 +j] = spu_insert((float)counter++, mat_v[i*2 +j], k);
//             }
//         }
//     }
//     vector float out_v[16];
//     dequantize_block(mat_v, out_v, 0);
//     printf("\nCELL:\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 2; j++) {
//             for(k = 0; k < 4; k++) {
//                 printf(", %.2f", spu_extract(out_v[i * 2 + j], k));
//             }
//         }
//         printf("\n");
//     }
//     printf("\nPRE:\n");
//     for(i = 0; i < 8; i++) {
//         for(j = 0; j < 8; j++) {
//             printf(", %.2f", out[i*8+j]);
//         }
//         printf("\n");
//     }
//     fflush(stdout);
//     exit(0);    
// }
void spu_dct_quant_block_8x8(uint8_t cc)
{
    vector float  mb[8*2] __attribute((aligned(16)));
    vector float mb2[8*2] __attribute((aligned(16)));

    int i, v;

    for (i=0; i<16; ++i)
        mb2[i] = spu_sub(original[i], predicted[i]);
    
    for (v=0; v<8; ++v)
    {
        dct_1d(&mb2[v*2], &mb[v*2]);
    }

    transpose_block(mb, mb2);

    for (v=0; v<8; ++v)
    {
        dct_1d(&mb2[v*2], &mb[v*2]);
    }

    transpose_block(mb, mb2);
    scale_block(mb2, mb);
    quantize_block(mb, mb2, cc);

    for (i=0; i<16; ++i) {
        residual[i] = mb2[i];
    }
}


void spu_dequant_idct_block_8x8(uint8_t cc)
{
    vector float mb[2*8] __attribute((aligned(16)));
    vector float mb2[2*8] __attribute((aligned(16)));

    int i, v;

    for (i=0; i<16; ++i)
        mb[i] = residual[i];

    dequantize_block(mb, mb2, cc);

    scale_block(mb2, mb);

    for (v=0; v<8; ++v)
    {
        idct_1d(mb+v*2, mb2+v*2);
    }

    transpose_block(mb2, mb);

    for (v=0; v<8; ++v)
    {
        idct_1d(mb+v*2, mb2+v*2);
    }

    transpose_block(mb2, mb);

    for (i=0; i<16; ++i) {
        reconstructed[i] = mb[i] + predicted[i];
    }
}

