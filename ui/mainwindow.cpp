#include "mainwindow.h"
//#include "./ui_mainwindow.h"
#include "./ui_mainwindow.h"
#include "SvgIconEngine.h"

//#include "cmake-build-debug/UI_autogen/include/ui_mainwindow.h"
#include<QDebug>
//#include <QSurface3DSeries>
#include <QHeightMapSurfaceDataProxy>
#include <QCustom3DLabel>
#include <QCloseEvent>
#include <limits>
#include <QtSvg/QSvgGenerator>
#include "constants.h"
//#include "gl2ps/gl2ps.h"

using namespace QtDataVisualization;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    waveformDesignerLoading = true;
    uiLoading = true;
    iconFilter = new IconPaintFilter();

    ui->setupUi(this);

//    comboPorts = findChild<QComboBox *>("comboPorts");
//    signalPlot = findChild<QCustomPlot *>("signalPlot");
//    snrPlot = findChild<QCustomPlot *>("snrPlot");
//    buttonConnect = findChild<QPushButton *>("buttonConnect");
    timer = new QTimer(this);
//    widget3d = findChild<QWidget *>("widget3d");

//    actionOffset = findChild<QAction *>("actionOffset");
    ui->actionOffset->setText(QString("Offset: ") + QString::number(m_sig_offset));

//    centralwidget = findChild<QWidget *>("centralwidget");
//    widgetDesigner = findChild<QWidget *>("widgetDesigner");
//    actionWaveformDesigner = findChild<QAction *>("actionWaveformDesigner");
    ui->widgetDesigner->setVisible(ui->actionWaveformDesigner->isChecked());

    labelFps.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    statusBar()->addPermanentWidget(&labelFps);
    m_elapsed.invalidate();

//    QFile file(":/icons/qdarkstyle/checkbox_checked.svg");
//    file.open(QFile::ReadOnly);
//    QString data = file.readAll();
//    buttonConnect->setIcon(QIcon(new SVGIconEngine(data.toStdString())));

    InitSaveSettings();

    InitPorts();
    SetupSignalPlot();
    SetupSnrPlot();
    Setup3d();
    SetupWaveformDesigner();
    InitThemeSettings();
//    this->radioModeCustom->installEventFilter(iconFilter);

    uiLoading = false;
}



void MainWindow::InitSaveSettings() {
//    checkSave = findChild<QCheckBox *>("checkSave");
//    textSave = findChild<QLineEdit*>("textSave");
    if(!appSettings.contains("Save")){
        appSettings.setValue("Save", false);
        appSettings.setValue("SaveFormat", R"(C:\dev\prbs\data\{date}-exp.data)");
    }
    ui->checkSave->setChecked(appSettings.value("Save").toBool());
    ui->textSave->setText(appSettings.value("SaveFormat").toString());
}

void MainWindow::onSaveSettingChanged() {
    if(!uiLoading) {
        appSettings.setValue("Save", ui->checkSave->isChecked());
        appSettings.setValue("SaveFormat", ui->textSave->text());
    }
}

void MainWindow::InitThemeSettings() {
    if(!appSettings.contains("Theme")){
        appSettings.setValue("Theme", "qdarkstyle");
    }
    QString theme = appSettings.value("Theme").toString();
    themeGroup = new QActionGroup(this);
    themeGroup->addAction(ui->actionThemeDark);
    themeGroup->addAction(ui->actionThemeWindows);
    themeGroup->setExclusive(true);
    ui->actionThemeDark->setChecked(theme.compare("qdarkstyle",Qt::CaseInsensitive)==0);
    ui->actionThemeWindows->setChecked(theme.compare("windows",Qt::CaseInsensitive)==0);
    UpdateThemeSetting();
}

void MainWindow::onThemeSettingChanged() {
    if (!uiLoading) {
        appSettings.setValue("Theme", ui->actionThemeDark->isChecked() ? "qdarkstyle" : "windows");
        UpdateThemeSetting();
    }
}

void MainWindow::UpdateThemeAxis(QCPAxis *axis, const QColor &color) {
    axis->setTickLabelColor(color);
    axis->setBasePen(QPen(color));
    axis->setLabelColor(color);
    axis->setTickPen(QPen(color));
    axis->setSubTickPen(QPen(color));
}

void MainWindow::UpdateThemeSetting() {
    if(ui->actionThemeDark->isChecked()){
        QFile file(":/themes/qdarkstyle.qss");
        file.open(QFile::ReadOnly);
        QString styleSheet = QString(file.readAll());
        this->setStyleSheet(styleSheet);

        ui->buttonRefresh->setIcon(QIcon(":/icons/refresh_dark.svg"));
        ui->buttonRandom->setIcon(QIcon(":/icons/random_dark.svg"));

        // dark settings for plots
        surface->activeTheme()->setType(Q3DTheme::ThemeEbony);

        QColor windowColor(17, 24, 31);// Qt::black);

        ui->signalPlot->setBackground(windowColor);
        ui->snrPlot->setBackground(windowColor); // Qt::black);

        UpdateThemeAxis(ui->signalPlot->xAxis, Qt::white);
        UpdateThemeAxis(ui->signalPlot->yAxis, Qt::white);
        UpdateThemeAxis( ui->snrPlot->xAxis, Qt::white);
        UpdateThemeAxis( ui->snrPlot->yAxis, Qt::white);
        UpdateThemeAxis(fftAxisRect->axis(QCPAxis::atBottom), Qt::white);
        UpdateThemeAxis(fftAxisRect->axis(QCPAxis::atLeft), Qt::white);
        UpdateThemeAxis(whAxisRect->axis(QCPAxis::atBottom), Qt::white);
        UpdateThemeAxis(whAxisRect->axis(QCPAxis::atLeft), Qt::white);
        UpdateThemeAxis(noiseAxisRect->axis(QCPAxis::atBottom), Qt::white);
        UpdateThemeAxis(noiseAxisRect->axis(QCPAxis::atLeft), Qt::white);

        surface->activeTheme()->setWindowColor(windowColor); //Qt::black);
        surface->activeTheme()->setBackgroundColor(Qt::transparent);

        // don't change the base colors
        surface->activeTheme()->setBaseColors(Q3DTheme(Q3DTheme::ThemeQt).baseColors());

        labelSnr->setColor(Qt::white);
        labelNoise->setColor(Qt::white);
        labelSignal->setColor(Qt::white);
        noiseDistGraph->setPen(QPen(windowColor,0));
    } else {
        this->setStyleSheet("");

        ui->buttonRefresh->setIcon(QIcon(":/icons/refresh.svg"));
        ui->buttonRandom->setIcon(QIcon(":/icons/random.svg"));

        // light settings for plots
        surface->activeTheme()->setType(Q3DTheme::ThemeQt);

        ui->signalPlot->setBackground(Qt::white);
        ui->snrPlot->setBackground(Qt::white);

        UpdateThemeAxis(ui->signalPlot->xAxis, Qt::black);
        UpdateThemeAxis(ui->signalPlot->yAxis, Qt::black);
        UpdateThemeAxis( ui->snrPlot->xAxis, Qt::black);
        UpdateThemeAxis( ui->snrPlot->yAxis, Qt::black);
        UpdateThemeAxis(fftAxisRect->axis(QCPAxis::atBottom), Qt::black);
        UpdateThemeAxis(fftAxisRect->axis(QCPAxis::atLeft), Qt::black);
        UpdateThemeAxis(whAxisRect->axis(QCPAxis::atBottom), Qt::black);
        UpdateThemeAxis(whAxisRect->axis(QCPAxis::atLeft), Qt::black);

        UpdateThemeAxis(noiseAxisRect->axis(QCPAxis::atBottom), Qt::black);
        UpdateThemeAxis(noiseAxisRect->axis(QCPAxis::atLeft), Qt::black);

        QColor darkblue(17, 24, 31);
        labelSignal->setColor(darkblue);
        labelSnr->setColor(darkblue);
        labelNoise->setColor(darkblue);
        noiseDistGraph->setPen(QPen(Qt::white,0));
    }
}




void MainWindow::UpdateFPS(qreal fps){
//    qDebug() << "FPS: " << fps;
//    statusBar()->showMessage(QString("FPS: ") + QString::number(fps));
    labelFps.setText(QString("FPS: ") + QString::number(fps, 'f', 2));
}

void MainWindow::Setup3d(){
//    Q3DScatter *graph = new Q3DScatter();
    surface = new Q3DSurface();
//    surface->setShadowQuality(QAbstract3DGraph::ShadowQualityNone);
    surface->setMeasureFps(true);
    connect(surface,
            &Q3DSurface::currentFpsChanged,
            this,
            &MainWindow::UpdateFPS);

    QWidget *container = QWidget::createWindowContainer(surface);
    if (!surface->hasContext()) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't initialize the OpenGL context.");
        msgBox.exec();

    }

    QGridLayout *gLayout = new QGridLayout(ui->widget3d);
    gLayout->setContentsMargins(0,0,0,0);
    gLayout->addWidget(container, 0,0);
