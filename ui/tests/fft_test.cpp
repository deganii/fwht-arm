//
// Created by Ismail Degani on 7/1/2021.
//

#include "fftw/fftw3.h"
typedef double fftw_real;

int main(int argc, char *argv[]) {
    unsigned m_length = 512;
    fftw_real *m_fft_in =  (fftw_real *) fftw_malloc(m_length * sizeof(fftw_real));
    fftw_complex *m_fft_out =  (fftw_complex *) fftw_malloc((m_length/2+1) * sizeof(fftw_complex));
    fftw_real *m_fft_pwr_spectrum =  (fftw_real *) fftw_malloc((m_length/2+1) * sizeof(fftw_real));


}