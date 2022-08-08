#include "SerialThread.h"
#include "fwht/fwht.h"
#include <QSerialPort>
#include <QTime>
#include <QDebug>
#include <QtCore/QDir>
#include <limits.h>
#include <QtCore/QRandomGenerator>
//#include <emmintrin.h>
#include <numeric>

#define printf(args...) fprintf(stderr, ##args)

SerialThread::SerialThread(QObject *parent) :
    QThread(parent)
{
    qDebug() << "Initializing Serial Thread...";

    pattern = QByteArray((const char *)&magic_number , sizeof(magic_number ));
    matcher = QByteArrayMatcher(pattern);
}

SerialThread::~SerialThread()
{
    m_quit = true;
    wait();
}

void
SerialThread::begin(const QString &portName, int baudRate, int waitTimeout, QSettings &settings)
{
    if (!isRunning()) {
        m_portName = portName;
        m_waitTimeout = waitTimeout;
        m_baudRate = baudRate;
        m_saveFormat = settings.value("SaveFormat").toString();
        bWriteToFile = settings.value("Save").toBool();
        applyWaveformSettings(settings);
        start();
    }
}

void SerialThread::requestData(){
    // m_mutex.lock(); skip mutex for intrinsic (atomic) type
    m_dataReqested = true;
    // m_mutex.unlock();
}




