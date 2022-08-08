#ifndef __FWHT__
#define __FWHT__

//================================================
// @title        fwht.h
// @author       Jonathan Hadida
// @contact      Jonathan.hadida [at] dtc.ox.ac.uk
//================================================

#include <cmath>
#include <vector>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <QtCore/qglobal.h>
#include<QDebug>


#include <immintrin.h> // avx+fma
#include <assert.h> // assert
#include <string.h> // memcpy


#define for_i(n) for ( unsigned i=0; i<n; ++i )



/********************     **********     ********************/
/********************     **********     ********************/



/**
 * Count the number of bits set.
 */
template <typename IntType>
unsigned bit_count( IntType n )
{
    unsigned c;
    for (c = 0; n; ++c)
        n &= n - 1;
    return c;
}

// ------------------------------------------------------------------------

/**
 * Reverse the bits (left-right) of an integer.
 */
template <typename IntType>
IntType bit_reverse( IntType n, unsigned nbits )
{
    IntType r = n;
    IntType l, m;

    // Compute shift and bitmask
    l = (IntType) (nbits - 1);
    m = (1 << nbits) - 1;

    // Permute
    while ( n >>= 1 )
    {
        r <<= 1;
        r |= n & 1;
        --l;
    }

    // Final shift and masking
    r <<= l;
    return r & m;
}

template <typename IntType>
inline IntType bit_reverse( IntType n )
{
    return bit_reverse( n, 8*sizeof(n) );
}

// ------------------------------------------------------------------------

/**
 * Print the bits of an integer.
 */
template <typename IntType>
void bit_print( IntType n )
{
    static const unsigned b = 8*sizeof(IntType);
    static char buf[b+1]; buf[b] = '\0';

    std::cout<< n;
    for (unsigned m = b; m; n >>= 1, --m)
        buf[m-1] = (n & 1) + '0';
    printf(" = %s\n",buf);
}

// ------------------------------------------------------------------------

/**
 * Gray codes transforms.
 */
template <typename IntType>
inline IntType bin2gray( IntType n )
{
    return (n >> 1) ^ n;
}

// ------------------------------------------------------------------------

/**
 * Integer logarithm base 2.
 */
template <typename IntType>
unsigned ilog2( IntType n )
{
    unsigned l;
    for ( l = 0; n; n >>= 1, ++l );
    return l;
}

// ------------------------------------------------------------------------

/**
 * Orthogonal transformation.
 */
template <typename T>
inline void rotate( T& a, T& b )
{
    static T A;
    A = a;
    a = A + b;
    b = A - b;
}

// ------------------------------------------------------------------------

/**
 * Compute the permutation of WH coefficients to sort them by sequency.
 */
template <typename IntType>
void fwht_sequency_permutation( QVector<IntType>& perm, unsigned order )
{
    //if ( perm.size() == (size_t)(1 << order) ) return;
    //perm.resize(1 << order);
    IntType k = 0;
    for ( auto& p: perm ) p = bit_reverse(bin2gray(k++),order);
}


void fwht_sequency_permutation( unsigned *perm, unsigned int order )
{
    int n = 1<<order;
    for (int k =0; k < n; k++ ) {
        perm[k] = bit_reverse(bin2gray(k),order);
    }
}

// ------------------------------------------------------------------------
/**
 * Fast Walsh-Hadamard transform.
 */
template <typename T, typename T2>
void fwht( T &data, unsigned n,
           T &out,
           T2 &sequency_ordering,
           bool normalize = true )
{
    // Require n to be a power of 2
    unsigned order = ilog2(n) - 1;

    // Compute the WHT
    for ( unsigned i = 0; i < order; ++i )
    {
        for ( unsigned j = 0; j < n; j += (1 << (i+1)) )
            for ( unsigned k = 0; k < (unsigned)(1 << i); ++k )
                rotate( data[j+k], data[j+ k + (unsigned)(1<<i)] );
    }
    //qDebug() << "Completed in-place transfer";

    // Copy transform
    for_i(n) out[i] = data[sequency_ordering[i]];

    //qDebug() << "Completed re-ordering";

    //if ( normalize )
    //    for_i(n) out[i] = out[i]/norm;
}

//TODO: make float16 / int16 versions
// todo - use SIMD, either direct SSE/AVX or using GCC Vector extensions....
// http://gcc.gnu.org/onlinedocs/gcc-4.6.0/gcc/Vector-Extensions.html#Vector-Extensions
// compare with QT's vectorized function qFromLittleEndian(src, dest)...


