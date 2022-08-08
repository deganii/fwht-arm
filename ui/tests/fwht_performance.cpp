//
// Created by Ismail Degani on 6/3/2021.
//

#include <QApplication>
#include <QDebug>
#include <QtCore/QElapsedTimer>
#include <fwht/fwht.h>
#define printf(args...) fprintf(stderr, ##args)

void init_values(double *keys, double *values, unsigned *seq, unsigned order){
    int idx[] = {3, 7, 12, 15, 21, 27};
    double *d_tmp = (double *) _mm_malloc(sizeof(double) * (1<<order), 256);
    int n = 1<<order;
    for(int i = 0; i < n; i++){
        seq[i] = 300;
    }
    fwht_sequency_permutation(seq, order);

    for(int i = 0; i < n; i++) {
        keys[i] = i;
        d_tmp[i] = 0;
    }

    for(int i = 0; i < sizeof(idx)/sizeof(idx[0]); i++){
        d_tmp[idx[i]] = 1;
    }
    fwht(d_tmp, n, values, seq, false);
}



int main(int argc, char *argv[]) {
    QElapsedTimer m_timer;
    int order = 5;
    int len = 1 << order;
    int ntrials = 1000;
    double *d_keys = (double *) _mm_malloc(sizeof(double) * len, 256);
    double *d_values = (double *) _mm_malloc(sizeof(double) * len, 256);
    unsigned *d_sequency = (unsigned *) _mm_malloc(sizeof(unsigned) * len, 256);
    double *d_transform = (double *) _mm_malloc(sizeof(double) * len, 256);
    init_values(d_keys, d_values, d_sequency, order);
    printf("values: ");
    for(int i = 0; i < len; i++){
        printf("%f ", d_values[i]);
    }
    printf("\n");


    m_timer.start();
    for(int i = 0; i < ntrials; i++) {
        fwht(d_values, len, d_transform, d_sequency, false);
    }
    qDebug() << "fwht (NO SIMD) Transform Took: " << m_timer.nsecsElapsed()/(ntrials) << "microseconds (average of "<< ntrials << "  trials)";
    printf("transform: ");
    for(int i = 0; i < len; i++){
        printf("%f ", d_transform[i]);
    }
    printf("\n");

    init_values(d_keys, d_values, d_sequency, order);
    m_timer.start();
    for(int i = 0; i < ntrials; i++) {
        fwht_wiki_simd(d_transform, d_values, d_sequency, len);
    }
    init_values(d_keys, d_values, d_sequency, order);
    fwht_wiki_simd(d_transform, d_values, d_sequency, len);
    qDebug() << "fwht_wiki_simd Transform Took: " << m_timer.nsecsElapsed()/(ntrials) << "nanoseconds (average of "<< ntrials << "  trials)";
    printf("transform: ");
    for(int i = 0; i < len; i++){
        printf("%f ", d_transform[i]);
    }
    printf("\n");
    init_values(d_keys, d_values, d_sequency, order);
    m_timer.start();
    for(int i = 0; i < ntrials; i++) {
        fwht_iterative_simd(d_transform, d_values, d_sequency, len);
    }
    init_values(d_keys, d_values, d_sequency, order);
    fwht_iterative_simd(d_transform, d_values, d_sequency, len);
    qDebug() << "fwht_iterative_simd Transform Took: " << m_timer.nsecsElapsed()/(ntrials) << "nanoseconds (average of "<< ntrials << "  trials)";
    printf("transform: ");
    for(int i = 0; i < len; i++){
        printf("%f ", d_transform[i]);
    }
    printf("\n");
    fwht_iterative_simd(d_values, d_transform, d_sequency, len);
    printf("inv transform: ");
    for(int i = 0; i < len; i++){
        printf("%f ", d_values[i] / len);
    }
    printf("\n");


//    fwht_wiki_simd(d_transform, d_values, d_sequency, len);
//    printf("transform: ");
//    for(int i = 0; i < len; i++){
//        printf("%f ", d_transform[i]);
//    }


    //    for(int i = 0; i < values.size(); i++) {
//        tmp[i] = 0;
//    }
//    for(int i = 0; i < indices.size(); i++){
//        tmp[indices[i]] = 1;
//    }

}