bool SerialThread::processMultiPartChunk(const uint16_t *uint16_data) {
//    QByteArray raw = ptr.join();
//
    int n_items = m_header.tx_buffer_size;
//
//    if(n_items*2 != raw.size() || n_items < 8 || n_items > 16384) {
//        qCritical() << "Unexpected bufsize: " << n_items << " at timestamp ";
//        m_quit = true;
//    }
//
//    // shuffle the pattern based  onthe desired offset
//    if(m_offset > 0 && m_offset < n_items) {
//        raw.append(raw.left(m_offset*2));
//        raw.remove(0, m_offset*2);
//    }
//
//    // should be raw 16-bit integer sequence (power of 2)
//    uint16_t *uint16_data = (uint16_t*)raw.constData();

    static __m128i tmp_128i;
    static __m256i tmp_256i;
    static __m128i *tmp_256i_p = (__m128i*)&tmp_256i;

    // the maximum adc value...
    const uint16_t ADC_MAX = 4096;
    static __m128i tmp_adc_max = _mm_set1_epi16(ADC_MAX);
    static __m128i tmp_zero = _mm_setzero_si128();
    static __m128i tmp_gt;

    if(m_tx_buffer_len != n_items){
        if(m_tx_buffer_len > 0) {
            qDebug() << "Freeing WHT Transform Buffers: " << m_tx_buffer_len;
            _mm_free(m_key_buffer);
            _mm_free(m_val_buffer);
            _mm_free(m_tx_buffer);
            _mm_free(m_sequency);
            fftw_destroy_plan(p_fwd);
            fftw_free(m_fft_out_buffer);
            fftw_free(m_fft_pwr_spectrum);
        }
        qDebug() << "Allocating WHT Transform Buffers" << n_items;
        m_key_buffer = ( double *)_mm_malloc(n_items * sizeof(double), 256);
        m_val_buffer = ( double *)_mm_malloc(n_items * sizeof(double), 256);
        m_tx_buffer = ( double *)_mm_malloc(n_items * sizeof(double), 256);
        m_sequency = ( unsigned *)_mm_malloc(n_items * sizeof(unsigned), 256);
        m_fft_out_buffer = (fftw_complex *)fftw_alloc_complex(n_items);
        m_fft_pwr_spectrum =  (fftw_real *) fftw_alloc_real(n_items+2);

        m_tx_buffer_len = n_items;
        fwht_sequency_permutation(m_sequency, ilog2(m_tx_buffer_len)-1);

        p_fwd = fftw_plan_dft_r2c_1d(n_items, m_val_buffer, m_fft_out_buffer, FFTW_ESTIMATE);

        // keys never change once len is set...
        // AVX Gives 4x Speedup, even after gcc -O3 (see data_read_performance.cpp)
        for(int i = 0; i < n_items; i+=4) {
            _mm256_store_pd(&m_key_buffer[i], _mm256_setr_pd(i,i+1, i+2, i+3));
        }
    }


    // AVX Gives 10x Speedup (see data_read_performance.cpp)
    for(int i = 0; i < n_items; i += 8) {
        // load up 128-bits of 16-bit integers (8 of them) and zero extend to 32-bit
        // change to a memcpy if aligned data is needed...
        tmp_128i = _mm_loadu_si128((__m128i_u *) &uint16_data[i]);

        // SSE sanity check: make sure all the inputs are less than the ADC Max (4096)

        // if no elements are greater than ADC_MAX, this should return zero
//        tmp_gt = _mm_cmpgt_epi16 (tmp_128i, tmp_adc_max);
//        // if any element is greater than ADC_MAX, this should return 0
//        if(!_mm_testz_si128(tmp_gt, tmp_gt)){
//            return false;
//        }

        // non-SSE sanity check: make sure all the inputs are less than the ADC Max (4096)
        for(int  p = 0; p < 8; p++){
            if((&uint16_data[i])[p] > ADC_MAX){
                // we have some corrupted input, abort...
//                qDebug() << "Bad ADC Input: " << uint16_data[i] << " ABORT FRAME";
                return false;
            }
        }

        // Since we don't have AVX2...
        tmp_256i_p[0] = _mm_cvtepu16_epi32(tmp_128i);
        tmp_256i_p[1] = _mm_cvtepu16_epi32(_mm_srli_si128(tmp_128i, 8));
        // AVX2 version -- switch to this on a newer/better processor
//            tmp_256i = _mm256_cvtepu16_epi32 (tmp_128i);

        // convert this to 512 bits of double-precision (64-bit) floats in two instructions
        _mm256_store_pd(&m_val_buffer[i], _mm256_cvtepi32_pd(tmp_256i_p[0]));
        _mm256_store_pd(&m_val_buffer[i + 4], _mm256_cvtepi32_pd(tmp_256i_p[1]));
    }

//    // second-SSE sanity check: make sure all the inputs are less than the ADC Max (4096)
//    for(int  p = 0; p < 8; p++){
//        if(m_val_buffer[i] > ADC_MAX){
//            // we have some corrupted input, abort...
////                qDebug() << "Bad ADC Input: " << uint16_data[i] << " ABORT FRAME";
//            return false;
//        }
//    }


    // perform SIMD WHT
//        qDebug() << "Attempting FWHT of size " << m_tx_buffer_len;
    fwht_iterative_simd(m_tx_buffer, m_val_buffer, m_sequency, m_tx_buffer_len);

    // perform FFT
    fftw_execute(p_fwd);
    // compute power spectral density
    for(int i =0;i < n_items/2; i++){
        // since we are explicitly putting all power into the sine component, we don't need to compute this
        //m_fft_pwr_spectrum[i] = m_fft_out_buffer[i][0] * m_fft_out_buffer[i][0] +
        //        m_fft_out_buffer[i][1] * m_fft_out_buffer[i][1];

        // note: can replace this with a simple memcpy
        m_fft_pwr_spectrum[2*i] = m_fft_out_buffer[i][0];
        m_fft_pwr_spectrum[2*i+1] = m_fft_out_buffer[i][1];
    }

    // normalize the DC component, because of the dc offset
//    m_fft_pwr_spectrum[0] -= 200*n_items;
//    m_tx_buffer[0] -= 100*n_items;

    // compute the
    double signal = 0;
    if(basis == Ui::BasisFunctions::Walsh){
        for(int i = 0; i < m_current_functions.size(); i++) {
            signal +=m_tx_buffer[m_current_functions[i]];
        }
    } else if (basis == Ui::BasisFunctions::Fourier){
        for(int i = 0; i < m_current_functions.size(); i++) {
            signal +=m_fft_pwr_spectrum[m_current_functions[i]];
        }
    }

    auto hist_mean = std::accumulate(signal_history.begin(), signal_history.end(), .0) / signal_history.size();
    auto hist_stdev = sqrt(std::inner_product(signal_history.begin(), signal_history.end(), signal_history.begin(), 0.0,
       [](double const & x, double const & y) { return x + y; },
       [hist_mean](double const & x, double const & y) { return (x - hist_mean) * (y - hist_mean); }));


    bool ret = true;
//    std::adjacent_difference (signal_history.begin(), signal_history.end(),
//          signal_history_diff.begin(), [](double const & x0, double const & x1) { return abs(x1-x0); });
//    auto min_abs_diff = *std::min_element(signal_history_diff.begin()+1, signal_history_diff.end());
//    double last_signal =signal_history[signal_history_idx];



    signal_history_idx = (++signal_history_idx)%signal_history.size();
    signal_history[signal_history_idx] = signal;
    m_signal_magnitude = signal;
    // run a sanity check
    // if the value is more than 3 std devs away from the history...
      if(abs(signal - hist_mean) > 5 * hist_stdev) { // 2x 25% interquartile range
//    if(abs(signal - hist_mean) > 2.698 * hist_stdev) { // 25% interquartile range
//    if(abs(signal - last_signal) > 5 * min_abs_diff) {
          QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
          qDebug() << dateTime << "Abrupt shift in magnitude: " << signal << ", hist_mean/std: " << hist_mean << "/"
                   << hist_stdev;
          // discard this frame, probably a glitch switching waveforms or an external perturbation...
//
//        qDebug() << "Offending input:";
//        for(int i = 0; i < 8; i++){
//            printf("%f ", m_val_buffer[i]);
//        }
//          for(int i = 128; i < 128+8; i++){
//              printf("%f ", m_val_buffer[i]);
//          }
//          for(int i = 496; i < 496+8; i++){
//              printf("%f ", m_val_buffer[i]);
//          }

//
//        qDebug() << "Offending transform:";
//        for(int i = 0; i < 8; i++){
//            printf("%f ", m_tx_buffer[i]);
//        }
//          for(int i = 128; i < 128+8; i++){
//              printf("%f ", m_tx_buffer[i]);
//          }
//          for(int i = 496; i < 496+8; i++){
//              printf("%f ", m_tx_buffer[i]);
//          }
          // replace the date

          QString filepath = R"(C:\dev\prbs\data\)" + dateTime + "-error.txt";
          QString filedir = QFileInfo(filepath).absolutePath();
          if (QDir::root().mkpath(filedir)) {
              m_file = new QFile(filepath);
              if (!(m_file->open(QIODevice::WriteOnly))) {
                  bWriteToFile = false;
                  qDebug() << "Could not open: " << m_file->fileName() << "for writing.";
                  delete m_file;
              }
              // write the data
              m_file->write("m_tx_buffer = [");
              for (int i = 0; i < n_items; i++) {
                  m_file->write(QString::number(m_tx_buffer[i], 'f', 1).toUtf8().constData());
                  m_file->write(", ");
              }
              m_file->write("]\nm_val_buffer = [");
              for (int i = 0; i < n_items; i++) {
                  m_file->write(QString::number(m_val_buffer[i], 'f', 1).toUtf8().constData());
                  m_file->write(", ");
              }
              m_file->write("]\n");
              m_file->close();
              delete m_file;

          } else {
              qDebug() << "Error creating directory for: " << filepath;
          }


          printf("\n");

          ret = false;
      }
//    } else if( hist_stdev > 2000) {
//        qDebug() << "Abnormal stdev for: " <<  signal << ", hist_mean/std: " << hist_mean << "/" << hist_stdev;
//        // discard this frame, probably a glitch switching waveforms or an external perturbation...
//
//        ret = false;
//    }

      /*else if(signal < -1000 || signal > 10000){
        qDebug() << "Negative signal uncaught: " <<  signal <<
        ", hist_mean/std: " << hist_mean << "/" << hist_stdev;
    }*/

    return ret;
}

