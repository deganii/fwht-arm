#include "mainwindow.h"

#include <QApplication>
#include "constants.h"

Q_DECLARE_METATYPE(QVector<float>);
Q_DECLARE_METATYPE(quint32_le);
Q_DECLARE_METATYPE(double *);
Q_DECLARE_METATYPE(QList<int>);
Q_DECLARE_METATYPE(Ui::BasisFunctions);
Q_DECLARE_METATYPE(DataCollection *);

int main(int argc, char *argv[])
{

    //qputenv("QSG_INFO", "1");
    //QGuiApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Massachusetts General Hospital");
    QCoreApplication::setApplicationName("HPV Fluorescence");



//    QFile file(":/themes/dark_theme.qss");
//    QFile file(":/themes/qdarkstyle.qss");
//    file.open(QFile::ReadOnly);
//    QString styleSheet = QString(file.readAll());
//    a.setStyleSheet(styleSheet);
//    a.setStyle(new QPlastiqueStyle());




    QLoggingCategory::setFilterRules("qt.scenegraph.general=true");
    qRegisterMetaType<QVector<float>>();
    qRegisterMetaType<QVector<double>>();
    qRegisterMetaType<QList<int>>();
    qRegisterMetaType<double *>();
    qRegisterMetaType<quint32_le>();
    qRegisterMetaType<Ui::BasisFunctions>();
    qRegisterMetaType<Ui::WaveformMode>();

    qRegisterMetaType<DataCollection *>();



    MainWindow w;
//    w.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    w.show();
    return a.exec();
}
