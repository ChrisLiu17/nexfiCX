#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QBitArray>

#include "console.h"

enum {
    LOGIN_TAB,
    NETWORK_TAB,
    WIRELESS_TAB,
    ADV_TAB
};

enum {
    DUIP,
    DAOCNDI,
    DRPS,
    DAPI,
    DDTC,
    NEXT_REQ_MAX,
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void openCommon();
    void closeCommon();
    void nextRequest();
    void tryEnableSaveBtn();
    void readData(QByteArray data);

private slots:
    void on_tabWidget_tabBarClicked(int index);

    void on_comboBox_currentIndexChanged(int index);

    void on_openButton_clicked();

    /* by hand */
    void serialRead();
    void netRead(QNetworkReply *reply);
    void writeData(QByteArray data);
//    void requestFinished(QNetworkReply *reply);

    void on_ipSetBtn_clicked();

    void on_refreshBtn_clicked();

    void on_bandComboBox_currentIndexChanged(int index);

    void on_bandwidthComboBox_currentIndexChanged(int index);

    void on_spinBox_valueChanged(int arg1);

    void on_saveBtn_clicked();

    void on_openNetBtn_clicked();

    void on_ipLineEdit_2_textEdited(const QString &arg1);

    void on_vlanLineEdit_textEdited(const QString &arg1);

    void on_advSaveBtn_clicked();

    void on_advSendBtn_clicked();

    void on_advAutoComboBox_currentIndexChanged(int index);

    void on_refreshAction_triggered();

private:
    Ui::MainWindow *ui;
    QSerialPort *m_serial;
    QNetworkAccessManager *m_net;
    QString m_netIP;
    bool m_isNet;
    QBitArray *m_nextBitArray;

    QString m_key = nullptr;
    int m_band_index = -1;
    QByteArrayList m_band = {"01", "04", "08"};
    int m_bandwidth = -1;
    int m_txpower = -1;

    int m_advAuto = -1;
    int m_advMode = -1;
    Console *m_console;
};
#endif // MAINWINDOW_H