//    QImage layerOneHMap(R"(C:\Qt\Examples\Qt-5.15.2\datavisualization\customitems\layer_1.png)");
//    QHeightMapSurfaceDataProxy *layerOneProxy = new QHeightMapSurfaceDataProxy(layerOneHMap);
//
    timeSeriesOverlap = new QSurface3DSeries();//layerOneProxy);
    timeSeriesOverlap->setDrawMode(QSurface3DSeries::DrawSurface);

    timeSeriesOverlap->setItemLabelFormat(QStringLiteral("(@xLabel, @zLabel): @yLabel"));
//    layerOneProxy->setValueRanges(34.0f, 40.0f, 18.0f, 24.0f);
    timeSeriesOverlap->setDrawMode(QSurface3DSeries::DrawSurface);
    timeSeriesOverlap->setFlatShadingEnabled(false);
    surface->axisX()->setLabelFormat("%.1f");
    surface->axisZ()->setLabelFormat("%.1f");


    surface->axisX()->setTitle(QStringLiteral("Sample"));
    surface->axisY()->setTitle(QStringLiteral("Time"));
    surface->axisZ()->setTitle(QStringLiteral("Intensity"));

    surface->addSeries(timeSeriesOverlap);

    surface->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetFrontLow);

    m_surface_data = new QSurfaceDataArray();

//    QFont titleFont = QFont("Century Gothic", 30);
//    titleFont.setBold(true);
    /*QCustom3DLabel *titleLabel = new QCustom3DLabel("Oil Rigs on Imaginary Sea", titleFont,
                                                    QVector3D(0.0f, 1.2f, 0.0f),
                                                    QVector3D(1.0f, 1.0f, 0.0f),
                                                    QQuaternion());
    titleLabel->setPositionAbsolute(true);
    titleLabel->setFacingCamera(true);
    titleLabel->setBackgroundColor(QColor(0x66cdaa));
    surface->addCustomItem(titleLabel);*/
}

void MainWindow::SetupSignalPlot(){
    QFont pfont("helvetica",10);
    pfont.setStyleHint(QFont::SansSerif);
//    pfont.setBold(true);
//    pfont.setItalic(true);
    pfont.setPointSize(10);

//    plot->setFont(pfont);

    //plot->setOpenGl(true, 8);
    ui->signalPlot->addGraph(); // blue line
    ui->signalPlot->graph(0)->setPen(QPen(QColor(40, 110, 255)));
    ui->signalPlot->graph(0)->setName("Observed Signal");
//    plot->yAxis->setRange(4000, 4200);
    ui->signalPlot->xAxis->setRange(0, 512);
    ui->signalPlot->yAxis->setRange(0, 100);
    ui->signalPlot->yAxis2->setRange(-500, 4500);

    ui->signalPlot->xAxis->setLabelFont(pfont);
    ui->signalPlot->yAxis->setLabelFont(pfont);
    ui->signalPlot->xAxis->setLabel("Time (μs)");
    ui->signalPlot->yAxis->setLabel("Fl. Intensity (a.u.)");

//    plot->yAxis2->setLabel("DEBUG");
    ui->signalPlot->legend->setVisible(true);
    // add the "expected" waveform plot
    // setup for graph 2: key axis top, value axis right
// will contain high frequency sine with low frequency beating:
    ui->signalPlot->addGraph(ui->signalPlot->xAxis, ui->signalPlot->yAxis2);
    ui->signalPlot->graph(1)->setPen(QPen(QColor(255, 127, 14)));
    ui->signalPlot->graph(1)->setName("Expected Signal");
    ui->signalPlot->graph(1)->setLineStyle(QCPGraph::lsStepCenter);

    // add the WH transform plot
    fftAxisRect = new QCPAxisRect(ui->signalPlot);
    ui->signalPlot->plotLayout()->addElement(1, 0, fftAxisRect);
    fftAxisRect->axis(QCPAxis::atBottom)->setLayer("axes");
    fftAxisRect->axis(QCPAxis::atBottom)->grid()->setLayer("grid");
    fftAxisRect->axis(QCPAxis::atLeft)->setLabelFont(pfont);
    fftAxisRect->axis(QCPAxis::atBottom)->setLabelFont(pfont);
    fftAxisRect->axis(QCPAxis::atBottom)->setLabel("Frequency (kHz)");
    fftAxisRect->axis(QCPAxis::atLeft)->setLabel("Pwr Spectral Density"); // v^2/hZ


    // add the WH transform plot
    whAxisRect = new QCPAxisRect(ui->signalPlot);
    ui->signalPlot->plotLayout()->addElement(2, 0, whAxisRect);
    //whAxisRect->setMaximumSize(QSize(QWIDGETSIZE_MAX, 100));
    whAxisRect->axis(QCPAxis::atBottom)->setLayer("axes");
    whAxisRect->axis(QCPAxis::atBottom)->grid()->setLayer("grid");


    ui->signalPlot->xAxis->setLabelFont(pfont);
    whAxisRect->axis(QCPAxis::atBottom)->setLabelFont(pfont);
    whAxisRect->axis(QCPAxis::atLeft)->setLabelFont(pfont);

    whAxisRect->axis(QCPAxis::atBottom)->setLabel("Walsh-Hadamard Sequency Index");
    whAxisRect->axis(QCPAxis::atLeft)->setLabel("Magnitude (a.u.)");


    //    subLayout->setColumnStretchFactor(0, 3); // left axis rect shall have 60% of width
    //    subLayout->setColumnStretchFactor(1, 2); // right one only 40% (3:2 = 60:40)

    // bring bottom and main axis rect closer together:
    ui->signalPlot->plotLayout()->setRowSpacing(0);
    whAxisRect->setAutoMargins(QCP::msLeft|QCP::msRight|QCP::msBottom);
//    whAxisRect->setAutoMargins(QCP::msRight|QCP::msBottom);
    whAxisRect->setMargins(QMargins(0, 0, 0, 0));

    //whAxisRect->xaxis  setRange(3500, 4500);
    whAxisRect->axis(QCPAxis::atBottom, 0) -> setRange(0, 512);
    whAxisRect->axis(QCPAxis::atLeft, 0) -> setRange( -1000, 1000);
    transformWHTGraph = new QCPGraph(whAxisRect->axis(QCPAxis::atBottom),
                                     whAxisRect->axis(QCPAxis::atLeft));


    const QColor &purple = QColor(93, 30, 223);
    transformWHTGraph->setPen(QPen(purple));
    transformWHTGraph->setName("Observed WHT");


    transformFFTGraph = new QCPGraph(fftAxisRect->axis(QCPAxis::atBottom),
                                     fftAxisRect->axis(QCPAxis::atLeft));


    const QColor &hotpink = QColor(223, 30, 93);
    transformFFTGraph->setPen(QPen(hotpink));
    transformFFTGraph->setName("Observed FFT");
    fftAxisRect->axis(QCPAxis::atBottom, 0) -> setRange(0, 512);
    fftAxisRect->axis(QCPAxis::atLeft, 0) -> setRange( -1000, 1000);

    // make axis rects' left side line up:
    QCPMarginGroup *group = new QCPMarginGroup(ui->signalPlot);
    ui->signalPlot->axisRect()->setMarginGroup(QCP::msLeft | QCP::msRight, group);
    whAxisRect->setMarginGroup(QCP::msLeft|QCP::msRight, group);
    fftAxisRect->setMarginGroup(QCP::msLeft|QCP::msRight, group);

    //whAxisRect->setAutoMargins(QCP::msRight | QCP::msTop | QCP::msBottom);
//    QMargins m = QMargins(QFontMetrics(plot->yAxis->tickLabelFont()).horizontalAdvance("000000000"), 0, 0, 0);
//    qDebug() << m;
//    plot->axisRect()->setMargins(m);
//    whAxisRect->setMargins(m);

    //plot->yAxis->setTickLabelSide(QCPAxis::lsInside);
    //whAxisRect->axis(QCPAxis::atLeft)->setTickLabelSide(QCPAxis::lsInside);
    whAxisRect->axis(QCPAxis::atLeft)->setLabelPadding(10);
    fftAxisRect->axis(QCPAxis::atLeft)->setLabelPadding(10);
    ui->signalPlot->yAxis->setLabelPadding(15);

    ui->signalPlot->setInteraction(QCP::iRangeDrag, true);
    ui->signalPlot->setInteraction(QCP::iRangeZoom, true);


    // interconnect x axis ranges of FFT and WHT:
    QCPAxis *wh_xAxis = whAxisRect->axis(QCPAxis::atBottom);
    QCPAxis *fft_xAxis = fftAxisRect->axis(QCPAxis::atBottom);

    connect(fft_xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(onFFTRangeChanged(QCPRange)));
    connect(wh_xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(onWHTRangeChanged(QCPRange)));

    //plot->legend->itemWithPlottable(transformGraph)->setVisible (false);
    transformWHTGraph->removeFromLegend();
    transformFFTGraph->removeFromLegend();
    //plot->xAxis->setRange(0, 512);

}

