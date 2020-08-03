#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"

//for system Sleep function, not use here
#ifdef Q_OS_MAC
// mac
#endif

#ifdef Q_OS_LINUX
#include <sys/stat.h>
#endif

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_serial(new QSerialPort(this))
    , m_net(new QNetworkAccessManager(this))
    , m_isNet(false)
{
    ui->setupUi(this);

    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos)
        ui->serialPortComboBox->addItem(info.portName());

    ui->tabWidget->setTabEnabled(1, false);
    ui->tabWidget->setTabEnabled(2, false);
    ui->tabWidget->setTabEnabled(3, false);

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QRegExp ipRegex ("^" + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange
                     + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);
    ui->ipLineEdit->setValidator(ipValidator);
    ui->ipLineEdit_2->setValidator(ipValidator);

    m_console = new Console();
    ui->gridLayout_2->addWidget(m_console, 0, 1, 1, 1);

    setWindowTitle("  NexFi自组网配置管理软件");
    qInfo() << this->font();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    qInfo("tab {%d} clicked", index);
}

void MainWindow::on_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
}

void MainWindow::openCommon()
{
    writeData("AT+CFUN=0");
    ui->ipSetBtn->setEnabled(false);
    ui->tabWidget->setTabEnabled(NETWORK_TAB, true);
    ui->tabWidget->setTabEnabled(WIRELESS_TAB, true);
    ui->tabWidget->setTabEnabled(ADV_TAB, true);
    ui->refreshAction->setEnabled(true);
    m_nextBitArray = new QBitArray(NEXT_REQ_MAX, 1);
}

void MainWindow::closeCommon()
{
    writeData("AT+CFUN=1");
    ui->tabWidget->setTabEnabled(NETWORK_TAB, false);
    ui->tabWidget->setTabEnabled(WIRELESS_TAB, false);
    ui->refreshAction->setEnabled(false);
    delete m_nextBitArray;
}

void MainWindow::on_openButton_clicked()
{
    auto portName = ui->serialPortComboBox->currentText();

    if (m_serial->isOpen()) {
        m_serial->close();
        ui->openButton->setText(tr("打开串口"));
        ui->statusbar->showMessage(tr("%1 已关闭").arg(portName));
        ui->openNetBtn->setEnabled(true);

        disconnect(m_serial, &QSerialPort::readyRead, this, &MainWindow::serialRead);
        closeCommon();
        return;
    }

    m_serial->setPortName(portName);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {

        ui->openButton->setText(tr("关闭串口"));
        ui->openNetBtn->setEnabled(false);
        ui->statusbar->showMessage(tr("已打开, 无数据;"));

        m_isNet = false;
        connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::serialRead);

        openCommon();
    } else {
        ui->statusbar->showMessage(tr("Can't open %1, error code %2")
                                   .arg(portName)
                                   .arg(m_serial->error()));
    }
}

void MainWindow::on_openNetBtn_clicked()
{
    m_netIP = ui->ipLineEdit->text();
    if (m_isNet) {
        m_isNet = false;
        disconnect(m_net, &QNetworkAccessManager::finished, this, &MainWindow::netRead);
        ui->openNetBtn->setText("打开网络");
        ui->openButton->setEnabled(true);
        closeCommon();
    } else {
        m_isNet = true;
        connect(m_net, &QNetworkAccessManager::finished, this, &MainWindow::netRead);
        ui->openNetBtn->setText("关闭网络");
        ui->openButton->setEnabled(false);
        openCommon();
    }
}

void MainWindow::netRead(QNetworkReply *reply)
{
    // 获取http状态码
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if(statusCode.isValid())
        qDebug() << "status code=" << statusCode.toInt();//200

    QVariant reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    if(reason.isValid())
        qDebug() << "reason=" << reason.toString();//"OK"

    QNetworkReply::NetworkError err = reply->error();
    if(err != QNetworkReply::NoError) {
        qDebug() << "Failed: " << reply->errorString();
        ui->statusbar->showMessage("网络错误;");
        ui->openNetBtn->click();//关闭
    }
    else {
        while (reply->canReadLine()) {
            readData(reply->readLine());
        }
    }
}

void MainWindow::serialRead()
{
    //确保是整行读取
    while (m_serial->canReadLine()) {
        char line[256];
        m_serial->readLine(line, 256);
        readData(QByteArray(line));
    }
}