// https://www.qt.io/blog/2010/08/24/improving-the-rendering-performance-with-more-simd
// https://software.intel.com/sites/landingpage/IntrinsicsGuide/
// https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
void fwht_iterative_simd(double *dst, double *src, unsigned *seq, unsigned len)
{
    static double *scratch_space;
    static unsigned scratch_space_len = 0;

    unsigned bit, dbit, j, k, l, r, p;
    double temp;


    if (len < 4)
        return;

    // handles length-2 transforms, and places data in output vector
    // assume we have at least 4 values in our array
    static __m128d tmp1, tmp2, sum1, sub1;
    for (j = 0; j < len; j+=2) {
        k = j+1;
        dst[j] = src[j] + src[k];
        dst[k] = src[j] - src[k];
    }

//    static __m128d sumdiff;
//    for (int j = 0; j < len; j+=2) {
//        tmp1 = _mm_load_pd(src + j);
//        tmp2 = _mm_permute_pd(tmp1, 1);
//        sumdiff = _mm_permute_pd(_mm_addsub_pd(tmp1, tmp2),1);
//        _mm_store_pd(dst + j, sumdiff);
//    }


    // handles length-4 transforms

    for (j = 0; j < len; j+=4) {
        k = j + 2;

        tmp1 = _mm_load_pd(dst + j);
        tmp2 = _mm_load_pd(dst + k);

        sum1 = _mm_add_pd(tmp1, tmp2);
        sub1 = _mm_sub_pd(tmp1, tmp2);

        _mm_store_pd(dst + j, sum1);
        _mm_store_pd(dst + k, sub1);
    }

    if (len < 8)
        return;

    // handles length-8 transforms
    static __m256d tmp3, tmp4, sum2, sub2;
    for (j = 0; j < len; j+=8) {
        k = j + 4;

        tmp3 = _mm256_load_pd(dst + j);
        tmp4 = _mm256_load_pd(dst + k);

        sum2 = _mm256_add_pd(tmp3, tmp4);
        sub2 = _mm256_sub_pd(tmp3, tmp4);

        _mm256_store_pd(dst + j, sum2);
        _mm256_store_pd(dst + k, sub2);
    }

    if (len < 16)
        return;

    // handles length-16 transforms
    static __m256d tmp5, tmp6, sum3, sub3;
    for (j = 0; j < len; j+=16) {
        k = j + 8;

        tmp3 = _mm256_load_pd(dst + j);
        tmp4 = _mm256_load_pd(dst + k);

        sum2 = _mm256_add_pd(tmp3, tmp4);
        sub2 = _mm256_sub_pd(tmp3, tmp4);

        _mm256_store_pd(dst + j, sum2);
        _mm256_store_pd(dst + k, sub2);

        l = j + 4;
        r = j + 12;

        tmp5 = _mm256_load_pd(dst + l);
        tmp6 = _mm256_load_pd(dst + r);

        sum3 = _mm256_add_pd(tmp5, tmp6);
        sub3 = _mm256_sub_pd(tmp5, tmp6);

        _mm256_store_pd(dst + l, sum3);
        _mm256_store_pd(dst + r, sub3);
    }

    if (len < 32)
        return;

    // handles length-32 transforms
    for (j = 0; j < len; j+=32) {
        k = j + 16;

        tmp3 = _mm256_load_pd(dst + j);
        tmp4 = _mm256_load_pd(dst + k);

        sum2 = _mm256_add_pd(tmp3, tmp4);
        sub2 = _mm256_sub_pd(tmp3, tmp4);

        _mm256_store_pd(dst + j, sum2);
        _mm256_store_pd(dst + k, sub2);

        l = j + 4;
        r = j + 20;

        tmp5 = _mm256_load_pd(dst + l);
        tmp6 = _mm256_load_pd(dst + r);

        sum3 = _mm256_add_pd(tmp5, tmp6);
        sub3 = _mm256_sub_pd(tmp5, tmp6);

        _mm256_store_pd(dst + l, sum3);
        _mm256_store_pd(dst + r, sub3);

        p = j + 8;
        k = j + 24;

        tmp3 = _mm256_load_pd(dst + p);
        tmp4 = _mm256_load_pd(dst + k);

        sum2 = _mm256_add_pd(tmp3, tmp4);
        sub2 = _mm256_sub_pd(tmp3, tmp4);

        _mm256_store_pd(dst + p, sum2);
        _mm256_store_pd(dst + k, sub2);

        l = j + 12;
        r = j + 28;

        tmp5 = _mm256_load_pd(dst + l);
        tmp6 = _mm256_load_pd(dst + r);

        sum3 = _mm256_add_pd(tmp5, tmp6);
        sub3 = _mm256_sub_pd(tmp5, tmp6);

        _mm256_store_pd(dst + l, sum3);
        _mm256_store_pd(dst + r, sub3);
    }

    // handles all remaining length-(bit*2) transforms
    for (bit = 32; bit < len; bit = dbit) {
        dbit = bit << 1;
        for (j = 0; j < len; j += dbit) {
            for (l = j; l < j+bit; l+=4) {
                k = l | bit;

                tmp3 = _mm256_load_pd(dst + l);
                tmp4 = _mm256_load_pd(dst + k);

                sum2 = _mm256_add_pd(tmp3, tmp4);
                sub2 = _mm256_sub_pd(tmp3, tmp4);

                _mm256_store_pd(dst + l, sum2);
                _mm256_store_pd(dst + k, sub2);
            }
        }
    }

    //  add sequency remap...

    if(seq != nullptr){
        if(scratch_space_len != len){
            if(scratch_space_len > 0) {
                qDebug() << "Freeing Scratch WHT Transform Buffers: " << scratch_space_len;
                _mm_free(scratch_space);
            }
            qDebug() << "Allocating Scratch WHT Transform Buffers" << len;
            scratch_space = ( double *)_mm_malloc(len * sizeof(double), 256);
            scratch_space_len = len;
        }

        // copy the tx to scratch space
        for(int i = 0; i < len; i+=4) {
            _mm256_store_pd(&scratch_space[i], _mm256_load_pd(&dst[i]));
        }
        // sequency order back to dst
        for(int i = 0; i < len; i++) {
            dst[i] = scratch_space[seq[i]];
        }
    }
    // todo think of a good place to release the scratch...
}