void SerialThread::run()
{
    QSerialPort serial;
    serial.setPortName(m_portName);
    serial.setBaudRate(m_baudRate);
    int processed_bytes;

    if(bWriteToFile) {
        // replace the date
        QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
        QString filepath = m_saveFormat.replace("{date}",dateTime);
        QString filedir = QFileInfo(filepath).absolutePath();
        qDebug() << "File Template: " << m_saveFormat;
        qDebug() << "Opening: " << filepath << "in" << filedir << " for writing.";
        if (QDir::root().mkpath(filedir)) {
            m_file = new QFile(filepath);
            if (!(m_file->open(QIODevice::WriteOnly))) {
                bWriteToFile = false;
                qDebug() << "Could not open: " << m_file->fileName() << "for writing.";
                delete m_file;
            }
        } else {
            bWriteToFile = false;
            qDebug() << "Error creating directory for: " << filepath;
        }
    }

    if (!serial.open(QIODevice::ReadWrite)) {
        emit error(tr("Can't open %1, error code %2")
                           .arg(m_portName).arg(serial.error()));
        return;
    }

    serial.clear();

    // set command mode to binary (fast)
    serial.write(CMD_BIN_MODE);
    qDebug() << "Requesting Binary Transfer Mode";

    // read the response
    if (serial.waitForBytesWritten(m_waitTimeout) &&
        serial.waitForReadyRead(m_waitTimeout)) {
        QString responseData = QString::fromUtf8(serial.readAll());
        qDebug() << "Response from MCU: " << responseData;
        emit this->status(responseData);
    }

    // write the desired waveform settings...
    generateUpdatedWaveform();

    if(mode == Ui::WaveformMode::Chirp) {
        m_chirp_hop.start();
    } else if(mode == Ui::WaveformMode::Adaptive){
        m_noise_recalc.start();
    }

    // start the data transfer
    serial.write(CMD_START);
    QByteArray buf;
    processed_bytes = 0;
    int recover_index;
    int failed_loops = 0;
    bool chunk_processed;
    //SerialHeader temp_header;

    while (!m_quit) {
        if(failed_loops > 10){
            qDebug() << "Failed too many times, exiting...";
            break;
        }

        // read response
        if (serial.waitForReadyRead(m_waitTimeout)) {
            buf.append(serial.readAll());
            const unsigned HEADER_SIZE = sizeof(SerialHeader);

            // buf should always have the first chunk of a multipart chunk at the front
            // processed_bytes is always a multiple of multipartChunkSizeBytes...
            if(buf.size() > HEADER_SIZE) {
                memcpy(&m_header, buf.constData() + processed_bytes, HEADER_SIZE);
                if(!validHeader(m_header)){
                    if(buf.size() < 2048){
                        // wait until we get at least 2k-3k worth of data
                        // and try to reset the alignment...
                        failed_loops ++;
                        continue;
                    }

//                    qDebug() << "Invalid Header, attempting to recover ";
//                    printHeader(m_header);

                    // scan forward until we find our magic number
                    recover_index =  matcher.indexIn(buf, 0);
                    if(recover_index > 0) {
//                        qDebug() << "Found a recover point. Discarding " << recover_index << "bytes";
                        buf.remove(0,recover_index);
                        processed_bytes = 0;
                        // loop around to re-read the header correctly....
                        continue;
                    } else {
                        failed_loops ++;
                        continue;
                    }

                    //m_quit = true;
                    //break;
                } else if(m_header.buffer_chunk_idx != 0){
                    // somehow we're off by a chunk, most likely the MCU failed to send a
                    qDebug() << "Misaligned Chunk, attempting recovery.";
                    printHeader(m_header);

                    // this has never happened, so leave unimplemented for now....

                }
            } else{
                // wait for more data
                continue;
            }

            // success, we have a valid chunk....
            failed_loops = 0;


            int chunkSizeBytes = HEADER_SIZE + sizeof(uint16_t)*m_header.item_count + 1;
            int multipartChunkSizeBytes = chunkSizeBytes*m_header.buffer_num_chunks;

            while(buf.size() - processed_bytes > 2*multipartChunkSizeBytes){
              // skip any stale multi-chunk sequences
              processed_bytes += multipartChunkSizeBytes;
            }

            if(m_dataReqested && buf.size() - processed_bytes > multipartChunkSizeBytes ){
                // process the last multi-chunk payload
                int payloadChunkSizeBytes =  m_header.item_count*sizeof(uint16_t);
                int payloadLen =  m_header.item_count * m_header.buffer_num_chunks;
                int payLoadSizeBytes = sizeof(uint16_t) * payloadLen;

                if(m_multipart_payload_len != payloadLen) {
                    if (m_multipart_payload_len > 0) {
                        qDebug() << "Freeing Old Payload Buffer: " << m_multipart_payload_len;
                        _mm_free(m_multipart_payload);
                    }
                    qDebug() << "Allocating Payload Buffers" << payloadLen;
                    m_multipart_payload = (char *) _mm_malloc(payLoadSizeBytes, 256);
                    m_multipart_payload_len = payloadLen;
                }

                const char *r_data = buf.constData() + processed_bytes;
                // put the chunks into one contiguous buffer...
                // todo: just pass the Qbuf and automatically skip
                //  bytes during the conversion to double*
                for (int i = 0; i < m_header.buffer_num_chunks; i++){
                    memcpy(m_multipart_payload + i*payloadChunkSizeBytes,
                           r_data + i*chunkSizeBytes + HEADER_SIZE, payloadChunkSizeBytes);
                    // check the stop byte

                }
                chunk_processed = processMultiPartChunk((uint16_t*)m_multipart_payload);

                if(chunk_processed) {
                    processed_bytes += multipartChunkSizeBytes;

                    if(m_datacollection == nullptr){
                        m_datacollection = (DataCollection*)malloc(sizeof(DataCollection));
//                        m_datacollection->currentFunctions = new QVector<int>();
                    }

                    m_datacollection->timestamp = m_header.timestamp;
                    m_datacollection->m_keys = m_key_buffer;
                    m_datacollection->m_values = m_val_buffer;
                    m_datacollection->transformWHT = m_tx_buffer;
                    m_datacollection->transformFFT = m_fft_pwr_spectrum;
                    m_datacollection->len = m_tx_buffer_len;
                    m_datacollection->signal_energy = m_signal_magnitude;
                    m_datacollection->basis = basis;
                    m_datacollection->num_functions = m_current_functions.size();
                    for(int i = 0; i < m_current_functions.size() && i < MAX_FUNCTIONS; i++){
                        m_datacollection->functions[i] = m_current_functions[i];
                    }
                    emit data(m_datacollection);

                    m_dataReqested = false;
                } else {
                    buf.remove(0, multipartChunkSizeBytes);
                }
            }

            // write the data for the current chunk set and clear the buffer
            if(processed_bytes > 0) {
                //qDebug() << " writing : " <<  processed_bytes << " to disk";
                if (bWriteToFile) {
                    m_file->write(buf.left(processed_bytes));
                }
                buf.remove(0, processed_bytes);
                processed_bytes = 0;
            }

            if(m_newsettings){
                generateUpdatedWaveform();
            } else {
                switch(mode){
                    case Ui::WaveformMode::Custom:
                        // nothing to be done once the waveform is set
                        break;
                    case Ui::WaveformMode::Chirp:
                        // if we are in chirp mode, check whether its time to hop
                        if(m_chirp_hop.elapsed() > m_chirp_hop_ms){
                            m_current_functions[0] = (m_current_functions[0]+1) % rangeEnd;
                            if(m_current_functions[0] == 0){
                                m_current_functions[0] = 1;
                            }
                            generateUpdatedWaveform();
                        }
                        break;
                    case Ui::WaveformMode::Adaptive:
                        if(m_noise_recalc.elapsed() > m_chirp_hop_ms){
                            // todo: find lowest noise components...
                            generateUpdatedWaveform();
                        }
                        break;
                    default:
                        qDebug() << "Unknown Mode: " << mode;
                        mode = Ui::WaveformMode::Custom;
                }
            }

            if(m_waveform_change){
//                qDebug() << "waveform change:";
//                qDebug() << "m_waveform_buffer_len" << m_waveform_buffer_len;
//                qDebug() << "m_multipart_payload_len" << m_multipart_payload_len;
                if(m_waveform_buffer_len == m_multipart_payload_len){
                    // because the buffers are the same length, we could
                    // attempt a "fast" change by directly writing to the
                    // existing DMA buffer and not interrupting the DMA or PDB clock
//                    qDebug() << "Requesting fast waveform change...";
                    serial.write(CMD_FAST_WAVEFORM_CHANGE);
                    if(basis == Ui::BasisFunctions::Fourier){
                        serial.write((char *) m_waveform_fft_cdata, sizeof(uint16_t) * m_waveform_buffer_len);
                    } else if(basis == Ui::BasisFunctions::Walsh){
                        serial.write((char *) m_waveform_cdata, sizeof(uint16_t) * m_waveform_buffer_len);
                    }
                }
                else {
                    qDebug() << "Requesting stop from MCU for Slow Waveform Change";
                    serial.write(CMD_STOP);
                    serial.waitForBytesWritten(50);
                    msleep(100);

                    buf.clear();
                    processed_bytes = 0;

                    qDebug() << "Clearing All Pending Serial Data";
                    serial.clear();

                    serial.write(CMD_WAVEFORM_CHANGE);
                    serial.waitForBytesWritten(5);

                    // send along the new waveform... length, buffer, stop byte
                    serial.write((char *) &m_waveform_buffer_len, sizeof(m_waveform_buffer_len));
                    serial.waitForBytesWritten(5);

                    if(basis == Ui::BasisFunctions::Fourier){
                        serial.write((char *) m_waveform_fft_cdata, sizeof(uint16_t) * m_waveform_buffer_len);
                    } else if(basis == Ui::BasisFunctions::Walsh){
                        serial.write((char *) m_waveform_cdata, sizeof(uint16_t) * m_waveform_buffer_len);
                    }

//                if(serial.waitForReadyRead(m_waitTimeout)){
//                    serial.readAll();
//                }
                    serial.waitForBytesWritten(100);
                    serial.write(CMD_END_BYTE);
                    msleep(100);

                    if (serial.waitForBytesWritten(5) &&
                        serial.waitForReadyRead(m_waitTimeout)) {
                        QString responseData = QString::fromLocal8Bit(serial.readAll());
                        qDebug() << "Response from MCU: " << responseData;
                    } else {
                        qDebug() << "No Response from MCU after waveform change?";
                    }

                    // restart the capture
                    serial.write(CMD_START);
                }
                //emit waveformChange();
                m_waveform_change = false;
            }

        } else {
            emit timeout(tr("Wait read response timeout %1")
                                 .arg(QTime::currentTime().toString()));
            m_quit=true;
        }
    }

    emit disconnecting();

    qDebug() << "Requesting stop from MCU";
    serial.write(CMD_STOP);
    serial.waitForBytesWritten(5);

    qDebug() << "Clearing All Pending Serial Data";
    serial.clear();

    qDebug() << "Closing Serial Port";
    serial.close();

    if(bWriteToFile) {
        qDebug() << "Closing file: " << m_file->fileName();
        m_file->close();

        qDebug() << "Freeing file pointer";
        delete m_file;
    }

    if(m_tx_buffer_len > 0){
        _mm_free(m_key_buffer);
        _mm_free(m_val_buffer);
        _mm_free(m_tx_buffer);
        _mm_free(m_sequency);
        //fftw_destroy_plan(p_fwd);
        //fftw_free(m_fft_out_buffer);
        //fftw_free(m_fft_pwr_spectrum);
    }
    if(m_waveform_buffer_len > 0){
        _mm_free(m_waveform_buffer);
        _mm_free(m_waveform_buffer_tx);
        _mm_free(m_waveform_cdata);
        _mm_free(m_waveform_sequency);
    }


    if(m_multipart_payload_len > 0){
        _mm_free(m_multipart_payload);
    }

    if(m_datacollection != nullptr) {
        free(m_datacollection);
    }
}

