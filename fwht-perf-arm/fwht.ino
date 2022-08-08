#include "arm_math.h"
#include "printb.h"
//#include "cmsis.h"

// TODO: Play with compiler optimizer...
// https://forum.pjrc.com/threads/43572-Optimization-Fast-Faster-Fastest-with-without-LTO


// Fast Walsh-Hadamard Transform
// goals - maximize use of SIMD / CMSIS library accelerators
// Ideally an in-place transform with unsigned 16-bit integers
// n must be a power of 2 / otherwise same signature as memcpy

// Question: What is faster and easier to accelerate with SIMD:
// precomputing sequency, simple FWT, re-order  - 1200us simple FWT (no re-order)
// VS.
// Beauchamp/Li/Duo transform and reorder in 2-stage algorithm - 1600us



// Adapted from https://github.com/bvssvni/fwht/blob/master/fwht-test.c
// note: NOT in sequency order
void fwht_nilsen(int16_t *dst, int16_t *src,  int16_t n){
    uint16_t adata[n];
    uint16_t bdata[n];
    uint16_t *a = adata;
    uint16_t *b = bdata;
    uint16_t *tmp;
    memcpy(a, src, sizeof(int16_t)*n);
    
    // Fast Walsh Hadamard Transform.
    uint16_t i, j, s;
    for (i = n>>1; i > 0; i>>=1) {
        for (j = 0; j < n; j++) {
            s = j/i%2;
            b[j]=a[(s?-i:0)+j]+(s?-1:1)*a[(s?0:i)+j];
        }
        tmp = a; a = b; b = tmp;
    }
    
    memcpy(dst, a, sizeof(int16_t)*n);
}


// Adapted from https://en.wikipedia.org/wiki/Fast_Walsh%E2%80%93Hadamard_transform
// note: NOT in sequency order
void fwht_wiki(int16_t *a, int16_t n){
    int16_t x,y,h = 1;
    while(h < n){
        for(int16_t i = 0; i< n; i += h<<1){
            for(int16_t j = i; j< i + h; j++){
                x = a[j];
                y = a[j + h];
                a[j] = x + y;
                a[j + h] = x - y;
            }
        }
        h <<= 1;
    }
}


// Adapted from https://en.wikipedia.org/wiki/Fast_Walsh%E2%80%93Hadamard_transform
// note: NOT in sequency order
void fwht_wiki_simd(int16_t *a, uint16_t n){
    uint16_t h = 1;

    // special case for h == 1: use a sum/difference SIMD
    for(int16_t i = 0; i< n; i += 2){
            //x = a[i];
            //y = a[i + 1];
            uint32_t xy = *((uint32_t*)(&a[i]));
            //res[15:0]  = val1[15:0]  - val2[31:16]   
            //res[31:16] = val1[31:16] + val2[15:0]  
            // a[i] = x + y;
            // a[i + 1] = x - y;
            *((uint32_t*)(&a[i])) = __SSAX(xy, xy); // a[i] = x + y | a[i+1] = y - x
            a[i+1] = -a[i+1];
    }   
    h <<= 1;

    
    Serial.println("Walsh 1st Stage");
    while(h < n){
        for(int16_t i = 0; i< n; i += h<<1){
            for(int16_t j = i; j< i + h; j+=2){
                //x1 = a[j];
                //x2 = a[j+1];
                uint32_t x1x2 = *((uint32_t*)(&a[j]));
                //y1 = a[j + h];
                //y2 = a[j + h + 1];
                uint32_t y1y2 = *((uint32_t*)(&a[j+h]));
                //a[j] = x1 + y1;
                //a[j+1] = x2 + y2;
                *((uint32_t*)(&a[j])) = __SADD16(x1x2, y1y2);
                //a[j + h] = x1 - y1;
                //a[j + h + 1] = x2 - y2;
                *((uint32_t*)(&a[j+h])) = __SSUB16(x1x2, y1y2);
            }
        }
        h <<= 1;
    }
}


inline uint32_t bin2gray(uint32_t n)
{
  return (n >> 1) ^ n;
}

