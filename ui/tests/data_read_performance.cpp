//
// Created by Ismail Degani on 6/16/2021.
//

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtGui/QtGui>
#include <mmintrin.h>
#include <stdint.h>
#include <string.h>

#define NO_SIMD 0
#define SIMD1   1
#define SIMD2   2
#define SIMD3   3

#define printf(args...) fprintf(stderr, ##args)

void process_no_simd(const double *dst, const quint32_le &n_items, const QByteArray &items);

inline void print_bytes(const char* var, int len)
{
    printf("RAW: ");
    for(int i = 0; i < len; i++){
        printf("%hhx ", var[i]);
    }
    printf("\n");
}

inline void print_128(__m128 var)
{
    float_t val[4];
    memcpy(val, &var, sizeof(val));
    printf("__m128: %i %i %i %i \n",
           val[0], val[1], val[2], val[3]);
}

void print_128i(__m128i var)
{
    uint16_t val[8];
    memcpy(val, &var, sizeof(val));
    printf("128i: %i %i %i %i %i %i %i %i \n",
           val[0], val[1], val[2], val[3], val[4], val[5],
           val[6], val[7]);
}

inline void print_m64(__m64 var)
{
    uint16_t val[4];
    memcpy(val, &var, sizeof(val));
    printf("__m64: %i %i %i %i \n",
           val[0], val[1], val[2], val[3]);
}

inline void print_m256d(__m256d var)
{
    double_t val[4];
    memcpy(val, &var, sizeof(val));
    printf("__m256d: %i %i %i %i \n",
           val[0], val[1], val[2], val[3]);
}

inline void process_no_simd(double *dst, const quint32_le &n_items, const QByteArray &items) {
    for (int i = 0; i < n_items; i++) {
        dst[i] = (double)qFromLittleEndian<quint16_le>(items.mid(i*2, 2));;
    }
}

inline void process_simd1(double *dst, const quint32_le &n_items, const QByteArray &items) {
    static __m128 tmp_f;
    static __m64 tmp_16i;
    uint16_t *uint16_data = (uint16_t*)items.constData();

    for(int i = 0; i < n_items; i += 4) {
        // copy out into an aligned m64 buffer
        memcpy(&tmp_16i, &uint16_data[i], sizeof(__m64) );
        // unpack & convert to 4 single-precision floating point values
        tmp_f = _mm_cvtpu16_ps(tmp_16i);
        // unpack & convert to 4 double-precision floating point values
        _mm256_store_pd(&dst[i], _mm256_cvtps_pd(tmp_f));
    }
}

inline void process_simd2(double *dst, const quint32_le &n_items, const QByteArray &items) {
    static __m128i tmp_128i;
    static __m256i tmp_256i;
    static __m128i *tmp_256i_p = (__m128i*)&tmp_256i;
    uint16_t *uint16_data = (uint16_t*)items.constData();

    for(int i = 0; i < n_items; i += 8) {
        // load up 128-bits of 16-bit integers (8 of them) and zero extend to 32-bit
        // change to a memcpy if aligned data is needed...
        tmp_128i = _mm_loadu_si128((__m128i_u*)&uint16_data[i]);

        // Since we don't have AVX2...
        tmp_256i_p[0] = _mm_cvtepu16_epi32 (tmp_128i);
        tmp_256i_p[1] = _mm_cvtepu16_epi32 (_mm_srli_si128(tmp_128i, 8));
        // AVX2 version
        // tmp_256i = _mm256_cvtepu16_epi32 (tmp_128i);

        // convert this to 512 bits of double-precision (64-bit) floats in two instructions
        _mm256_store_pd(&dst[i], _mm256_cvtepi32_pd (tmp_256i_p[0]));
        _mm256_store_pd(&dst[i+4], _mm256_cvtepi32_pd (tmp_256i_p[1]));
    }
}