void SerialThread::setOffset(int offset) {
    m_offset = offset;
}

void SerialThread::printHeader(const SerialHeader &header) {
    qDebug() << "SERIAL HEADER";
    qDebug() << "    timestamp:         " << header.timestamp;
    qDebug() << "    item_count:        " << header.item_count;
    qDebug() << "    buffer_chunk_idx:  " << header.buffer_chunk_idx;
    qDebug() << "    buffer_num_chunks: " << header.buffer_num_chunks;
    qDebug() << "    tx_buffer_size:    " << header.tx_buffer_size;
}

// sanity checks on the header...
inline bool SerialThread::validHeader(const SerialHeader &header) {
    static unsigned const tx_limit = 1u<<16u;
    static unsigned const item_limit = 1u<<12u;

    return header.item_count < item_limit &&
        header.buffer_chunk_idx < 10 &&
        header.buffer_num_chunks > 0 &&
        header.buffer_num_chunks < 10 &&
        header.tx_buffer_size < tx_limit;
}

void SerialThread::applyWaveformSettings(const QSettings &settings ) {
    QString mode_str = settings.value(KEY_MODE).toString();
    QString basis_str = settings.value(KEY_BASIS).toString();
    m_length = settings.value(KEY_WAVEFORM_LENGTH).toString().split(' ')[0].toInt();
    m_order = ilog2(m_length) - 1;

    m_current_functions.clear();

    // eventually handle fourier
    //settings.setValue("Basis", "Walsh" );

    rangeStart = settings.value(KEY_RANGE_START).toInt();
    rangeEnd = settings.value(KEY_RANGE_END).toInt();

    if(mode_str.compare(VAL_MODE_CUSTOM, Qt::CaseInsensitive) == 0){
        mode = Ui::WaveformMode::Custom;
        foreach(const QString &function, settings.value(KEY_CUSTOM_FUNCTIONS).toString().split(',')){
            m_current_functions.append(function.trimmed().toInt());
        }
    } else if(mode_str.compare(VAL_MODE_CHIRP, Qt::CaseInsensitive) == 0) {
        mode = Ui::WaveformMode::Chirp;
        // start with the first sequency
        m_current_functions.append(1);
        m_chirp_hop_ms = settings.value(KEY_HOP_FREQ).toInt();
        m_chirp_hop.invalidate();

    } else if(mode_str.compare(VAL_MODE_ADAPTIVE, Qt::CaseInsensitive) == 0) {
        mode = Ui::WaveformMode::Adaptive;
        m_noise_recalc_ms = settings.value(KEY_NOISE_INTERVAL).toInt();
        m_num_adaptive_fns = settings.value(KEY_NUM_FUNCTIONS).toInt();
        m_noise_recalc.invalidate();
        // start with random values...
        for(int i = 0; i < m_num_adaptive_fns; i++){
            m_current_functions.append(QRandomGenerator::global()->bounded(rangeStart, rangeEnd));
        }
    }

    if(basis_str.compare(VAL_BASIS_WALSH, Qt::CaseInsensitive) == 0){
        basis = Ui::BasisFunctions::Walsh;
    } else if(basis_str.compare(VAL_BASIS_FOURIER, Qt::CaseInsensitive) == 0){
        basis = Ui::BasisFunctions::Fourier;
    }

    m_newsettings = true;
}