void MainWindow::SetupSnrPlot() {
    QFont pfont("helvetica",10);
    pfont.setStyleHint(QFont::SansSerif);
//    pfont.setBold(true);
//    pfont.setItalic(true);
    pfont.setPointSize(10);
//    plot->setFont(pfont);

    //plot->setOpenGl(true, 8);


    signalGraph = new QCPGraph(ui->snrPlot->xAxis, ui->snrPlot->yAxis);
    snrGraphMean = new QCPGraph(ui->snrPlot->xAxis, ui->snrPlot->yAxis);
    snrGraphVarPos = new QCPGraph(ui->snrPlot->xAxis,ui->snrPlot->yAxis);
    snrGraphVarNeg = new QCPGraph(ui->snrPlot->xAxis,ui->snrPlot->yAxis);
    noiseGraph = new QCPGraph(ui->snrPlot->xAxis, ui->snrPlot->yAxis2);
    snrGraph = new QCPGraph(ui->snrPlot->xAxis, ui->snrPlot->yAxis2);
    measurementGraphMajor = new QCPGraph(ui->snrPlot->xAxis2, ui->snrPlot->yAxis);
    measurementGraphMinor = new QCPGraph(ui->snrPlot->xAxis2, ui->snrPlot->yAxis);
    errorBars = new QCPErrorBars(ui->snrPlot->xAxis2, ui->snrPlot->yAxis);

    //snrPlot->addGraph(signalGraph); // blue line
    const QColor &blue = QColor(40, 110, 255, 128);
    signalGraph->setPen(QPen(blue));
    signalGraph->setName("Processed Signal");
    snrGraphMean->setPen(QPen(QColor(123, 185, 65),2.0));
    snrGraphMean->setName("Mean (μ)");

    const QColor &orange = QColor(255, 127, 14);
    snrGraphVarPos->setPen(QPen(orange));
    snrGraphVarPos->setName("Var (μ +/- 2σ)");
    snrGraphVarNeg->setPen(QPen(orange));

    const QColor &gray = QColor(128,128,128,128);
    noiseGraph->setPen(gray);
    noiseGraph->setBrush(gray.lighter());
    noiseGraph->setName("Noise (σ)");

    const QColor &purple = QColor(93, 30, 223,128);
    snrGraph->setPen(QPen(purple.lighter(), 2.0));
    snrGraph->setName("SNR (μ/σ)");


    measurementGraphMajor->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 10));
    measurementGraphMajor->setLineStyle(QCPGraph::lsNone);
    measurementGraphMajor->setPen(QPen(orange));

    measurementGraphMinor->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 2));
    measurementGraphMinor->setLineStyle(QCPGraph::lsNone);

    measurementGraphMinor->setPen(QPen(purple));

    errorBars->removeFromLegend();
    errorBars->setAntialiased(false);
    errorBars->setDataPlottable(measurementGraphMajor);
    errorBars->setPen(QPen(QColor(180,180,180)));
    measurementGraphMajor->setName("Measurements");


//    snrGraphVarNeg->setName("Variance (-2σ)");

//    plot->yAxis->setRange(4000, 4200);
    ui->snrPlot->xAxis->setRange(0, 512);
    ui->snrPlot->yAxis->setRange(0, 100);
//    snrPlot->yAxis2->setRange(-500, 4500);

    ui->snrPlot->xAxis->setLabelFont(pfont);
    ui->snrPlot->yAxis->setLabelFont(pfont);
    ui->snrPlot->xAxis->setLabel("Time (s)");
    ui->snrPlot->yAxis->setLabel("Fl. Intensity (a.u.)");

//    plot->yAxis2->setLabel("DEBUG");

    ui->snrPlot->setInteraction(QCP::iRangeDrag, true);
    ui->snrPlot->setInteraction(QCP::iRangeZoom, true);

    ui->snrPlot->legend->setVisible(true);


    labelSignal = new QCPItemText(ui->snrPlot);
    labelSignal->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    labelSignal->position->setType(QCPItemPosition::ptAbsolute);
    labelSignal->position->setCoords(75, 25);
    labelSignal->setText("Fl: ");
    labelSignal->setFont(QFont(font().family(), 16)); // make font a bit larger
    labelSignal->setColor(QColor(17, 24, 31));

    labelNoise = new QCPItemText(ui->snrPlot);
    labelNoise->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    labelNoise->position->setType(QCPItemPosition::ptAbsolute);
    labelNoise->position->setCoords(75, 50);
    labelNoise->setText("σ: ");
    labelNoise->setFont(QFont(font().family(), 11)); // make font a bit larger
    labelNoise->setColor(QColor(17, 24, 31));

    labelSnr = new QCPItemText(ui->snrPlot);
    labelSnr->setPositionAlignment(Qt::AlignTop | Qt::AlignLeft);
    labelSnr->position->setType(QCPItemPosition::ptAbsolute);
    labelSnr->position->setCoords(75, 68);
    labelSnr->setText("SNR: ");
    labelSnr->setFont(QFont(font().family(), 11)); // make font a bit larger
    labelSnr->setColor(QColor(17, 24, 31));



//    labelSnr->setPen(QPen(Qt::darkBlue)); // show black border around text

    noiseAxisRect = new QCPAxisRect(ui->snrPlot);
    ui->snrPlot->plotLayout()->addElement(0, 1, noiseAxisRect);
    noiseAxisRect->axis(QCPAxis::atBottom)->setLayer("axes");
    noiseAxisRect->axis(QCPAxis::atBottom)->grid()->setLayer("grid");
    noiseAxisRect->axis(QCPAxis::atLeft)->setLabelFont(pfont);
    noiseAxisRect->axis(QCPAxis::atBottom)->setLabelFont(pfont);
    noiseAxisRect->axis(QCPAxis::atBottom)->setLabel("Noise (σ)");
//    noiseAxisRect->axis(QCPAxis::atLeft)->setLabel("Counts");
    ui->snrPlot->plotLayout()->setColumnSpacing(0);
//    noiseAxisRect->axis(QCPAxis::atLeft)->setVisible(false);

    // 3 to 1 spacing
    ui->snrPlot->plotLayout()->setColumnStretchFactor(0,3);
    ui->snrPlot->plotLayout()->setColumnStretchFactor(1,1);

//    ui->snrPlot->axisRect(0)->setMargins(QMargins(0, 0, 0, 0));
    noiseAxisRect->setMargins(QMargins(0, 0, 0, 0));


    noiseDistGraph = new QCPBars(noiseAxisRect->axis(QCPAxis::atLeft),
                                 noiseAxisRect->axis(QCPAxis::atBottom));
    noiseDistGraph->setWidthType(QCPBars::wtPlotCoords);
    noiseDistGraph->setBrush(QBrush(blue));
    noiseDistGraph->setPen(QPen(Qt::white,0));
    snrGraphVarNeg->removeFromLegend();
    noiseDistGraph->removeFromLegend();
    measurementGraphMajor->removeFromLegend();
    measurementGraphMinor->removeFromLegend();
}



void MainWindow::onWHTRangeChanged(QCPRange r) {
    QCPAxis *fft_xAxis = fftAxisRect->axis(QCPAxis::atBottom);
    QCPAxis *wh_xAxis = whAxisRect->axis(QCPAxis::atBottom);
    QCPRange newRange(r.lower * bin_width_khz, r.upper * bin_width_khz);
    QCPRange oldRange = fft_xAxis->range();
//    qDebug()<< "onWHTRangeChanged("  << r.lower  << "," << r.upper << ")";
//    qDebug()<< "newRange("  << newRange.lower  << "," << newRange.upper << ")";
//    qDebug()<< "oldRange("  << oldRange.lower  << "," << oldRange.upper << ")";
    // avoid bouncing back and fdorht
    if(abs(newRange.lower - oldRange.lower) > range_eps || abs(newRange.upper - oldRange.upper) > range_eps) {
        fft_xAxis->setRange(newRange);
//        qDebug()<< "updated fft_xAxis";
    }
}