inline void process_simd3(double *dst, const quint32_le &n_items, const QByteArray &items) {
    static __m256i tmp_256i_0;
    static __m128i *tmp_256i_p0 = (__m128i*)&tmp_256i_0;

    static __m256i tmp_256i_1[2];
    static __m128i *tmp_256i_p1 = (__m128i*)&tmp_256i_1[0];

    uint16_t *uint16_data = (uint16_t*)items.constData();

    for(int i = 0; i < n_items; i += 16) {
        // load up 128-bits of 16-bit integers (16 of them) and zero extend to 32-bit
        // change to a memcpy if aligned data is needed...
        tmp_256i_0 = _mm256_loadu_si256((__m256i_u*)&uint16_data[i]);

        // Since we don't have AVX2...
        tmp_256i_p1[0] = _mm_cvtepu16_epi32 (tmp_256i_p0[0]);
        tmp_256i_p1[1] = _mm_cvtepu16_epi32 (_mm_bsrli_si128(tmp_256i_p0[0], 8));
        tmp_256i_p1[2] = _mm_cvtepu16_epi32 (tmp_256i_p0[1]);
        tmp_256i_p1[3] = _mm_cvtepu16_epi32 (_mm_bsrli_si128(tmp_256i_p0[1], 8));
        // AVX2 version
        // tmp_256i = _mm256_cvtepu16_epi32 (tmp_128i);

        // convert this to 1024 bits of double-precision (64-bit) floats in two instructions
        _mm256_store_pd(&dst[i], _mm256_cvtepi32_pd (tmp_256i_p1[0]));
        _mm256_store_pd(&dst[i+4], _mm256_cvtepi32_pd (tmp_256i_p1[1]));
        _mm256_store_pd(&dst[i+8], _mm256_cvtepi32_pd (tmp_256i_p1[2]));
        _mm256_store_pd(&dst[i+12], _mm256_cvtepi32_pd (tmp_256i_p1[3]));
    }
}

inline void double_keys_no_simd(double *dst, int len){
    for(int i = 0; i < len; i++) {
        dst[i] = (double)i;
    }
}

inline void double_keys0(double *dst, int len){
    static __m128i tmp_128i;
    for(int i = 0; i < len; i+=4) {
        tmp_128i = _mm_setr_epi32(i,i+1, i+2, i+3);
        _mm256_store_pd(&dst[i], _mm256_cvtepi32_pd(tmp_128i));
    }
}

inline void double_keys1(double *dst, int len){
    static __m256d tmp_256d;
    for(int i = 0; i < len; i+=4) {
        _mm256_store_pd(&dst[i], _mm256_setr_pd(i,i+1, i+2, i+3));
    }
}





int run(QFile &f, double *dst, int method){
    QElapsedTimer m_timer;
    m_timer.start();
    int t_start, t_end, total_samples = 0;
    quint32_le timestamp;
    while(!f.atEnd()) {
        timestamp = qFromLittleEndian<quint32_le>(f.read(4));
        quint32_le n_items = qFromLittleEndian<quint32_le>(f.read(4));
        QByteArray items = f.read(n_items * 2 + 1); // 2-bytes for 16-bit unsigned int + end byte

        switch(method){
            case NO_SIMD:
                process_no_simd(dst, n_items, items);
                break;
            case SIMD1:
                process_simd1(dst, n_items, items);
                break;
            case SIMD2:
                process_simd2(dst, n_items, items);
                break;
            case SIMD3:
                process_simd3(dst, n_items, items);
                break;
            default:
                throw;
        }

        if(total_samples == 0){
            t_start = timestamp;
        }
        total_samples += n_items;
    }
    t_end = timestamp;
    qDebug() << "Read " << total_samples << " data points";
    qDebug() << "Fs = " << 1e3 * total_samples/(t_end - t_start) << " ksps";
    qDebug() << "File Processing with method " << method << " took: " << m_timer.nsecsElapsed()/(1000) << "microseconds";
    qDebug() << "Sample: " << dst[0] << dst[1] << dst[2] << dst[3];
    return m_timer.nsecsElapsed()/(1000); // microseconds
}