void SerialThread::generateUpdatedWaveform() {
    const unsigned ADC_BIT_DEPTH = 12u;
    const double ADC_HALF_RANGE = (1u << (ADC_BIT_DEPTH - 1u)) - 0.5; // 2047.5

    // create the necesary buffers for this length
    if(m_waveform_buffer_len != m_length) {
        if (m_waveform_buffer_len > 0) {
            qDebug() << "generateUpdatedWaveform: Freeing Old Waveform Buffer: " << m_waveform_buffer_len;
            _mm_free(m_waveform_buffer);
            _mm_free(m_waveform_buffer_tx);
            _mm_free(m_waveform_cdata);
            _mm_free(m_waveform_sequency);

            fftw_destroy_plan(p_rev);
            _mm_free(m_waveform_fft_cdata);
            _mm_free(m_fft_waveform_buffer);
            fftw_free(m_fft_waveform_buffer_tx);
        }
        qDebug() << "generateUpdatedWaveform: Allocating Waveform Buffers" << m_length;
        m_waveform_buffer = (double *) _mm_malloc(m_length * sizeof(double), 256);
        m_waveform_buffer_tx = (double *) _mm_malloc(m_length * sizeof(double), 256);
        m_waveform_cdata = (uint16_t *) _mm_malloc(m_length * sizeof(uint16_t), 256);
        m_waveform_sequency = (unsigned *) _mm_malloc(sizeof(unsigned) * m_length, 256);

        m_fft_waveform_buffer_tx =  (fftw_real *) fftw_malloc(m_length * sizeof(fftw_real));
        m_fft_waveform_buffer =  (fftw_complex *) _mm_malloc((m_length) * sizeof(fftw_complex),256);

        m_waveform_fft_cdata = (uint16_t *) _mm_malloc(m_length * sizeof(uint16_t), 256);
        m_waveform_buffer_len = m_length;

        qDebug() << "generateUpdatedWaveform Done Allocating " << m_length;
        // recompute the sequency permutation for the new length...
        fwht_sequency_permutation(m_waveform_sequency, m_order);
        //qDebug() << "fwht_sequency_permutation complete";

        p_rev = fftw_plan_dft_c2r_1d(m_length, m_fft_waveform_buffer, m_fft_waveform_buffer_tx, FFTW_ESTIMATE);
    }
    // AVX Gives 4x Speedup, even after gcc -O3 (see data_read_performance.cpp)
    for(int i = 0; i < m_length; i+=4) {
        _mm256_store_pd(&m_waveform_buffer[i], _mm256_setr_pd(0,0, 0, 0));
    }
    // AVX Gives 4x Speedup, even after gcc -O3 (see data_read_performance.cpp)
//    for(int i = 0; i < m_length; i++) {
//        //_mm256_store_pd((double*)&m_fft_waveform_buffer[i], _mm256_setr_pd(0,0, 0, 0));
//        m_fft_waveform_buffer_tx[i] = 0.0;
//    }
    // AVX Gives 4x Speedup, even after gcc -O3 (see data_read_performance.cpp)
    for(int i = 0; i < m_length/2+1; i++) {
        m_fft_waveform_buffer[i][0] = 0.0;
        m_fft_waveform_buffer[i][1] = 0.0;
        //_mm256_store_pd((double*)&m_fft_waveform_buffer_tx[i], _mm256_setr_pd(0,0, 0, 0));
    }

    // even though we validate the user input, re-validate the indexes
    // here just in case (otherwise seg fault...)
    for(int i = m_current_functions.size()-1; i>=0; i--) {
        if(m_current_functions[i] < 0 || m_current_functions[i] > m_length-1){
            qDebug() << "Removing bad Waveform Basis Function at index " << i << ":" << m_current_functions[i];
            m_current_functions.remove(i);
        }
    }
    if(m_current_functions.size()  == 0){
        qDebug() << "No valid waveforms! Adding \"5\" to recover...";
        m_current_functions.append(5);
    }

    //qDebug() << "m_current_functions.size()" << m_current_functions.size();
    double idx_mag = ADC_HALF_RANGE / (double)m_current_functions.size();
    //qDebug() << "idx_mag" << idx_mag;
//    qDebug() << "ADC_HALF_RANGE" << ADC_HALF_RANGE;
    m_waveform_buffer[0] = ADC_HALF_RANGE;

    for(int i = 0; i<m_current_functions.size(); i++){
        m_waveform_buffer[m_current_functions[i]] = idx_mag;
    }
//    qDebug() << "2" << m_length;
    fwht_iterative_simd(m_waveform_buffer_tx, m_waveform_buffer, m_waveform_sequency, m_length);

    m_fft_waveform_buffer[0][0] = ADC_HALF_RANGE;
    double fft_mag = ADC_HALF_RANGE / (2 * (double)m_current_functions.size());
    for(int i = 0; i<m_current_functions.size(); i++){
        m_fft_waveform_buffer[m_current_functions[i]/2][m_current_functions[i]%2] = fft_mag;
    }
    fftw_execute(p_rev);

//    for(int i =0;i < m_length/2+1; i++){
//        m_fft_pwr_spectrum[i] = m_fft_waveform_tx[i][0] * m_fft_waveform_tx[i][0] + m_fft_waveform_tx[i][1] * m_fft_waveform_tx[i][1];
//    }

//    qDebug() << "3" << m_length;
    for(int i = 0; i < m_length; i++) {
        // TODO: use AVX -> double -> 32-bit -> 16-bit unsigned
        // _mm256_cvtpd_epi32
        m_waveform_cdata[i] = (uint16_t)(m_waveform_buffer_tx[i]);
//        if(i<16){
//            qDebug() << "m_waveform_buffer_tx[i]" << m_waveform_buffer_tx[i];
//            qDebug() << "m_waveform_cdata[i]" << m_waveform_cdata[i];
//        }
    }

    for(int i = 0; i < m_length; i++) {
        m_waveform_fft_cdata[i] = (uint16_t)(m_fft_waveform_buffer_tx[i]);
//        if(i<16){
//            qDebug() << "m_fft_waveform_buffer_tx[" << i << "] = " << m_fft_waveform_buffer_tx[i];
//        }
    }

//    qDebug() << "4" << m_length;
    // announce the new waveform to the UI

    if(basis == Ui::BasisFunctions::Walsh){
        emit waveformUpdate(m_waveform_buffer_tx, m_waveform_buffer, m_length, basis, mode);
    } else if(basis == Ui::BasisFunctions::Fourier){
        emit waveformUpdate((double *)m_fft_waveform_buffer_tx,
                            (double *)m_fft_waveform_buffer, m_length, basis, mode);
    }

    if(mode == Ui::WaveformMode::Chirp) {
        m_chirp_hop.start();
    } else if(mode == Ui::WaveformMode::Adaptive){
        m_noise_recalc.start();
    }

    m_newsettings = false;
    m_waveform_change = true;
//    qDebug() << "5" << m_length;
}


