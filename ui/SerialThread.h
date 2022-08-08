#ifndef SERIALTHREAD_H
#define SERIALTHREAD_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QtEndian>
#include <QFile>
#include <QtCore/QByteArrayMatcher>
#include <QtCore/QSettings>
#include "constants.h"

#include "fftw/fftw3.h"
typedef double fftw_real;

typedef struct SerialHeader {
    unsigned magic_number;
    unsigned timestamp;
    unsigned item_count;
    unsigned tx_buffer_size;
    unsigned buffer_chunk_idx;
    unsigned buffer_num_chunks;
} SerialHeader;

const int MAX_FUNCTIONS = 255;
typedef struct DataCollection {
    unsigned timestamp;
    double *m_keys;
    double *m_values;
    double *transformWHT;
    double *transformFFT;
    double signal_energy;
    double noise_energy;
    double snr;
    unsigned len;
    int functions[MAX_FUNCTIONS];
    int num_functions = -1;
    Ui::BasisFunctions basis;
} DataCollection;

//namespace Ui { class SerialThread; }

//using namespace Ui;

class SerialThread : public QThread
{
    Q_OBJECT

public:
    explicit SerialThread(QObject *parent = nullptr);
//    explicit SerialThread();
    ~SerialThread() override;
    void begin(const QString &portName, int baudRate, int waitTimeout,
                QSettings &settings);
    void setOffset(int offset);


signals:
    void response(const QString &s);
//    void data(quint32_le timestamp,
//              const QVector<double> &keys,
//              const QVector<double> &values,
//              const QVector<double> &transform);

    void data(DataCollection *data);

    void disconnecting();

    // TODO add waveform data and statistics...
    void waveformChange();
    void waveformUpdate(double *walsh_waveform, double *walsh_transform,
                        unsigned len, Ui::BasisFunctions basis, Ui::WaveformMode mode);

//    void data(const int &timestamp);
    void status(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);

    void connectionRecovered();
public slots:
    void requestData();
    //void requestNewWaveform(const QList<int>&walshIndices, unsigned order);
    void applyWaveformSettings(const QSettings &settings );
private:

    void generateUpdatedWaveform();

    void run() override;
    QString m_portName;
    int m_waitTimeout = 0;
    int m_baudRate = 6000000;
    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_quit = false;
    QVector<int> m_dataBuffer = QVector<int>(4096);
    bool m_dataReqested = false;

    // byte aligned buffer for SIMD Walsh-Hadamard Transform
    double *m_key_buffer;
    double *m_val_buffer;
    double *m_tx_buffer;
    unsigned *m_sequency;
    fftw_complex *m_fft_out_buffer;

    // fftw buffers for real-valued FFT (just a double* ...)
    fftw_real *m_fft_waveform_buffer_tx;
    fftw_complex *m_fft_waveform_buffer;
    uint16_t *m_waveform_fft_cdata;
    double *m_fft_pwr_spectrum;
    fftw_plan p_fwd;
    fftw_plan p_rev;

    // size of a buffer chunk from MCU
    // padded with header information and a "stop" bit
    unsigned m_buffer_len;

    // logical size of the transform buffer and sequence length
    unsigned m_tx_buffer_len = 0;
    int m_tx_buffer_filled = 0;

    double *m_waveform_buffer;
    double *m_waveform_buffer_tx;
    uint16_t *m_waveform_cdata;
    unsigned *m_waveform_sequency;
    unsigned m_waveform_buffer_len = 0;

    DataCollection *m_datacollection = nullptr;

//    QVector<float> m_keys = QVector<float>(4096);
//    QVector<float> m_values = QVector<float>(4096);
//    QVector<float> m_transform = QVector<float>(4096);

    QVector<double> m_keys = QVector<double>(4096);
    QVector<double> m_values = QVector<double>(4096);
    QVector<double> m_transform = QVector<double>(4096);
//    QVector<uint> m_sequency = QVector<uint>(4096);
    bool m_sequency_init = false;


    const QByteArray CMD_START = QString("1").toUtf8();
    const QByteArray CMD_STOP = QString("0").toUtf8();
    const QByteArray CMD_BIN_MODE = QString("b").toUtf8();
    const QByteArray CMD_TEXT_MODE = QString("t").toUtf8();
    const QByteArray CMD_WAVEFORM_CHANGE = QString("w").toUtf8();
    const QByteArray CMD_FAST_WAVEFORM_CHANGE = QString("x").toUtf8();

    const QByteArray CMD_END_BYTE = QString("\n").toUtf8();
    //const int END_CHAR = QString("\n").toUtf8().at(0);
    const char END_CHAR = 10;
    const char STOP_BYTE = (char)0; // some


    QElapsedTimer m_timer;

    //void updateStats();
    //void processChunk(const QByteArray &ptr);
    bool processMultiPartChunk(const uint16_t *uint16_data);

    static void printHeader(const SerialHeader &header);
    static inline bool validHeader(const SerialHeader &header);

    // file storage
    // Save the data to a file.
    QFile *m_file; //"C:/MyDir/some_name.ext");
    bool bWriteToFile = false;
    QString m_saveFormat;
    /*file.open(QIODevice::WriteOnly);
    file.write(data);
    file.close();*/

    int m_offset;
    SerialHeader m_header;
    unsigned magic_number = 0xDABBAD00;
    QByteArray pattern;
    QByteArrayMatcher matcher;

    QByteArrayList m_multipart_chunk;
    char *m_multipart_payload;
    int m_multipart_payload_len = 0;
    bool m_waveform_change = false;

    // waveform shaping
//    QString mode;


    Ui::WaveformMode mode;
    Ui::BasisFunctions basis;
    unsigned rangeStart;
    unsigned rangeEnd;
    QElapsedTimer m_noise_recalc;
    unsigned m_noise_recalc_ms = 250;
    QElapsedTimer m_chirp_hop;
    unsigned m_chirp_hop_ms = 250;
    QVector<int> m_current_functions;
    double m_signal_magnitude;

    // in order to discard anomalous values during
    // waveform change, store the last 5 and check for outliers
    const int HISTORY_SIZE = 7;
    QVector<double> signal_history = QVector<double>(HISTORY_SIZE);
    QVector<double> signal_history_diff = QVector<double>(HISTORY_SIZE);
    int signal_history_idx = 0;
    int history_init = 0;

    unsigned m_num_adaptive_fns;
    unsigned m_length;
    unsigned m_order;
    bool m_newsettings = false;
};

#endif // SERIALTHREAD_H