int main(int argc, char *argv[]) {
    QString f_name = "C:/dev/prbs/ui/cmake-build-debug/data/2021-06-16-17-08-39_exp.raw";
    qDebug() << "Reading File" << f_name;
    QFile f(f_name);
    f.open(QIODevice::ReadOnly);

    double *d_data0 = (double*)_mm_malloc(sizeof(double)*512, 256);
    double *d_data1 = (double*)_mm_malloc(sizeof(double)*512, 256);
    double *d_data2 = (double*)_mm_malloc(sizeof(double)*512, 256);
    double *d_data3 = (double*)_mm_malloc(sizeof(double)*512, 256);

    f.seek(0);
    run(f, d_data0, 0);
    f.seek(0);
    run(f, d_data1, 1);
    f.seek(0);
    run(f, d_data2, 2);
    f.seek(0);
    run(f, d_data3, 3);
    f.close();

    _mm_free(d_data0);
    _mm_free(d_data1);
    _mm_free(d_data2);
    _mm_free(d_data3);

    // test double key creation
    double *d_key0 = (double*)_mm_malloc(sizeof(double)*512, 256);
    double *d_key1 = (double*)_mm_malloc(sizeof(double)*512, 256);
    double *d_key_no_simd = (double*)_mm_malloc(sizeof(double)*512, 256);
    QElapsedTimer m_timer;
    int ntrials=10000;


    m_timer.start();
    for(int i = 0; i < ntrials; i++) {
        double_keys0(d_key0, 512);
    }
    qDebug() << "Output double_keys0: " << d_key0[0] << d_key0[1]<< d_key0[2]<< d_key0[3];
    qDebug() << "double_keys0 Took: " << m_timer.nsecsElapsed()/ntrials << "nanoseconds (average of "<< ntrials << "  trials)";

    m_timer.start();
    for(int i = 0; i < ntrials; i++) {
        double_keys1(d_key1, 512);
    }
    qDebug() << "double_keys1 Took: " << m_timer.nsecsElapsed()/ntrials << "nanoseconds (average of "<< ntrials << "  trials)";
    qDebug() << "Output double_keys1: " << d_key1[0] << d_key1[1]<< d_key1[2]<< d_key1[3];

    m_timer.start();
    for(int i = 0; i < ntrials; i++) {
        double_keys_no_simd(d_key_no_simd, 512);
    }
    qDebug() << "d_key_no_simd Took: " << m_timer.nsecsElapsed()/ntrials << "nanoseconds (average of "<< ntrials << "  trials)";
    qDebug() << "Output d_key_no_simd: " << d_key_no_simd[0] << d_key_no_simd[1]<< d_key_no_simd[2]<< d_key_no_simd[3];



}


// alternate: zero pad unsigned 16-bits to 32 bits, then convert 32-bit int to double?
// __m128i _mm_cvtepu16_epi32 (__m128i a) zero-extend unsigned
// __m256i _mm256_cvtepu16_epi32 (__m128i a) zero-extend unsigned
// __m512i _mm512_cvtepu16_epi64 (__m128i a) or this if AVX512

// __m256d _mm256_cvtepi32_pd (__m128i a)
// _m512d _mm512_cvtepi32_pd (__m256i a)

// might be useful for Wlash-Hadamard
//__m256i _mm256_loadu2_m128i (__m128i const* hiaddr, __m128i const* loaddr)