void sequency_order(uint16_t* dst, uint16_t order){
  //uint16_t temp;
  uint16_t n = 1<<order;
  uint16_t shft = 32-order;
  //work in 32bits because of RBIT
  for(uint32_t i = 0; i < n; i++){
    dst[i] = (uint16_t)(__RBIT(bin2gray(i)) >> shft);
    //temp = (uint16_t)((bin2gray(i)));
    //Serial.printf("dst[%d] = %d\n", i,  dst[i]);
    
//    Serial.printf("dst[%d] = %d (", i);
//    printb(dst[i]);
//    Serial.println(")");

    //Serial.printf("dst[%d] = %d\n", i,  dst[i]);
  }
}

// Adapted from https://en.wikipedia.org/wiki/Fast_Walsh%E2%80%93Hadamard_transform
// note: NOT in sequency order
void fwht_wiki_seq_simd(int16_t *dst, int16_t *src, uint16_t *seq,  uint16_t n){
    uint16_t h = 1;

    // special case for h == 1: use a sum/difference SIMD
    for(uint32_t i = 0; i< n; i += 2){
            //x = a[i];
            //y = a[i + 1];
            uint32_t xy = *((uint32_t*)(&src[i]));
            //res[15:0]  = val1[15:0]  - val2[31:16]   
            //res[31:16] = val1[31:16] + val2[15:0]  
            // a[i] = x + y;
            // a[i + 1] = x - y;
            *((uint32_t*)(&src[i])) = __SSAX(xy, xy); // a[i] = x + y | a[i+1] = y - x
            src[i+1] = -src[i+1];
    }   
    h <<= 1;

    
    Serial.println("Walsh 1st Stage");
    while(h < n){
        for(uint32_t i = 0; i< n; i += h<<1){
            for(uint32_t j = i; j< i + h; j+=2){
                //x1 = a[j];
                //x2 = a[j+1];
                uint32_t x1x2 = *((uint32_t*)(&src[j]));
                //y1 = a[j + h];
                //y2 = a[j + h + 1];
                uint32_t y1y2 = *((uint32_t*)(&src[j+h]));
                //a[j] = x1 + y1;
                //a[j+1] = x2 + y2;
                *((uint32_t*)(&src[j])) = __SADD16(x1x2, y1y2);
                //a[j + h] = x1 - y1;
                //a[j + h + 1] = x2 - y2;
                *((uint32_t*)(&src[j+h])) = __SSUB16(x1x2, y1y2);
            }
        }
        h <<= 1;
    }

    for(uint32_t i = 0; i < n; i++){
      dst[i] = src[seq[i]];
    }
}


// Adapted from https://en.wikipedia.org/wiki/Fast_Walsh%E2%80%93Hadamard_transform
// note: NOT in sequency order
void fwht_wiki_simd2(int16_t* a, uint16_t n){
    uint16_t h = 1;
    // special case for h == 1: use a sum/difference SIMD
    for (uint32_t* xy = (uint32_t *)(&a[n-2]); xy >= (uint32_t *)a; xy--) {
            *xy = __SSAX(*xy, *xy);
            ((int16_t *)xy)[1] = -((int16_t *)xy)[1];
    }
    h <<= 1;
    while(h < n){
        for(int16_t i = 0; i< n; i += h<<1){
            for(int16_t j = i; j< i + h; j+=2){
                uint32_t x1x2 = *((uint32_t*)(&a[j]));
                uint32_t y1y2 = *((uint32_t*)(&a[j+h]));
                *((uint32_t*)(&a[j])) = __SADD16(x1x2, y1y2);
                *((uint32_t*)(&a[j+h])) = __SSUB16(x1x2, y1y2);
            }
        }
        h <<= 1;
    }
}