//template<typename T> void init_values(
//        QVector<T> &values,
//        QVector<uint> &indices,
//        QVector<uint> &m_sequency){
//
//    QVector<T>tmp(values.size());
//    for(int i = 0; i < values.size(); i++) {
//        tmp[i] = 0;
//    }
//    for(int i = 0; i < indices.size(); i++){
//        tmp[indices[i]] = 1;
//    }
//
//    double *tmp_malloc  = ( double *)_mm_malloc(values.size()*sizeof(double), 256);
//    double *values_malloc  = ( double *)_mm_malloc(values.size()*sizeof(double), 256);
//
////    qDebug() << "Pre-transform: ";
//    for(int i = 0; i < tmp.size(); i++){
//        tmp_malloc[i] = tmp[i];
//        values_malloc[i] = values[i];
////        qDebug() << tmp_malloc[i];
//    }
//
////    fwht(tmp, values.size(), values, m_sequency, false);
//    fwht_iterative_simd(values_malloc, tmp_malloc, (unsigned)values.size());
//
////    qDebug() << "Post-transform: ";
//    for(int i = 0; i < tmp.size(); i++){
////        qDebug() << values_malloc[i];
//        indices[i] = i;
//        values[i] = values_malloc[i];
////    }
//
//    _mm_free(tmp_malloc);
//    _mm_free(values_malloc);
//
//}
//
//int main(int argc, char *argv[]) {
//    qDebug() << "Performance Tests of Fast Walsh-Hadamard Transform Examples";
//    QElapsedTimer m_timer;
//
//    int order = 12; // 4096 point transform
////    int order = 5; // 32 point transform
//    int n = 1<<order;
//    QVector<uint> w_idx = {3, 7, 12, 15, 21, 27};
//    QVector<float> m_keys = QVector<float>(n);
//    QVector<float> m_values = QVector<float>(n);
//    QVector<float> m_transform = QVector<float>(n);
//    QVector<uint> m_sequency = QVector<uint>(n);
//    QVector<double> d_sequency = QVector<double>(n);
//    QVector<double> d_values = QVector<double>(n);
//    QVector<double> d_dest = QVector<double>(n);
//
//    int ntrials = 1000;
////    double d_values[n];
////    double d_dest[n];
//
////    fwht_sequency_permutation(m_sequency, order);
//    fwht_sequency_permutation(d_sequency.data(), (unsigned)order);
//
//    init_values(d_values, w_idx, m_sequency);
//
//    for(int i = 0; i < m_values.size(); i++){
//        d_values[i] = (double)m_values[i];
//    }
//
//    m_timer.start();
//    for(int i = 0; i < ntrials; i++) {
//        fwht(m_values, n, m_transform, m_sequency, false);
//    }
//    qDebug() << "Basic Transform Took: " << m_timer.nsecsElapsed()/(1000*ntrials) << "microseconds (average of "<< ntrials << "  trials)";
//
//
//    double *dest_ptr = d_dest.data();
//    double *values_ptr = d_values.data();
//
//    //double *dv_malloc = ( double *)malloc(sizeof(double)*n*8);
////    double *dd_malloc = ( double *)malloc(sizeof(double)*n*8);
//    double *dv_malloc  = ( double *)_mm_malloc(n*sizeof(double), 256);
//    double *dd_malloc  = ( double *)_mm_malloc(n*sizeof(double), 256);
//
//    m_timer.start();
//    for(int i = 0; i < 1000; i++) {
//        fwht_recursive_simd(dv_malloc, n);
//    }
//    qDebug() << "Recursive SIMD Transform Took: " << m_timer.elapsed() << "microseconds (average of 1000 trials)";
//
//
//    qDebug() << "sizeof(d_values): " << sizeof(d_values.data());
//    m_timer.start();
//    for(int i = 0; i < ntrials; i++) {
//        fwht_iterative_simd(dd_malloc, dv_malloc, n);
//    }
//
//
//    qDebug() << "Iterative SIMD Transform Took: " << m_timer.nsecsElapsed()/(1000*ntrials) << "microseconds (average of "<<ntrials<< " trials)";
//
//
//
////    free(dv_malloc);
////    free(dd_malloc);
//
//    _mm_free(dv_malloc);
//    _mm_free(dd_malloc);
//
//
//
//    //QApplication a(argc, argv);
//    //return a.exec();
//}