//
//    //QVector<double> data;
//    double *d_data = (double*)_mm_malloc(sizeof(double)*512, 256);
//
//    // try reading some data into QVector and transforming
//    int c = 0;
//    int t_start, t_end, total_samples = 0;
//    quint32_le timestamp;
//    while(!f.atEnd()) {
//        qDebug() << "Reading Chunk" << c++;
//        // the slow way
//        timestamp = qFromLittleEndian<quint32_le>(f.read(4));
//        quint32_le n_items = qFromLittleEndian<quint32_le>(f.read(4));
//        QByteArray items = f.read(n_items * 2+1); // 2-bytes for 16-bit unsigned int + end byte
//        const char *r_data = items.constData();
//        print_bytes(r_data, 8);
//
//        //for (int i = 0; i < 4; i++) {
//        int val1 = qFromLittleEndian<quint16_le>(items.mid(0, 2));
//        int val2 = qFromLittleEndian<quint16_le>(items.mid(2, 2));
//        int val3 = qFromLittleEndian<quint16_le>(items.mid(4, 2));
//        int val4 = qFromLittleEndian<quint16_le>(items.mid(6, 2));
//        qDebug() << "No SIMD:" << val1 << val2 << val3 << val4 ;
//
//        // the fast way
//        __m128 tmp_f;
//        __m64 tmp_16i;
//        __m64 *tmp_16i_ptr;
////        static __m128i tmpi;
//        __m256d tmp_d;
////        double *d_data = data.data();
//        // load 128 bits of 16 bit data
//        //tmpi = _mm_loadu_si128((__m128i const*)items.data());
//        /*tmpi = _mm_set_epi16(
//                *r_data, *(r_data+2), *(r_data + 4), *(r_data+6),
//                *(r_data+8), *(r_data+10), *(r_data + 12), *(r_data+14));*/
//
//        // load 4 16-bit values
//        // try memcpy... memcpy(val, &var, sizeof(val));
//        //__m256i const perm_mask = _mm128_set_epi8(7, 6, 3, 2, 5, 4, 1, 0);
////        __m128i const perm = _mm_set_epi8(14,15,12,13,10,11,8,9,6,7,4,5,2,3,0,1);
////        __m128i tmp_128i;
//
//
////        __m128i msrc, map, mdst;
////        memcpy(&msrc, r_data, sizeof(__m128i));
////        map = _mm_setr_epi8(14,15,12,13,10,11,8,9,6,7,4,5,2,3,0,1);
////        mdst = _mm_shuffle_epi8(msrc, map);
////        // little endian to big endian...
//////        _mm_permutexvar_epi8(tmp_128i, perm);
////        print_128i(mdst);
//
////        tmp_16i = _mm_set_pi16(*r_data, *(r_data+2), *(r_data + 4), *(r_data+6));
//        memcpy(&tmp_16i, r_data, sizeof(__m64) );
//        //tmp_16i = _mm_set_pi16(468, 471, 444, 463);
//        print_m64(tmp_16i);
//
//        // convert to 4 single-precision floating point values
//        tmp_f = _mm_cvtpu16_ps (tmp_16i);
//        print_128(tmp_f);
//        // convert to 4 double-precision floating point values
//        tmp_d = _mm256_cvtps_pd(tmp_f);
//        //print_m256d(tmp_d);
//        // cannot store in the double pointer, memory is not aligned
//        _mm256_store_pd(d_data, tmp_d);
//
////        tmp_d
//
//        qDebug() << "With SIMD:" << *d_data << *(d_data+1) << *(d_data+2) << *(d_data+3) ;
//
//
//
//        // _mm_loadu_si16
//        // __m128 _mm_cvtpu16_ps (__m64 a) Convert packed unsigned 16-bit integers in a to packed single-precision (32-bit) floating-point elements, and store the results in dst.
//        // __m256d _mm256_cvtps_pd (__m128 a)
//
//
//        if(total_samples == 0){
//            t_start = timestamp;
//        }
//        total_samples += n_items;
//    }
//    t_end = timestamp;
//
//    qDebug() << "Read " << total_samples << " data points";
//    qDebug() << "Fs = " << 1e3 * total_samples/(t_end - t_start) << " ksps";
//




//    QElapsedTimer m_timer;