void MainWindow::onFFTRangeChanged(QCPRange r) {
    QCPAxis *fft_xAxis = fftAxisRect->axis(QCPAxis::atBottom);
    QCPAxis *wh_xAxis = whAxisRect->axis(QCPAxis::atBottom);
//    qDebug()<< "onFFTRangeChanged("  << r.upper  << "," << r.lower << ")";
    QCPRange newRange(r.lower / bin_width_khz, r.upper / bin_width_khz);
    QCPRange oldRange = wh_xAxis->range();
    if(abs(newRange.lower - oldRange.lower) > range_eps || abs(newRange.upper - oldRange.upper) > range_eps) {
        wh_xAxis->setRange(newRange);
//        qDebug()<< "updated wh_xAxis";
    }
//    QCPAxis *fft_xAxis = fftAxisRect->axis(QCPAxis::atBottom);
//    QCPRange fftRange =  fft_xAxis->range();

}




void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    bool show = event->size().height() > 900 &&
            event->size().width() > 900;
    ui->signalPlot->legend->setVisible(show);
    ui->snrPlot->legend->setVisible(show);
}

void MainWindow::changeEvent( QEvent* e )
{
    if( e->type() == QEvent::WindowStateChange )
    {
        QWindowStateChangeEvent* event = static_cast< QWindowStateChangeEvent* >( e );
        if( event->oldState() & Qt::WindowMinimized )
        {
            auto sz = size();
            bool show = sz.width() > 900 && sz.height() > 900;
            ui->signalPlot->legend->setVisible(show);
            ui->snrPlot->legend->setVisible(show);
            qDebug() << "Window restored (to normal or maximized state)!";
        }
        else if( event->oldState() == Qt::WindowNoState && this->windowState() == Qt::WindowMaximized )
        {
            ui->signalPlot->legend->setVisible(true);
            ui->snrPlot->legend->setVisible(true);
            qDebug() << "Window Maximized!";
        }
    }
}