// Adapted from https://en.wikipedia.org/wiki/Fast_Walsh%E2%80%93Hadamard_transform
// note: in sequency order
void fwht_wiki_seq_simd2(int16_t *dst, int16_t *src, uint16_t *seq,  uint16_t n){
    uint16_t h = 1;
    // special case for h == 1: use a sum/difference SIMD
    for (uint32_t* xy = (uint32_t *)(&src[n-2]); xy >= (uint32_t *)src; xy--) {
            *xy = __SSAX(*xy, *xy);
            ((int16_t *)xy)[1] = -((int16_t *)xy)[1];
    }
    h <<= 1;
    while(h < n){
        for(int16_t i = 0; i< n; i += h<<1){
            for(int16_t j = i; j< i + h; j+=2){
                uint32_t x1x2 = *((uint32_t*)(&src[j]));
                uint32_t y1y2 = *((uint32_t*)(&src[j+h]));
                *((uint32_t*)(&src[j])) = __SADD16(x1x2, y1y2);
                *((uint32_t*)(&src[j+h])) = __SSUB16(x1x2, y1y2);
            }
        }
        h <<= 1;
    }

    for(uint32_t i = 0; i < n; i++){
      dst[i] = src[seq[i]];
    }
}



int16_t* fwht_beauchamp_li(int16_t *y, int16_t *x, uint16_t N){
    // Fast Walsh-Hadamard Transform
    // Based on mex function written by Chengbo Li@Rice Uni for his TVAL3 algorithm.
    // His code is according to the K.G. Beauchamp's book -- Applications of Walsh and Related Functions.
    uint16_t S = N/2; // stage
    uint16_t G = N/2; // Number of Groups (N/2)
    uint16_t M = 2; // Number of Members in Each Group
    int16_t *temp;
    //First stage
    //y = np.zeros((G,M)) // instead of reshaping/copying, let's keep track of indices....
//    y[:,0] = x[0::2] + x[1::2]
//    y[:,1] = x[0::2] - x[1::2]

    for(uint16_t i = 0; i < N; i+=2){
        y[i] = x[i] + x[i+1];
        y[i+1] = x[i] - x[i+1];
    }
    S >>= 1;
    
    // swap the pointers...
    temp = x;
    x = y;
    y = temp;
    
    //Second and further stage
    //for nStage in xrange(2,int(log(N,2))+1):
    while(S > 0) {
        for(int i = 0; i < G/2; i++){
          for(int j = 0; j < M*2; j+=4){
            // UNROLL y[0:G/2,2:M*2:4] = x[0:G:2,1:M:2] - x[1:G:2,1:M:2]
            // y[0,2]  = x[0,1] - x[1,1]
            // y[0,6]  = x[0,3] - x[1,3]
            // y[0,10] = x[0,5] - x[1,5]
            // y[0,0]  = x[0,1] - x[1,1]
            // y[1,0]  = x[2,1] - x[3,1]
            // y[2,0]  = x[4,1] - x[5,1]
            y[i*(2*M) + j]     = x[i*(M)*2 + j/2]   + x[(i*2+1)*M + j/2];
            y[i*(2*M) + j + 1] = x[i*(M)*2 + j/2]   - x[(i*2+1)*M + j/2];            
            y[i*(2*M) + j+2]   = x[i*(M)*2 + j/2 + 1]   - x[(i*2+1)*M + j/2 + 1];
            y[i*(2*M) + j+3]   = x[i*(M)*2 + j/2 + 1]   + x[(i*2+1)*M + j/2 + 1];
          }
        }
//
//        if(S == 4) { // nStage == 3
//          Serial.println("y2");
//          for(uint16_t i = 0; i < N; i++) {
//            Serial.printf("y[%d] = [%d]\n", i, y[i]);
//          }
//        }

        // swap the pointers...
        temp = x;
        x = y;
        y = temp;
        G = G/2;
        M = M*2;
        S >>= 1;
//        Serial.printf("G = %d, M = %d\n", G,M);
    }
    return x;
}

//#define SWAP(x, y) temp=x;x=y;y=temp

inline void swap16(int16_t **a, int16_t **b){
    int16_t *temp = *a;
    *a=*b;
    *b=temp;
}


