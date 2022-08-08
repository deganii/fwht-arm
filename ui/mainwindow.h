#ifndef MAINWINDOW_H
#define MAINWINDOW_H



#include <QMainWindow>
#include <QSerialPortInfo>
#include <Q3DSurface>
#include <QSurface3DSeries>
#include "qcp/qcustomplot.h"
#include "SerialThread.h"

using namespace QtDataVisualization;

namespace Ui { class MainWindow; }


class BasisValidator : public QValidator
{
Q_OBJECT
public:
    explicit BasisValidator(QObject *parent = 0){}

    virtual State validate ( QString & input, int & pos ) const
    {
        if (input.isEmpty() || input.trimmed().isEmpty()) {
            return Intermediate;
        }
        // split into integers
        bool ok;
        foreach(const QString &token, input.split(',')){
            // try to parse
            int function = token.toInt(&ok);
            if(!ok){
                return Intermediate;
            }
            if(function < 0 ||  function >= r){
                return Intermediate;
            }
        }
        return Acceptable;
    }

    void setRange(int range){
        r = range;
    };

private:
    int r;
};

class IconPaintFilter : public QObject
{
Q_OBJECT
public:
    IconPaintFilter(): QObject(nullptr) {}
    bool eventFilter(QObject *watched, QEvent *event);
};

const int WINDOW_SIZE = 60;
//const int MAX_FUNCTIONS = 255;
struct Measurement {
    int id = -1;
    double mean = -1;
    double stdev = -1;
    int num_constituents = -1;
    double constituent_keys[WINDOW_SIZE];
    double constituent_values[WINDOW_SIZE];
    QString basis;
    int functions[MAX_FUNCTIONS];
    int num_functions = -1;
    int seq_length;
    QString settings;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


public slots:
    void onData(DataCollection *data);
    void onResponse(const QString &s);
    void handleConnect();
    void UpdateFPS(qreal fps);
    void onAutoCenterToggled(bool newValue);
    void onOffsetTriggered();
//    void onNewWaveform();
    void disconnectCleanup();
    void handleWaveformChange();
    void handleWaveformUpdate(double *waveform, double *walsh,
          unsigned len, Ui::BasisFunctions basis, Ui::WaveformMode mode);
    void handleConnectionRecovered();
    void onWaveformDesignerToggled(bool);

    void onWaveformDesignerChanged();
    void onWaveformLengthChanged(QString newText);
    void onWaveformDesignerApply();
    void onWaveformDesignerCancelClose();
    void onWaveformDesignerReset();
    void onWaveformDesignerDefault();

    void onSerialPortRefreshRequested();

    void onSaveSettingChanged();

    void onWHTRangeChanged(QCPRange r);
    void onFFTRangeChanged(QCPRange r);


    void onRequestScreenshot();
    void onClose();

    void onCopyRequested();
    void onThemeSettingChanged();
    void onLogScaleChanged(bool);
protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
private:
    void InitPorts();
    void SetupSignalPlot();
    void SetupSnrPlot();
    void Setup3d();
    void SetupWaveformDesigner();

    Ui::MainWindow *ui;
    QCPAxisRect *whAxisRect;
    QCPAxisRect *fftAxisRect;
    QCPAxisRect *noiseAxisRect;
    QCPGraph *noiseGraph;
    QCPGraph *snrGraph;
    QCPGraph *measurementGraphMajor;
    QCPGraph *measurementGraphMinor;
    QCPErrorBars *errorBars;
    QCPBars *noiseDistGraph;
    QCPGraph *transformWHTGraph;
    QCPGraph *transformFFTGraph;
    QCPGraph *signalGraph;
    QCPGraph *snrGraphMean;
    QCPGraph *snrGraphVarPos;
    QCPGraph *snrGraphVarNeg;
//    QCPGraph *filteredSignalGraph;
//    QCPGraph *lockinReadoutGraph;
    QCPItemText *labelSignal;
    QCPItemText *labelSnr;
    QCPItemText *labelNoise;
    double bin_width_khz;
    const double range_eps = 1e-5;

    double running_mean = 0;
    double running_variance = 0;
    double running_stdev = 0;
    unsigned running_count = 0;
    double running_sum = 0;
    double running_sq_sum = 0;
    QVector<double> outliers;

//    QPointF
//    QVector<QCPData>experiment = QVector<QVector<double>>(10);

    bool ctrlDown = false;
    QVector<int>experimentNumber = QVector<int>(4);
    int requestMeasurement = -1;
    QVector<Measurement> measurements;


    QSurface3DSeries *timeSeriesOverlap;
    QSurfaceDataArray *m_surface_data;
    Q3DSurface *surface;

    SerialThread *serialThread;
    bool serialConnected = false;
    QTimer *timer;

    bool uiLoading=false;

    QStandardItemModel *model;

    QLabel labelFps;
    QActionGroup *themeGroup;


    bool blAutoScale = true;
    bool blNeedsAutoScale = true;
    bool writeToFile = false;
    //QTimer timerAutoCenter;
    QElapsedTimer m_elapsed;
    int fCounter = 0;
    int m_sig_offset = 0; // current MCU offset (random)


    QList<int> m_list = {21, 10 };

    //DataCollection *m_last_data = nullptr;

    // waveform designer fields

    BasisValidator *basisValidator;

    IconPaintFilter *iconFilter;

    QSettings appSettings;
    bool waveformDesignerLoading=false;
    bool waveformDesignerChanged=false;
    void loadWaveformSettings();
    void saveWaveformSettings();
    void loadWaveformDefaults();
    void updateWaveformUIEnabled();
    QString basisToString(Ui::BasisFunctions basis);
    QString modeToString(Ui::WaveformMode mode);

    void InitSaveSettings();
    bool copyRequested = false;
    bool screenshotRequested = false;

    void doCopyToClipboard(DataCollection *data);
    void doDataSnapshot(DataCollection *data, QString &dest);
    void doSaveScreenshot(DataCollection *data);

    void resetRunningStats();
    void InitThemeSettings();
    void UpdateThemeSetting();
    void UpdateThemeAxis(QCPAxis * axis, const QColor &color);
    void resetSignalPlot();
    void settingsToString(QSettings &settings, QString &dest);
    void waveformSettingsToString(QSettings &settings, QString &dest);

};




#endif // MAINWINDOW_H