void MainWindow::readData(QByteArray data)
{
    qInfo() << data;
    m_console->putData(data);
    if (data.contains("DUIP")) {
        m_netIP = data.split('"')[1];//192.168.10.50
        ui->ipLineEdit->setText(m_netIP);
        ui->ipLineEdit_2->setText(m_netIP);
        ui->ipLineEdit_2->setEnabled(true);
        m_nextBitArray->clearBit(DUIP);
    } else if (data.contains("DAPI")) {
        //接入密钥
        m_key = QString(data.split('"')[1]);
        ui->vlanLineEdit->setText(m_key);

        ui->vlanLineEdit->setCursorPosition(0);
        ui->vlanLineEdit->setEnabled(true);
        m_nextBitArray->clearBit(DAPI);
    } else if (data.contains("DAOCNDI")) {
        /*
         * AT^DAOCNDI?
         * ^DAOCNDI: 0004  OK
         * 频段 //08表示 2.4G/ 04表示 1.4G/ 01表示 800M
        */
        if (data.contains("01")) {
            m_band_index = 0;
        } else if (data.contains("04")) {
            m_band_index = 1;
        } else if (data.contains("08")) {
            m_band_index = 2;
        }
        ui->bandComboBox->setCurrentIndex(m_band_index);
        ui->bandComboBox->setEnabled(true);
        m_nextBitArray->clearBit(DAOCNDI);
    } else if (data.contains("DRPS")) {
        auto three = data.split(' ')[1].split(',');
        if (three.length() < 3) {
            qCritical("DRPS should be three, but less");
        }
        m_bandwidth = three[1].toInt();
        ui->bandwidthComboBox->setCurrentIndex(m_bandwidth);

        m_txpower = three[2].split('"')[1].toInt();//"15" - > 15
        ui->spinBox->setValue(m_txpower);

        ui->bandwidthComboBox->setEnabled(true);
        ui->spinBox->setEnabled(true);
        m_nextBitArray->clearBit(DRPS);
    } else if (data.contains("DDTC")) {//^DDTC: 0,0
        data.replace(':', ',');
        m_advAuto = data.split(',')[1].toInt();
        ui->advAutoComboBox->setCurrentIndex(m_advAuto);
        ui->advAutoComboBox->setEnabled(true);
        ui->advSaveBtn->setEnabled(true);
        m_nextBitArray->clearBit(DDTC);
    } else if (data.contains("OK")) {
        ui->statusbar->showMessage(tr("OK"));
        return;
    } else if (data.contains("ERROR")) {
        ui->statusbar->showMessage(tr("ERROR"));
        return;
    }

    nextRequest();
}

void MainWindow::writeData(QByteArray data)
{
    if (m_isNet) {
        //"\r\n^DUIP: 0,\"192.168.11.11\",8CFF5F00,\"00:01:00:5f:ff:8c\",9250353\r\n\r\nOK\r\n"
        QNetworkRequest request;
//        request.setTransferTimeout(1000);//This function was introduced in Qt 5.15.
        request.setUrl(QUrl(tr("http://%1/boafrm/formAtcmdProcess").arg(m_netIP)));
        m_net->post(request, "FormAtcmd_Param_Atcmd="+data);
    } else {
        m_serial->write(data.append('\r'));
    }
}

void MainWindow::nextRequest()
{
    QByteArray req;
    int var;
    for (var = 0; var < NEXT_REQ_MAX; ++var) {
        if (m_nextBitArray->testBit(var))
            break;
    }
    switch (var) {
    case DUIP:
        req = "AT^DUIP?";
        break;
    case DAOCNDI:
        req = "AT^DAOCNDI?";
        break;
    case DRPS:
        req = "AT^DRPS?";
        break;
    case DAPI:
        req = "AT^DAPI?";
        break;
    case DDTC://接入类型
        req = "AT^DDTC?";
        break;
    default:
        qInfo() << "Request done";
        return;
    }
    qInfo() << "nextRequest" << req;
    writeData(req);
}

void MainWindow::on_ipSetBtn_clicked()
{
    auto cmd = ui->ipLineEdit_2->text();
    writeData(cmd.toLocal8Bit());
}

void MainWindow::tryEnableSaveBtn()
{

    if ((m_key == nullptr)
            || (m_band_index == -1)
            || (m_bandwidth == -1)
            || (m_txpower == -1))
        return;

    if (m_key == ui->vlanLineEdit->text()
            && (m_band_index == ui->bandComboBox->currentIndex())
            && (m_bandwidth == ui->bandwidthComboBox->currentIndex())
            && (m_txpower == ui->spinBox->value())
            ){
        ui->saveBtn->setEnabled(false);
    } else {
        ui->saveBtn->setEnabled(true);
    }
}