void MainWindow::InitPorts(){
    ui->comboPorts->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        if(info.description().contains("Teensy")){
            //  "C:\Program Files (x86)\Arduino\hardware\tools\teensy_ports.exe" -v
            int serial = info.serialNumber().toInt();
            QString version;
            QString mcu;
            switch(serial){
                case 1436120:
                    version = "3.2";
                    mcu = "MK20DX256";
                    break;
                case 6457830:
                    mcu = "MK66FX1M0";
                    version = "3.6";
                    break;
                default:
                    version = info.serialNumber();
                    break;
            }
            QString s = info.portName() + " (PJRC T" + version + " " + mcu + ")";
            ui->comboPorts->addItem(s, QVariant(info.portName()));
        }
    }
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if(serialConnected){
        handleConnect();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::onClose() {
    if(serialConnected){
        handleConnect();
    } else {
        QApplication::quit();
    }
}


void MainWindow::onAutoCenterToggled(bool newValue) {
    blAutoScale = newValue;
}

void MainWindow::onOffsetTriggered(){
    bool ok;
    QString newOffset = QInputDialog::getText(0, "Offset input dialog",
                                         "Enter signal offset:", QLineEdit::Normal,
                                         QString::number(m_sig_offset), &ok);
    if (ok && !newOffset.isEmpty()) {
        m_sig_offset = newOffset.toInt();
    }
    if(serialConnected){
        serialThread->setOffset(m_sig_offset);
        ui->actionOffset->setText("Offset: " + newOffset);
    }
}

void MainWindow::resetSignalPlot() {
    // clear up the running plots
    noiseDistGraph->data()->clear();
    signalGraph->data()->clear();
    snrGraphMean->data()->clear();
    snrGraphVarNeg->data()->clear();
    snrGraphVarPos->data()->clear();
    noiseGraph->data()->clear();
    snrGraph->data()->clear();
    measurementGraphMajor->data()->clear();
    measurementGraphMinor->data()->clear();
    measurements.resize(0);
}

void MainWindow::handleConnect(){
    if(!serialConnected) {
        resetSignalPlot();
        // connect to the serial port
        int selectedIdx = ui->comboPorts->currentIndex();
        QString portName = ui->comboPorts->itemData(selectedIdx).toString();
        ui->buttonConnect->setText("Disconnect");
        qDebug() << "Connecting to: " << portName;
        serialThread = new SerialThread();

        serialThread->setOffset(m_sig_offset);
        //snrGraph->data()->clear();


        // handle data coming from the serial thread
        connect(serialThread,
                  &SerialThread::data,
                  this,
                  &MainWindow::onData);

        // ask the serial thread for data every n milliseconds
        connect(timer, &QTimer::timeout,
                serialThread,
                &SerialThread::requestData);

        connect(serialThread, &SerialThread::disconnecting,
                this, &MainWindow::disconnectCleanup);

        connect(serialThread, &SerialThread::waveformChange,
                this, &MainWindow::handleWaveformChange);

        connect(serialThread, &SerialThread::connectionRecovered,
                this, &MainWindow::handleConnectionRecovered);

        connect(serialThread, &SerialThread::waveformUpdate,
                this, &MainWindow::handleWaveformUpdate);

//        connect(serialThread,
//                &SerialThread::response,
//                this,
//                &MainWindow::onResponse);

//        connect(serialThread,
//                SIGNAL(data_sig(int, const QVector<float>&, const QVector<float>&)),
//                this, SLOT(onData(int, const QVector<float>&, const QVector<float>&)));


        serialThread->begin(portName.toUpper(), 6000000, 3000,appSettings);

        timer->start(33); //30 FPS
//        timer->start(250); // 4FPS
        serialConnected = true;
    } else {
        disconnectCleanup();
    }
}
void MainWindow::handleWaveformChange() {
    // force an axis refresh on the next frame
    m_elapsed.invalidate();
}

void MainWindow::disconnectCleanup(){
    if(serialConnected) {
        timer->stop();
        // end the serial thread, disconnect all signals/slots
        qDebug() << "Starting deleteLater";
        serialThread->deleteLater();
        serialThread = nullptr;
        ui->buttonConnect->setText("Connect");
        qDebug() << "Done with delete";
        serialConnected = false;
        m_elapsed.invalidate();
    }
}

void MainWindow::onResponse(const QString &s) {
    qDebug() << "onResponse: " << s;
}


void MainWindow::onCopyRequested() {
    // get the raw waveform
    copyRequested = true;
}



void MainWindow::doCopyToClipboard(DataCollection *data) {
    QString clip;
    QClipboard *clipboard = QApplication::clipboard();
    doDataSnapshot(data, clip);
    clipboard->setText(clip);
}

void MainWindow::doDataSnapshot(DataCollection *data, QString &dest) {
    // reserve the preamble plus 10 chars for each double in the 3 arrays
    int reserveNeeded = 255 + data->len * 10 * 7 + (25*4+WINDOW_SIZE*10)*measurements.size();
    
    if(dest.capacity() - dest.size() < reserveNeeded) {
        dest.reserve(dest.size() + reserveNeeded);
    }

    dest.append("basis = '" + basisToString(data->basis) + "'\n");
    dest.append("seq_length = " + QString::number(data->len) + "\n");
    dest.append("functions = [");
    for(int j = 0 ; j < data->num_functions; j++){
        dest.append(QString::number( data->functions[j]));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);

//    qDebug() << "Clipboard String Capacity:  " << clip.capacity() << ", Size : " << clip.size();
    auto expected_data = ui->signalPlot->graph(1)->data();
    dest.append("]\nexpected_waveform = [");
    for(auto i = expected_data->begin(); i != expected_data->end(); i++) {
        dest.append(QString::number(i->value, 'f', 1));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);
    dest.append("]\nobserved_waveform = [");
    for(int i = 0; i < data->len; i++) {
        dest.append(QString::number(data->m_values[i], 'f', 1));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);
//    qDebug() << "Clipboard String Capacity:  " << clip.capacity() << ", Size : " << clip.size();
    dest.append("]\nfwht = [");
    for(int i = 0; i < data->len; i++) {
        dest.append(QString::number(data->transformWHT[i], 'f', 1));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);
    dest.append("]\nfft = [");
    for(int i = 0; i < data->len; i++) {
        dest.append(QString::number(data->transformFFT[i], 'g', 6));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);
//    clip.append("]\n");
//    qDebug() << "Clipboard String Capacity:  " << clip.capacity() << ", Size : " << clip.size();

    auto snr_data = signalGraph->data();
    dest.append("]\nsignal = [");
    for(auto i = snr_data->begin(); i != snr_data->end(); i++) {
        dest.append(QString::number(i->value, 'f', 1));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);

    snr_data = snrGraphMean->data();
    dest.append("]\nsignal_avg = [");
    for(auto i = snr_data->begin(); i != snr_data->end(); i++) {
        dest.append(QString::number(i->value, 'f', 2));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);

    snr_data = noiseGraph ->data();
    dest.append("]\nsignal_noise = [");
    for(auto i = snr_data->begin(); i != snr_data->end(); i++) {
        dest.append(QString::number(i->value, 'f', 2));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);

    snr_data = snrGraph ->data();
    dest.append("]\nsignal_snr = [");
    for(auto i = snr_data->begin(); i != snr_data->end(); i++) {
        dest.append(QString::number(i->value, 'f', 2));
        dest.append(", ");
    }
    dest.truncate(dest.size() - 2);


    if(measurements.size() > 0){
        dest.append("]\nmeasurements = [\n");
        for(int i = 0; i < measurements.size(); i++){
            if(measurements[i].id == i){
                dest.append("   { 'id': " + QString::number(measurements[i].id) + ",\n");
                dest.append("     'basis': '" + measurements[i].basis + "',\n");
                dest.append("     'seq_length': " + QString::number(measurements[i].seq_length) + ",\n");
                dest.append("     'functions': [");
                for(int j = 0 ; j < measurements[i].num_functions; j++){
                    dest.append(QString::number(measurements[i].functions[j]));
                    dest.append(", ");
                }
                dest.truncate(dest.size() - 2);
                dest.append("],\n");
                dest.append("     'mean': " + QString::number(measurements[i].mean, 'f', 3) + ",\n");
                dest.append("     'stdev': " + QString::number(measurements[i].stdev, 'f', 3) + ",\n");
                dest.append("     'settings': '" + measurements[i].settings + "',\n");
                dest.append("     'constituents': [");
                for(int j = 0 ; j < measurements[i].num_constituents; j++){
                    dest.append(QString::number(measurements[i].constituent_values[j], 'f', 1));
                    dest.append(", ");
                }
                dest.truncate(dest.size() - 2);
                dest.append("]\n   },\n");
            }
        }
        dest.truncate(dest.size() - 2);
        dest.append("\n");
    }


    dest.append("]\n");
    qDebug() << "Clipboard String Capacity:  " << dest.capacity() << ", Size : " << dest.size();

}

void MainWindow::onData(DataCollection *data)
//                    unsigned timestamp,
//                        const double *m_keys,
//                        const double *m_values,
//                        const double* transformWHT,
//                        const double* transformFFT,
//                        unsigned len)
                        {

//    QSurfaceDataRow *row = new QSurfaceDataRow(m_keys.size());
    int max3drows = 64; // should be equal to the sequence length
    int max3dcols = 64; // number of previous sequences to save
    unsigned len = data->len;
    double *m_keys = data->m_keys;
    double *m_values = data->m_values;
    double* transformWHT = data->transformWHT;
    double* transformFFT = data->transformFFT;

    const double srate = 200e3; // 200kHz - Hardcoded for now...
    const double srate_factor = 1e6/srate; // 200kHz == 5us per sample
    bin_width_khz = srate / (1e3*2*len); // up to Nyquist
    QSurfaceDataRow *row = new QSurfaceDataRow(max3drows);
    fCounter++;
    // plot the data here
    ui->signalPlot->graph(0)->data()->clear();
    for(int i = 0; i < len; i++){
        ui->signalPlot->graph(0)->addData(m_keys[i] * srate_factor, m_values[i]);
        if (i < max3drows) {
            (*row)[i] = QSurfaceDataItem(QVector3D(m_keys[i], m_values[i], fCounter));
        }
    }
    transformWHTGraph->data()->clear();
    for(int i = 0; i < len; i++){
        transformWHTGraph->addData(m_keys[i], transformWHT[i]);
    }

    transformFFTGraph->data()->clear();
//    for(int i = 0; i < len/2+1; i++){ // power-specttral density
    for(int i = 0; i < len; i++){ // PSD but splitting out the components...
        transformFFTGraph->addData(bin_width_khz*m_keys[i], transformFFT[i]);
    }

    //transformGraph->setData(m_keys, m_values, true);

    // update our 3d surface
    m_surface_data->append(row); //insert(0, row);
    while(m_surface_data->size() > max3dcols){
        QSurfaceDataRow *qsdr = m_surface_data->first();
        m_surface_data->removeFirst();
        delete qsdr;
    }

    timeSeriesOverlap->dataProxy()->resetArray(m_surface_data);

    double key = (data->timestamp)/1e6;

    signalGraph -> addData(key, data->signal_energy);

//    running_sum += data->signal_energy;
//    running_sq_sum += data->signal_energy * data->signal_energy;
//    running_count++;
//    double d1 = data->signal_energy - running_mean;
//    running_mean = running_mean + d1/(running_count);
//    double d2 = data->signal_energy - running_mean;
//    running_variance = running_variance + d1*d2;
//    float stdev = sqrt(running_variance/running_count);

    // full std-dev and mean calc (TODO: switch to incremental using Welford's algo above)
    auto s_data = signalGraph -> data();
    auto n_data = noiseGraph -> data();
    auto snr_data = snrGraph -> data();
    auto item = s_data->end();
//    outliers.clear();
    resetRunningStats();
    for(int i = 0;  i < WINDOW_SIZE; i++){
        item--;
        running_count++;
        running_sum += item->value;

        // TODO: check for weird outliers via stdev
//        if(running_stdev > 0 && item->value > 7*running_stdev){
//            outliers.add(item.value);
//        }

        if(item == s_data->begin()){
            break;
        }
    }
    running_mean = running_sum/running_count;
    item = s_data->end();
    for(int i = 0;  i < WINDOW_SIZE; i++){
        item--;
        running_sq_sum += (item->value - running_mean)*(item->value - running_mean);
        if(item == s_data->begin()){
            break;
        }
    }

    running_variance = running_sq_sum/running_count;
    running_stdev = sqrt(running_variance);

    bool found = false;
    QVector<int>bins(20);
    QCPRange vrange = signalGraph->getValueRange(found, QCP::sdBoth);
//    qDebug()<< "Noise range: (" << vrange.lower << ","<< vrange.upper << ")";
    double binwidth = vrange.size()/bins.size();
    for(auto i = s_data->begin();  i != s_data->end(); ++i){
        int idx = (int)(bins.size()*(i->value - vrange.lower)/vrange.size());
        if(idx == bins.size()) idx--;

        if(idx >= 0 && idx < bins.size()){
            bins[idx]++;
        } else {
            qDebug()<< "BAD bins[" << (int)(bins.size()*(i->value - vrange.lower)/vrange.size()) << "]++";
        }
    }
    noiseDistGraph->data()->clear();
    for(int i = 0; i < bins.size(); i++){
        noiseDistGraph->addData(vrange.lower+i*binwidth, bins[i]);
    }

    noiseDistGraph->setWidth(binwidth);
    if(running_mean > 0) {
        labelSignal->setText("Fl:   " + QString::number((int) running_mean) + " (" +
                             QString::number(20 * log10(running_mean), 'f', 1) + "dB)");
        labelSnr->setText("SNR:  " + QString::number(running_mean / running_stdev, 'f', 1) + "(" +
             QString::number(20 * log10(running_mean / running_stdev), 'f', 1) + " dB)");
        labelNoise->setText("σ(t):  " + QString::number(running_stdev, 'f', 1) + "(" +
             QString::number(20 * log10(running_stdev), 'f', 1) + " dB)");
    } else {
        labelSignal->setText("Fl:   " + QString::number((int) running_mean) + " (No Signal)");
        labelSnr->setText("SNR:   N/A");
        labelNoise->setText("σ(t):  N/A");
    }
    snrGraphMean -> addData(key, running_mean);
    snrGraphVarPos -> addData(key, running_mean+2*running_stdev);
    snrGraphVarNeg -> addData(key, running_mean-2*running_stdev);
    noiseGraph -> addData(key, running_stdev);
    if(running_stdev > 0) {
        snrGraph->addData(key, running_mean / running_stdev);
    } else {
        snrGraph->addData(key, 0);
    }

    // keep the graph at a constant length
    found = false;
    double range = 30;

    s_data->removeAfter(key);
    s_data->removeBefore(key - range*0.6);

    n_data->removeAfter(key);
    n_data->removeBefore(key - range*0.6);

    snr_data->removeAfter(key);
    snr_data->removeBefore(key - range*0.6);


//    while(snrGraph->getKeyRange(found, QCP::sdPositive).size() > range*0.6){
//        // remove this value from our running mean and variance
////        running_count --;
//        // https://math.stackexchange.com/questions/102978/incremental-computation-of-standard-deviation
//
//        s_data->remove(s_data->begin()->key);
//
//
//    }

//    qDebug() << "Current time range: (" << s_data->begin()->key << "," <<
//       s_data->end()->key << ") =" <<  s_data->end()->key - s_data->begin()->key;

    // display the last minute of data
//    snrPlot->xAxis->setRange(s_data->begin()->key, s_data->begin()->key + 60, Qt::AlignRight);

//    auto range = s_data->keyRange(found, QCP::sdPositive);
//    if(range.upper - range.lower < 60){
//        snrPlot->xAxis->setRange(range.lower, range.lower + 60);
//    } else {
//        snrPlot->xAxis->setRange(range.lower, range.lower + 60);
//    }
    double lower = s_data->begin()->key;
    ui->snrPlot->xAxis->setRange(lower, lower + range);

    if(requestMeasurement >= 0){
        auto m_data = measurementGraphMajor->data();

        if(requestMeasurement+1 > measurements.size()){
            measurements.resize(requestMeasurement+1);
        }

        if(measurements[requestMeasurement].id > -1){
            qDebug() << "WARNING: overwriting previous measurement " << requestMeasurement;
        }

        measurements[requestMeasurement].id = requestMeasurement;
        measurements[requestMeasurement].mean = running_mean;
        measurements[requestMeasurement].stdev = running_stdev;
        measurements[requestMeasurement].num_constituents = 0;
        measurements[requestMeasurement].settings.truncate(0);
//        QMetaEnum::fromType<Ui::BasisFunctions>().valueToKey(Ui::BasisFunctions::Fourier)
        measurements[requestMeasurement].basis = basisToString(data->basis);
        measurements[requestMeasurement].seq_length = data->len;
        measurements[requestMeasurement].num_functions = data->num_functions;
        for(int i = 0; i < data->num_functions && i < MAX_FUNCTIONS; i++){
            measurements[requestMeasurement].functions[i] =data->functions[i];
        }


        waveformSettingsToString(appSettings, measurements[requestMeasurement].settings);
        auto d = s_data->end();
        for(int i = 0; i < WINDOW_SIZE; i++){
            d--;
            // randomly add 0.1 to spread out
            measurements[requestMeasurement].constituent_keys[i] = requestMeasurement+
                QRandomGenerator::global()->bounded(0.2) - 0.1;
            measurements[requestMeasurement].constituent_values[i] = d->value;
            measurements[requestMeasurement].num_constituents++;
            if(d == s_data->begin()) {
                break;
            }
        }

        measurementGraphMajor->data()->clear();
        measurementGraphMinor->data()->clear();
        errorBars->data()->clear();
        for(int i = 0; i < measurements.size(); i++){
            if(measurements[i].id == i){
                measurementGraphMajor->addData(i,measurements[i].mean);
                errorBars->addData(measurements[i].stdev);
                for(int j = 0; j < measurements[i].num_constituents; j++){
                    measurementGraphMinor->addData(measurements[i].constituent_keys[j],
                                                   measurements[i].constituent_values[j]);
                }
            }
        }
        requestMeasurement = -1;
    }




    static double max;
    static double min;
    if(blAutoScale){
//        qDebug("checking if rescale needed");
//        qDebug() << "!m_elapsed.isValid() || m_elapsed.elapsed() > 10000" << !m_elapsed.isValid() << (m_elapsed.elapsed() > 10000);
        // if the timer was never started or
        if(!m_elapsed.isValid() || m_elapsed.elapsed() > 3000){
            //qDebug() << "Rescaling...";
//            plot->xAxis->setRange(0, m_keys.size());
//            plot->yAxis->setRange(0, 100);
            //QCustomPlot::rescaleAxes
//            qDebug("Rescaling Axes");


            //rescaleAxes(); <== don't rescale yAxis2
            ui->signalPlot->xAxis->rescale();
            ui->signalPlot->yAxis->rescale();

            // debug FOURIER
//            plot->yAxis2->rescale();

            // need a special scalnig for the WHT because the DC term is large...
            double max = std::numeric_limits<double>::min();
            double min = std::numeric_limits<double>::max();
            //for(auto i = m_transform.begin()+1; i<m_transform.end(); i++){
            for(int i = 1; i<len; i++){
                if(transformWHT[i] > max){
                    max = transformWHT[i];
                }
                if(transformWHT[i] < min){
                    min = transformWHT[i];
                }
            }
            whAxisRect->axis(QCPAxis::atLeft, 0) -> setRange( min, max);

            max = std::numeric_limits<double>::min();
            min = std::numeric_limits<double>::max();
            //for(auto i = m_transform.begin()+1; i<m_transform.end(); i++){
            for(int i = 1; i<len; i++){
                if(transformFFT[i] > max){
                    max = transformFFT[i];
                }
                if(transformFFT[i] < min){
                    min = transformFFT[i];
                }
            }
            fftAxisRect->axis(QCPAxis::atLeft, 0) -> setRange( min, max);


            whAxisRect->axis(QCPAxis::atBottom, 0) -> rescale();

            noiseAxisRect->axis(QCPAxis::atBottom, 0) -> rescale();
            noiseAxisRect->axis(QCPAxis::atLeft, 0) -> rescale();

            //bool found = false;
//            s_data->valueRange(found, QCP::sdPositive);
            ui->snrPlot->yAxis->setRange(vrange.lower*0.85, vrange.upper * 1.25);
            // in the lower 20% of the graph..
            double noiseRange = noiseGraph->getValueRange(found, QCP::sdBoth).upper * 5;
            double snrRange = snrGraph->getValueRange(found, QCP::sdBoth).upper * 5;
            ui->snrPlot->yAxis2->setRange(0, std::max(noiseRange,snrRange));

//            qDebug() << "noiseRange: " << noiseRange;
//            qDebug() << "snrRange: " << snrRange;
//            qDebug() << "max: " << std::max(noiseRange,snrRange);

            if(measurements.size() > ui->snrPlot->xAxis2->range().upper) {
                ui->snrPlot->xAxis2->setRange(0,measurements.size() + 1);
            } else if(measurements.size() < 10) {
                ui->snrPlot->xAxis2->setRange(0,10);
            }

            m_elapsed.start();
        }
    }


    // hack: get the max value from each plot in order to align the labels
    // too many cases to be reliable - decimal places, negative signs, etc
//    QCPRange whRange = whAxisRect->axis(QCPAxis::atLeft)->range();
//    QCPRange dataRange = plot->yAxis->range();
//    int whDigits = abs(log10(whRange.upper));
//    int dataDigits = abs(log10(dataRange.upper));
//    if(whRange.lower < 0 ) { whDigits++;}   // minus sign takes up an extra space
//    if(dataRange.lower < 0 ) { dataDigits++;}

//    qDebug() << "whRange" << whAxisRect->axis(QCPAxis::atLeft)->range().upper;
//    qDebug() << "dataRange" << plot->yAxis->range().upper;
//    whAxisRect->axis(QCPAxis::atLeft)->setLabelPadding(10);
//    plot->yAxis->setLabelPadding(25);

    // plot->yAxis->setLabelPadding(10);

    ui->signalPlot->replot();
    ui->snrPlot->replot();
    if(copyRequested){
        doCopyToClipboard(data);
        copyRequested = false;
    }
    if(screenshotRequested){
        doSaveScreenshot(data);
        screenshotRequested = false;
    }
}

void MainWindow::handleConnectionRecovered() {
    //plot->rescaleAxes();
}


MainWindow::~MainWindow()
{
    for(int i = m_surface_data->size() - 1; i >= 0; i--){
        delete m_surface_data->at(i);
    }
    m_surface_data->clear();
    delete ui;
}

//void MainWindow::onNewWaveform() {
//    int order = 9;
//    m_list.clear();
//    for(int i = 0; i < 2; i++) {
////        m_list.append(QRandomGenerator::global()->bounded(1<<order));
//        m_list.append(QRandomGenerator::global()->bounded(20));
//    }
//    // order 9 = 512 bytes
//    if(serialConnected) {
//        serialThread->requestNewWaveform(m_list, order);
//    }
//
//    // add custom plottables to our fwt and time series plots showing the "expecte
//}

void MainWindow::onWaveformDesignerToggled(bool visible) {
    if(visible){


    } else {

    }

    ui->widgetDesigner->setVisible(visible);
    ui->centralwidget->updateGeometry();

}

void MainWindow::updateWaveformUIEnabled() {
    ui->spinHopFreq->setEnabled(ui->radioModeChirp->isChecked());
    ui->spinNoiseInterval->setEnabled(ui->radioModeAdaptive->isChecked());
    ui->textCustomFunctions->setEnabled(ui->radioModeCustom->isChecked());

    ui->comboBoxNumFunctions->setEnabled(!ui->radioModeCustom->isChecked());
    ui->spinRangeEnd->setEnabled(!ui->radioModeCustom->isChecked());
    ui->spinRangeStart->setEnabled(!ui->radioModeCustom->isChecked());
}

void MainWindow::onWaveformDesignerChanged() {
    if(!waveformDesignerLoading){
        updateWaveformUIEnabled();
        if(!ui->textCustomFunctions->hasAcceptableInput()){
            ui->textCustomFunctions->setStyleSheet("color: red");
            ui->buttonApply->setEnabled(false);
        }
        else {
            ui->textCustomFunctions->setStyleSheet("");
            ui->buttonApply->setEnabled(true);
        }
        ui->buttonReset->setEnabled(true);
        ui->buttonClose->setText("Cancel");
        waveformDesignerChanged = true;
    }
}

void MainWindow::onWaveformLengthChanged(QString newText) {
    if(!uiLoading) {
        int newVal = newText.split(' ')[0].toInt();
        basisValidator->setRange(newVal);
    }
}

void MainWindow::onWaveformDesignerApply() {
    saveWaveformSettings();

    // send the new settings to the serial thread...
    //if(serialThread.
    if(serialConnected) {
        serialThread->applyWaveformSettings(appSettings);
    }
}

void MainWindow::onWaveformDesignerCancelClose() {
    ui->actionWaveformDesigner->setChecked(false);

    // load the existing settings
    loadWaveformSettings();

    // hide the designer
    onWaveformDesignerToggled(false);
}


void MainWindow::onWaveformDesignerDefault() {
//    qDebug() << "Loading defaults";
    loadWaveformDefaults();
    loadWaveformSettings();
    onWaveformDesignerChanged();
}

void MainWindow::SetupWaveformDesigner() {
//    radioModeAdaptive = findChild<QRadioButton *>("radioModeAdaptive");
//    radioModeCustom = findChild<QRadioButton *>("radioModeCustom");
//    radioModeChirp = findChild<QRadioButton *>("radioModeChirp");
//    comboWaveformLength = findChild<QComboBox *>("comboWaveformLength");
//    comboBoxNumFunctions = findChild<QComboBox *>("comboBoxNumFunctions");
//    textCustomFunctions = findChild<QLineEdit *>("textCustomFunctions");
//    radioWalsh = findChild<QRadioButton *>("radioWalsh");
//    radioFourier = findChild<QRadioButton *>("radioFourier");
//    spinRangeStart = findChild<QSpinBox *>("spinRangeStart");
//    spinRangeEnd = findChild<QSpinBox *>("spinRangeEnd");
//    buttonApply = findChild<QPushButton *>("buttonApply");
//    buttonReset = findChild<QPushButton *>("buttonReset");
//    buttonClose = findChild<QPushButton *>("buttonClose");
//    spinNoiseInterval = findChild<QSpinBox *>("spinNoiseInterval");
//    spinHopFreq = findChild<QSpinBox *>("spinHopFreq");
    basisValidator = new BasisValidator();
    ui->textCustomFunctions->setValidator(basisValidator);

    if(!appSettings.contains(KEY_MODE)) {
        // we don't have any saved configuration, load up the defaults
        loadWaveformDefaults();
    }
    loadWaveformSettings();

    // A proper data model is not worth it for a small project like this...
//    mapper = new NamedWidgetMapper(this);
//    model = new QStandardItemModel(1, 14, this);
//    mapper->setModel(model);
//
//    // map the widgets onto a named tabular model...
//    mapper->addNamedMapping(radioModeAdaptive,"ModeAdaptive");
//    mapper->addNamedMapping(radioModeCustom,"ModeCustom");
//    mapper->addNamedMapping(radioModeChirp,"ModeChirp");
//    mapper->addNamedMapping(comboWaveformLength,"WaveformLength", "currentText");
//    mapper->addNamedMapping(comboBoxNumFunctions,"BoxNumFunctions", "currentText");
//    mapper->addNamedMapping(textCustomFunctions,"CustomFunctions");
//    mapper->addNamedMapping(radioWalsh,"Walsh");
//    mapper->addNamedMapping(radioFourier,"Fourier");
//    mapper->addNamedMapping(spinRangeStart,"RangeStart");
//    mapper->addNamedMapping(spinRangeEnd,"RangeEnd");
//    mapper->addNamedMapping(buttonApply,"Apply");
//    mapper->addNamedMapping(buttonClose,"Close");
//    mapper->addNamedMapping(spinNoiseInterval,"NoiseInterval");
//    mapper->addNamedMapping(spinHopFreq,"HopFreq");
}

void MainWindow::onWaveformDesignerReset() {
    loadWaveformSettings();
}


void MainWindow::loadWaveformDefaults() {
    appSettings.setValue(KEY_MODE, VAL_MODE_CUSTOM);

    appSettings.setValue(KEY_BASIS, VAL_BASIS_WALSH);

    appSettings.setValue(KEY_RANGE_START, 0);
    appSettings.setValue(KEY_RANGE_END, 512);

    appSettings.setValue(KEY_NOISE_INTERVAL, 250);
    appSettings.setValue(KEY_HOP_FREQ, 250);

    appSettings.setValue(KEY_WAVEFORM_LENGTH, "512 (Default)");
    appSettings.setValue(KEY_NUM_FUNCTIONS, "3");

    appSettings.setValue(KEY_CUSTOM_FUNCTIONS, "3,  7, 12, 15, 21, 27");
}

void MainWindow::loadWaveformSettings() {
    waveformDesignerLoading = true;

    // reload from settings...
    QString mode = appSettings.value(KEY_MODE, VAL_MODE_CUSTOM).toString();
    QString basis = appSettings.value(KEY_BASIS, VAL_BASIS_WALSH).toString();

    if(QString::compare(mode, VAL_MODE_CUSTOM, Qt::CaseInsensitive) == 0){
        ui->radioModeCustom->setChecked(true);
    } else if(QString::compare(mode, VAL_MODE_ADAPTIVE, Qt::CaseInsensitive) == 0){
        ui->radioModeAdaptive->setChecked(true);
    }  else if(QString::compare(mode, VAL_MODE_CHIRP, Qt::CaseInsensitive) == 0){
        ui->radioModeChirp->setChecked(true);
    }

    if(QString::compare(basis, VAL_BASIS_WALSH, Qt::CaseInsensitive) == 0){
        ui->radioWalsh->setChecked(true);
    } else if(QString::compare(basis, VAL_BASIS_FOURIER, Qt::CaseInsensitive) == 0){
        ui->radioFourier->setChecked(true);
    }

    ui->spinRangeStart->setValue(appSettings.value(KEY_RANGE_START).toInt());
    ui->spinRangeEnd->setValue(appSettings.value(KEY_RANGE_END).toInt());

    ui->spinNoiseInterval->setValue(appSettings.value(KEY_NOISE_INTERVAL).toInt());
    ui->spinHopFreq->setValue(appSettings.value(KEY_HOP_FREQ).toInt());

    QString waveformLen = appSettings.value(KEY_WAVEFORM_LENGTH).toString();
    ui->comboWaveformLength->setCurrentText(waveformLen);
    ui->comboBoxNumFunctions->setCurrentText(appSettings.value(KEY_NUM_FUNCTIONS).toString());

    ui->textCustomFunctions->setText(appSettings.value(KEY_CUSTOM_FUNCTIONS).toString());
    basisValidator->setRange(waveformLen.split(' ')[0].toInt());

    ui->buttonApply->setEnabled(false);
    ui->buttonReset->setEnabled(false);
    ui->buttonClose->setText("Close");

    waveformDesignerLoading = false;
    updateWaveformUIEnabled();
}

void MainWindow::saveWaveformSettings() {
    appSettings.setValue(KEY_MODE, ui->radioModeAdaptive->isChecked() ? VAL_MODE_ADAPTIVE :
                                   ui->radioModeCustom->isChecked() ? VAL_MODE_CUSTOM :
                                   ui->radioModeChirp->isChecked() ? VAL_MODE_CHIRP : nullptr);

    appSettings.setValue(KEY_BASIS, ui->radioFourier->isChecked() ? VAL_BASIS_FOURIER :
                                    ui->radioWalsh->isChecked() ? VAL_BASIS_WALSH : nullptr);

    appSettings.setValue(KEY_RANGE_START, ui->spinRangeStart->value());
    appSettings.setValue(KEY_RANGE_END, ui->spinRangeEnd->value());

    appSettings.setValue(KEY_NOISE_INTERVAL, ui->spinNoiseInterval->value());
    appSettings.setValue(KEY_HOP_FREQ, ui->spinHopFreq->value());

    appSettings.setValue(KEY_WAVEFORM_LENGTH, ui->comboWaveformLength->currentText());
    appSettings.setValue(KEY_NUM_FUNCTIONS, ui->comboBoxNumFunctions->currentText());

    appSettings.setValue(KEY_CUSTOM_FUNCTIONS, ui->textCustomFunctions->text());

    ui->buttonApply->setEnabled(false);
    ui->buttonReset->setEnabled(false);
    ui->buttonClose->setText("Close");

    waveformDesignerChanged = false;

    // debug print....
//    qDebug() << "Saved settings: ";
//    foreach (const QString &key, waveformSettings.allKeys()) {
//        qDebug() << key << ": " << waveformSettings.value(key);
//    }
}

void MainWindow::onSerialPortRefreshRequested() {
    InitPorts();
}

void MainWindow::handleWaveformUpdate(double *waveform, double *walsh,
     unsigned int len, Ui::BasisFunctions basis, Ui::WaveformMode mode) {
    // paint an expected waveform on the QCustomplot
    double srate_factor = 1e6/200000; // 200kHz == 5us per sample
    // plot the data here
    ui->signalPlot->graph(1)->data()->clear();
    if(basis == Ui::BasisFunctions::Fourier) {
        ui->signalPlot->graph(1)->setLineStyle(QCPGraph::lsLine);
    }else if(basis == Ui::BasisFunctions::Walsh) {
        ui->signalPlot->graph(1)->setLineStyle(QCPGraph::lsStepCenter);
    }
    for(int i = 0; i < len; i++){
        ui->signalPlot->graph(1)->addData(i * srate_factor, waveform[i]);
    }
    ui->signalPlot->replot();
    if(ui->actionAutoMeasure->isChecked()){
        // take a measurement in 25ms (give a few moments to allow the waveform to
        // transfer to MCU, propagate back to the UI, and settle (60 averages)
        QTimer::singleShot(3500, this, [this]() {
            this->requestMeasurement = this->measurements.size();
        });
    }
}

bool IconPaintFilter::eventFilter(QObject *watched, QEvent *event)
{
    if( event->type() == QEvent::Paint)
    {
        QPaintEvent *evt =  (QPaintEvent *)event;
        QRadioButton * radio = dynamic_cast<QRadioButton*>(watched);
        //QPicture
        QPainter painter(radio);
//        radio->style()->dr
//        painter.begin()
//        QPixmap pixmap = radio->pixmap()->scaled(radio->size());
//        radio->style()->drawItemPixmap(&painter, radio->rect(), Qt::AlignHCenter | Qt::AlignVCenter, pixmap);

        return true;
    }
    return false;
}

void MainWindow::doSaveScreenshot(DataCollection *data) {
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString time = QDateTime::currentDateTime().toString("hh-mm-ss");
    QString filepath = R"(C:\dev\prbs\plots\)" + date + R"(-UI-Screenshots\)" + time  + "\\";

    if (QDir::root().mkpath(filepath)) {
        // save the direct bitmap
        QPixmap pixmap = grab();//filepath + time + "-direct.png");
        pixmap.save(filepath +  "direct.png");

        int width = 400;
        int height = 225;
        // save the 2d plot in vectorized form
        ui->signalPlot->savePdf(filepath + "2d-signal.pdf", width, height*3);
        ui->snrPlot->savePdf(filepath + "2d-snr.pdf", width*2, height*2);
        ui->signalPlot->savePng(filepath + "2d-signal.png", width, height*3, 2.0, 100);
        ui->snrPlot->savePng(filepath + "2d-snr.png", width*2, height*2, 2.0, 100);



//    QCPPainter qcpPainter;
//    qcpPainter.begin(&pic);
//    plot->toPainter(&qcpPainter, plot->width(), plot->height());
//    qcpPainter.end();

        // save a large version of the 3d plot
        QSize surfaceSize = surface->size();
        float aspect = surfaceSize.height()/surfaceSize.width();
        QImage image = surface->renderToImage(8, QSize(2048,aspect*2048));
//        qDebug() << "Writing 3d surface" << surfaceSize << "to: " << filepath + "3d-plot.png";
        image.save(filepath + "3d-plot.png");

        //save the full UI in partially vectorized form

        // First render the widget to a QPicture, which stores QPainter commands.
        QPicture pic;
        QPainter picPainter;
        picPainter.begin(&pic);
        picPainter.setRenderHint(QPainter::LosslessImageRendering);
//        picPainter.
        this->render(&picPainter);
        picPainter.end();

        // Set up the printer
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filepath + "ui-full.pdf");

        // Finally, draw the QPicture to your printer
        QPainter painter;
        painter.begin(&printer);
        painter.drawPicture(QPointF(0, 0), pic);
        painter.end();

        // save the text data
        QFile dataFile(filepath+"data.txt");
        if( !dataFile.open(QIODevice::WriteOnly) )
        {
            QString msg = QString( tr("Unable to save data to file: %1") ).arg(filepath+"data.txt");
            QMessageBox::critical(this, tr("HPV Analysis Tool"), msg, QMessageBox::Ok );
            return;
        }
        QTextStream outputStream(&dataFile);
        QString dataStr;
        doDataSnapshot(data, dataStr);
        outputStream << dataStr;
        dataFile.close();
    }
    // is vectorized but looks extremely ugly...
//    QSvgGenerator generator;
//    generator.setFileName(R"(C:\dev\prbs\plots\2021-06-27-WaveformDesigner\test-.svg)");
//    generator.setSize(this->size());
//    generator.setViewBox(this->rect());
//    generator.setTitle(tr("HPV UI"));
//    generator.setDescription(tr("Vectorized Screenshot SVG"));
//    this->render(&generator);
//
//    generator.setFileName(R"(C:\dev\prbs\plots\2021-06-27-WaveformDesigner\test-.svg)");
//
//
//    QCPPainter qcpPainter;
//    qcpPainter.begin(&picture);
//    plot->toPainter(&qcpPainter, width, height);
//    qcpPainter.end();


    /// LOOKS GREAT! (But no "rest of the UI")

//    this->radioModeCustom->removeEventFilter(iconFilter);



}