//void SerialThread::requestNewWaveform(const QList<int> &walshIndices, unsigned int order) {
//    const unsigned ADC_BIT_DEPTH = 12u;
//    const double ADC_HALF_RANGE = (1u << (ADC_BIT_DEPTH - 1u)) - 0.5;
//    unsigned length = 1u<<order;
//
//    if(m_waveform_buffer_len != length) {
//        if (m_waveform_buffer_len > 0) {
//            qDebug() << "Freeing Old Waveform Buffer: " << m_waveform_buffer_len;
//            _mm_free(m_waveform_buffer);
//            _mm_free(m_waveform_buffer_tx);
//            _mm_free(m_waveform_cdata);
//            _mm_free(m_waveform_sequency);
//        }
//        qDebug() << "Allocating Waveform Buffers" << length;
//        m_waveform_buffer = (double *) _mm_malloc(length * sizeof(double), 256);
//        m_waveform_buffer_tx = (double *) _mm_malloc(length * sizeof(double), 256);
//        m_waveform_cdata = (uint16_t *) _mm_malloc(length * sizeof(uint16_t), 256);
//        m_waveform_sequency = (unsigned *) _mm_malloc(sizeof(unsigned) * length, 256);
//        m_waveform_buffer_len = length;
//
//        // recompute the sequency for the new order...
//        fwht_sequency_permutation(m_waveform_sequency, order);
////        qDebug() << "Sequency Recomputed";
////        for(int i = 0; i < length; i++){
////            printf("%d ", m_waveform_sequency[i]);
////        }
////        printf("\n");
//
//    }
//
//    // AVX Gives 4x Speedup, even after gcc -O3 (see data_read_performance.cpp)
//    for(int i = 0; i < length; i+=4) {
//        _mm256_store_pd(&m_waveform_buffer[i], _mm256_setr_pd(0,0, 0, 0));
//    }
//
//    // NOTE: can potentially skip the entire scaling process by altering the peak hights
//    /// and the DC-value (pre-transform) accordingly.
//    double idx_mag = ADC_HALF_RANGE / (double)walshIndices.size();
//    m_waveform_buffer[0] = ADC_HALF_RANGE;
//    for(int i = 0; i<walshIndices.size(); i++){
//        m_waveform_buffer[walshIndices[i]] = idx_mag;
//    }
//
//
////    for(int i = 0; i<walshIndices.size(); i++){
////        m_waveform_buffer[walshIndices[i]] = 1;
////    }
////    qDebug() << "m_waveform_buffer input:";
////    for(int i = 0; i < length; i++){
////        printf("%f ", m_waveform_buffer[i]);
////    }
////    printf("\n");
//
//
////    fwht_iterative_simd(m_waveform_buffer_tx, m_waveform_buffer, length);
//
//    fwht_iterative_simd(m_waveform_buffer_tx, m_waveform_buffer, m_waveform_sequency, length);
////    qDebug() << "fwht_iterative_simd1:";
////    for(int i = 0; i < length; i++){
////        printf("%f ", m_waveform_buffer_tx[i]);
////    }
////    printf("\n");
//
////    double min = m_waveform_buffer_tx[0];
////    // todo: use _mm256_min_pd (__m256 a, __m256 b)
////    for(int i = 0; i < length; i++) {
////        if(min > m_waveform_buffer_tx[i]){
////            min = m_waveform_buffer_tx[i];
////        }
////    }
////
////    double max = m_waveform_buffer_tx[0] - min;
////    for(int i = 0; i < length; i++) {
////        m_waveform_buffer_tx[i] = m_waveform_buffer_tx[i] - min;
////
////        if(max < m_waveform_buffer_tx[i]){
////            max = m_waveform_buffer_tx[i];
////        }
////    }
////    qDebug() << "fwht_iterative_shifted:";
////    for(int i = 0; i < length; i++){
////        printf("%f ", m_waveform_buffer_tx[i]);
////    }
////    printf("\n");
//
//    // bit-depth is always 12 due to the MCU's ADC
////    double factor = ((double)((1 << 12) - 1)) / max;
////    for(int i = 0; i < length; i++) {
////        m_waveform_cdata[i] = (uint16_t)(factor * m_waveform_buffer_tx[i]);
////    }
//
//
//    for(int i = 0; i < length; i++) {
//        // TODO: use AVX -> double -> 32-bit -> 16-bit unsigned
//        // _mm256_cvtpd_epi32
//        m_waveform_cdata[i] = (uint16_t)(m_waveform_buffer_tx[i]);
//    }
//
//
////    qDebug() << "m_waveform_cdata:";
////    for(int i = 0; i < length; i++){
////        printf("%d ", m_waveform_cdata[i]);
////    }
////    printf("\n");
//
//    m_waveform_change = true;
//}