int16_t* fwht_beauchamp_simd(int16_t *y, int16_t *x, uint16_t N){
    // Fast Walsh-Hadamard Transform
    // Based on mex function written by Chengbo Li@Rice Uni for his TVAL3 algorithm.
    // His code is according to the K.G. Beauchamp's book -- Applications of Walsh and Related Functions.
    uint16_t S = N/2; // stage
    uint16_t G = N/2; // Number of Groups (N/2)
    uint16_t M = 2; // Number of Members in Each Group
    int16_t *temp;
    
    //First stage
    for(uint16_t i = 0; i < N; i+=2){
        uint32_t x1x2 = *((uint32_t*)(&x[i]));
        *((uint32_t*)(&y[i])) = __SSAX(x1x2, x1x2);
        y[i+1] = -y[i+1];
    }
    S >>= 1;
    
    // swap the pointers...
//    swap16(x,y); //SWAP(x,y);
    temp = x;
    x = y;
    y = temp;
    
    //Second and further stages
    while(S > 0) {
        for(int i = 0; i < G/2; i++){
          for(int j = 0; j < M*2; j+=4){
            int16_t *xl = &x[i*(M)*2 + j/2];
            int16_t *xr = &x[(i*2+1)*M + j/2];
            int16_t *yp = &y[i*(2*M) + j];
            yp[0] = xl[0] + xr[0];
            yp[1] = xl[0] - xr[0];
            yp[2] = xl[1] - xr[1];
            yp[3] = xl[1] + xr[1];
          }
        }
        // swap the pointers...
//        swap16(x,y);
        temp = x;
        x = y;
        y = temp;
        G = G/2;
        M = M*2;
        S >>= 1;
    }
    return x;
}

    // Fast Walsh-Hadamard Transform
    // Based on mex function written by Chengbo Li@Rice Uni for his TVAL3 algorithm.
    // His code is according to the K.G. Beauchamp's book -- Applications of Walsh and Related Functions.
int16_t* fwht_beauchamp_simd2(int16_t *y, int16_t *x, uint16_t N){
    uint16_t S = N/2; // stage
    uint16_t G = N/2; // Number of Groups (N/2)
    uint16_t M = 2; // Number of Members in Each Group
    //int16_t *temp;
    uint16_t iM2 = 0;
    //First stage
    for(uint16_t i = 0; i < N; i+=2){
        uint32_t x1x2 = *((uint32_t*)(&x[i]));
        *((uint32_t*)(&y[i])) = __SSAX(x1x2, x1x2);
        y[i+1] = -y[i+1];
    }
    S >>= 1;
    swap16(&x,&y);
    
    //Second and further stages
    while(S > 0) {
        for(int i = 0; i < G/2; i++){
          iM2 = i*M*2;
          for(int j = 0; j < M*2; j+=4){
            uint32_t xl = *(uint32_t *)(&x[iM2 + j/2]);
            uint32_t xr = *(uint32_t *)(&x[iM2+ M + j/2]);
            uint16_t *yp = (uint16_t *)(&y[iM2 + j]);
            uint32_t *yp_32_1 = (uint32_t *)(&y[iM2 + j + 1]);
            *yp_32_1 = __SSUB16(xl, xr);
            uint32_t add16 = __SADD16(xl, xr);
//            yp[0] = add16 >> 16; // ((int16_t*)(&add16))[0];
//            yp[3] = add16 & 0xffff; //((int16_t*)(&add16))[1];
            yp[0] = ((int16_t*)(&add16))[0];
            yp[3] = ((int16_t*)(&add16))[1];            
          }
        }
        swap16(&x,&y);
        G = G/2;
        M = M*2;
        S >>= 1;
    }
    return x;
}

void init_walsh_input(int16_t *walsh, uint16_t *w_idx, uint16_t n, uint16_t s){
  memset(walsh, 0, n*sizeof(int16_t));
//  Serial.printf("Size of w_idx: %d\n", sizeof(w_idx)/sizeof(uint16_t));
  for(uint16_t i = 0; i < s; i++) {
//    Serial.printf("Setting walsh[%d] to %d\n", w_idx[i], 1);
    walsh[w_idx[i]] = 1;
  }
}