void MainWindow::resetRunningStats() {
    running_count = running_mean = running_variance = running_stdev = 0;
    running_sum = running_sq_sum = 0;
}

void MainWindow::onLogScaleChanged(bool useLogScale) {
    if(useLogScale){
        fftAxisRect->axis(QCPAxis::atLeft,0)->setScaleType(QCPAxis::stLogarithmic);
        whAxisRect->axis(QCPAxis::atLeft,0)->setScaleType(QCPAxis::stLogarithmic);
    } else {
        fftAxisRect->axis(QCPAxis::atLeft,0)->setScaleType(QCPAxis::stLinear);
        whAxisRect->axis(QCPAxis::atLeft,0)->setScaleType(QCPAxis::stLinear);
    }
}

void MainWindow::settingsToString(QSettings &settings, QString &dest) {

    auto keys = settings.allKeys();
    for(const auto &i : keys){
        dest.append(i + ": " + settings.value(i).toString() + "\n");
    }
    dest.append('\n');
}

void MainWindow::waveformSettingsToString(QSettings &settings, QString &dest) {
    for(const auto &i : WAVEFORM_SETTINGS){
        dest.append(QString(i) + ": " + settings.value(i).toString() + ", ");
    }
    dest.truncate(dest.size()-2);
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
//    qDebug() << "You Pressed Key: " << event->key();
    if(event->key() == Qt::Key_Control)
    {
        ctrlDown = true;
    }
    else if(ctrlDown && event->key() >= 0x30 && event->key() <= 0x39){
        experimentNumber.append(event->key() - 0x30);
//        qDebug() << "Appended: " << event->key() - 0x30;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
//   qDebug() << "You Released Key: " << event->key();
    if(event->key() == Qt::Key_Control)
    {
        if(experimentNumber.size() > 0){
            int num = 0;
            int multiple = 1;
            for (int i = experimentNumber.size()-1; i >= 0; i--) {
                num += multiple * experimentNumber[i];
                multiple *= 10;
            }
            experimentNumber.clear();
            qDebug() << "Setting Experiment " << num;
            requestMeasurement = num;

        }
        ctrlDown = false;
    }
}

void MainWindow::onRequestScreenshot() {
    screenshotRequested = true;
}

QString MainWindow::basisToString(Ui::BasisFunctions basis) {
    return basis == Ui::BasisFunctions::Fourier ? "Fourier" :
           basis == Ui::BasisFunctions::Walsh ? "Walsh" : "Unknown";
}

QString MainWindow::modeToString(Ui::WaveformMode mode) {
    return mode == Ui::WaveformMode::Custom ? "Custom"  :
    mode == Ui::WaveformMode::Adaptive ? "Adaptive"  :
    mode == Ui::WaveformMode::Chirp ? "Chirp" : "Unknown";
}