void fwht_wiki_simd(double *dst, double *src, unsigned int *seq, unsigned int n){
    unsigned h = 1, i, j, k;

    // special case for h == 1: use a sum/difference SIMD
//    static __m128d tmp1, tmp2, sumdiff;
//    for (j = 0; j < n; j+=2) {
//        tmp1 = _mm_load_pd(src + j);
//        tmp2 = _mm_permute_pd(tmp1, 1);
//        sumdiff = _mm_permute_pd(_mm_addsub_pd(tmp1, tmp2),1);
//        _mm_store_pd(src + j, sumdiff);
//    }
    int a,b;
    for (j = 0; j < n; j+=2) {
        k = j+1;
        a = src[j];
        b = src[k];
        src[j] = a + b;
        src[k] = a - b;
    }


    h <<= 1;
    static __m128d x1x2, y1y2;
    while(h < n){
        for(i = 0; i< n; i += h<<1){
            for(j = i; j< i + h; j+=2){
                x1x2 = _mm_load_pd(src + j);
                y1y2 = _mm_load_pd(src + j + h);
                _mm_store_pd(src + j, _mm_add_pd(x1x2,y1y2));
                _mm_store_pd(src + j + h, _mm_sub_pd(x1x2,y1y2));
            }
        }
        h <<= 1;
    }

    //  add sequency remap...
    for(int i = 0; i < n; i++){
        dst[i] = src[seq[i]];
    }
}
//
//void fwht_wiki_simd2(double *dst, double *src, unsigned int *seq, unsigned int n){
//    unsigned h = 1;
//
//    // special case for h == 1: use a sum/difference SIMD
//    static __m128d tmp1, tmp2, sumdiff;
//    for (int j = 0; j < n; j+=2) {
//        tmp1 = _mm_load_pd(src + j);
//        tmp2 = _mm_permute_pd(tmp1, 1);
//        sumdiff = _mm_permute_pd(_mm_addsub_pd(tmp1, tmp2),1);
//        _mm_store_pd(src + j, sumdiff);
//    }
//
//    h <<= 1;
//    static __m128d x1x2, y1y2;
//    while(h < n){
//        for(int i = 0; i< n; i += h<<1){
//            for(int j = i; j< i + h; j+=2){
//                x1x2 = _mm_load_pd(src + j);
//                y1y2 = _mm_load_pd(src + j + h);
//                _mm_store_pd(src + j, _mm_add_pd(x1x2,y1y2));
//                _mm_store_pd(src + j + h, _mm_sub_pd(x1x2,y1y2));
//            }
//        }
//        h <<= 1;
//    }
//
//    //  add sequency remap...
//    for(int i = 0; i < n; i++){
//        dst[i] = src[seq[i]];
//    }
//}




// ------------------------------------------------------------------------

template <typename T>
void ifwht( T &data, unsigned n,
            bool sequency_ordered = false )
{
    fwht( data, n, sequency_ordered, false );
}

#endif