void setup() {
  while (!Serial && (millis () <= 5000));
  unsigned long start_time;
  uint16_t w_idx[]= {3,  7, 12, 15, 21, 27};
  uint16_t w_idx_n = sizeof(w_idx)/sizeof(uint16_t);
  
  // order is power of two, i.e. 5 == 32 / 12 == 4096 / 14 == 16384
  uint16_t order = 12;
  uint16_t n = 1 << order; // 
  int16_t walsh[n] __attribute__((aligned(32)));
  int16_t out[n] __attribute__((aligned(32))) ;
  uint16_t sequency[n] __attribute__((aligned(32))) ;
  
//  memset(walsh, 0, n*sizeof(int16_t));
  memset(out, 0, n*sizeof(int16_t));
  
//  for(uint16_t i = 0; i < sizeof(w_idx)/sizeof(uint16_t); i++) {
//    walsh[w_idx[i]] = 1;
//  }

  init_walsh_input(walsh, w_idx, n, w_idx_n);

//  Serial.println("Walsh Input");
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = [%d]\n", i, walsh[i]);
//  }
//
//  // run first method:
//  init_walsh_input(walsh, w_idx, n, w_idx_n);
//  start_time = micros(); 
//  fwht_nilsen(out, walsh, n);
//  Serial.printf("fwht_nilsen with size %d: %d us\n", n, micros()-start_time);

//  fwht_nilsen(out, walsh, n);
//  Serial.println("Walsh Output");
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, out[i]);
//  }
//  

  // run 2nd / wikipedia in-place method
//  init_walsh_input(walsh, w_idx, n, w_idx_n);
//  start_time = micros(); 
//  fwht_wiki(walsh, n);
//  Serial.printf("fwht_wiki with size %d: %d us\n", n, micros()-start_time);
//  
  //fwht_wiki(walsh, n);
//  Serial.println("Walsh Output");
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, walsh[i]);
//  }

//  // run 2nd / wikipedia in-place method
//  init_walsh_input(walsh, w_idx, n, w_idx_n);
//  start_time = micros(); 
//  fwht_wiki_simd2(walsh, n);
//  Serial.printf("fwht_wiki_simd2 with size %d: %d us\n", n, micros()-start_time);


//  Serial.println("Walsh Output");
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, walsh[i]);
//  }

//
//  init_walsh_input(walsh, w_idx, n);
//  start_time = micros(); 
//  fwht_beauchamp_li(out, walsh, n);
//  Serial.printf("fwht_beauchamp_li with size %d: %d us\n", n, micros()-start_time);
//
//  init_walsh_input(walsh, w_idx, n);
//  start_time = micros(); 
//  fwht_beauchamp_simd(out, walsh, n);
//  Serial.printf("fwht_beauchamp_simd with size %d: %d us\n", n, micros()-start_time);
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, out[i]);
//  }

  // run beauchamp sequency ordered method
  init_walsh_input(walsh, w_idx, n, w_idx_n);

//  Serial.println("Walsh Input");
//  for(uint16_t i = 0; i < sizeof(walsh)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, walsh[i]);
//  }
//
  start_time = micros(); 
//  int16_t *ans = fwht_beauchamp_simd2(out, walsh, n);
//  Serial.printf("fwht_beauchamp_simd2 with size %d: %d us\n", n, micros()-start_time);
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, ans[i]);
//  }

  // Computing the sequency only needs to be done once and can be re-used
  // for all discrete-time transforms. Do not include in performance characterization
  //Serial.println("Sequency Order:");
  sequency_order(sequency, order);
  init_walsh_input(walsh, w_idx, n, w_idx_n);

  start_time = micros(); 
  fwht_wiki_seq_simd2(out, walsh, sequency, n);
  Serial.printf("fwht_wiki_seq_simd2 with size %d: %d us\n", n, micros()-start_time);
//  for(uint16_t i = 0; i < sizeof(out)/sizeof(uint16_t); i++) {
//    Serial.printf("Walsh[%d] = %d\n", i, out[i]);
//  }

  
  // TODO: try 8-bit conversions x2 speedup due to SIMD
  // Compare with FFT

}

void loop() {
  // put your main code here, to run repeatedly:

}