/*
 * 修改后左对齐, 光标放在右端
*/
void MainWindow::on_vlanLineEdit_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1)
    ui->vlanLineEdit->setText(arg1);
    ui->vlanLineEdit->setCursorPosition(arg1.length());
    tryEnableSaveBtn();
}

/*
 * After remove refresh button, not use anymore
 * see
 */
void MainWindow::on_refreshBtn_clicked()
{
    delete m_nextBitArray;
    m_nextBitArray = new QBitArray(NEXT_REQ_MAX, 1);
    nextRequest();
}

void MainWindow::on_bandComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    tryEnableSaveBtn();
}

void MainWindow::on_bandwidthComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    tryEnableSaveBtn();
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    Q_UNUSED(arg1)
    tryEnableSaveBtn();
}

void MainWindow::on_saveBtn_clicked()
{
    if (ui->vlanLineEdit->text().length() % 2 == 1) {
        QMessageBox::critical(this, tr("Error"), "格式不对接入密钥必须是一个偶数长度的16进制数");
        return;
    }

    if (m_key != ui->vlanLineEdit->text()){
        qInfo("m_key write");
        auto cmd = tr("AT^DAPI=\"%1\"").arg(ui->vlanLineEdit->text());
        writeData(cmd.toLocal8Bit());
        m_nextBitArray->setBit(DAPI);
    }
    if (m_band_index != ui->bandComboBox->currentIndex()) {
        qInfo("m_band_index write");//
        auto cmd = tr("AT^DAOCNDI=%1\r").arg(m_band[ui->bandComboBox->currentIndex()].toStdString().c_str());
        writeData(cmd.toLocal8Bit());
        m_nextBitArray->setBit(DAOCNDI);
    }
    if (m_bandwidth != ui->bandwidthComboBox->currentIndex()) {
        qInfo("m_bandwidth write");
        auto cmd = tr("AT^DRPS=,%1,\"%2\"\r").arg(ui->bandwidthComboBox->currentIndex()).arg(ui->spinBox->value());
        writeData(cmd.toLocal8Bit());
        m_nextBitArray->setBit(DRPS);
    }
    if (m_txpower != ui->spinBox->value()) {
        //强制发射和最大功率设置一样
        qInfo("m_txpower write");
        auto cmd = tr("AT^DRPS=,%1,\"%2\"\r").arg(ui->bandwidthComboBox->currentIndex()).arg(ui->spinBox->value());
        writeData(cmd.toLocal8Bit());
        cmd = tr("AT^DSONSFTP=1,\"%1\"\r").arg(ui->spinBox->value());
        writeData(cmd.toLocal8Bit());
        m_nextBitArray->setBit(DRPS);
    }
}

void MainWindow::on_ipLineEdit_2_textEdited(const QString &arg1)
{
    if (m_netIP == arg1) {
        ui->ipSetBtn->setEnabled(false);
    } else {
        ui->ipSetBtn->setEnabled(true);
    }
}

void MainWindow::on_advSaveBtn_clicked()
{
    qInfo() << "on_advSaveBtn_clicked" <<m_advAuto<<","<<ui->advAutoComboBox->currentIndex();
    //0-自动 1-主控 2-接入
    if (m_advAuto != ui->advAutoComboBox->currentIndex()) {
        auto cmd = tr("AT^DDTC=%1").arg(ui->advAutoComboBox->currentIndex());
        writeData(cmd.toLocal8Bit());
        m_nextBitArray->setBit(DDTC);
    }
}

void MainWindow::on_advSendBtn_clicked()
{
    auto cmd = ui->advLineEdit->text();
    if (cmd.startsWith("AT")){
        writeData(cmd.toLocal8Bit());
    } else {
        QMessageBox::critical(this, tr("Error"), "格式不对");
    }
}

void MainWindow::on_advAutoComboBox_currentIndexChanged(int index)
{
    if (m_advAuto == index) {
        ui->advSaveBtn->setEnabled(false);
    } else {
        ui->advSaveBtn->setEnabled(true);
    }
}

void MainWindow::on_refreshAction_triggered()
{
    delete m_nextBitArray;
    m_nextBitArray = new QBitArray(NEXT_REQ_MAX, 1);
    nextRequest();
}
