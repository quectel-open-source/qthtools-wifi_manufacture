#include "burning.h"

#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QDateTime>
#include <QRandomGenerator>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QWriteLocker>
#include <QSettings>
#include <QDir>
#include <QReadWriteLock>

QReadWriteLock g_readWriteLock;
extern QByteArray currentMac;
extern QString vitalFilePath;
extern QList<QByteArray> availableMac_list;
extern HttpClient* httpClient;

Burning::Burning(QString comName, int baud,  int tid, QObject *parent) : QObject(parent)
{
    qserial = new qSerial(this);
    serialTimer = new QTimer(this);
    connect(serialTimer, &QTimer::timeout, this, &Burning::slot_serialTimeout);
    burnTime = new QTimer(this);
    connect(burnTime, &QTimer::timeout, this, &Burning::slot_burnTimeout);
    serialTimer->start(10 * 1000);
    burnTime->start(500);
    myId = tid;
    isOpen = qserial->serialOpen(comName, baud, QSerialPort::Data8, QSerialPort::NoParity, QSerialPort::OneStop, QSerialPort::NoFlowControl);
    connect(qserial, &qSerial::dataReadNoticSignal, this, &Burning::slot_dataRead);
}

Burning::~Burning()
{
}

// 日志输出
void Burning::addLogs(QString log, bool isrecv)
{
    if (isrecv)
    {
        qDebug() << "[RX]" + QString("Port%1: ").arg(QString::number(myId)) + log;
    }
    else
    {
        qDebug() << "[TX]" + QString("Port%1: ").arg(QString::number(myId)) + log;
    }
}
// 串口发送
void Burning::qserialSendData(QString sendData)
{
    if (this->qserial->serialIsOpen())
    {
        if (qserial->SerialSend((sendData + "\r\n").toUtf8()) != -1)
        {
            addLogs((sendData + "\r\n"), false);
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "serial send data is failed !";
        }
    }
}

char Burning::convertHexChar(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return ch - 0x30;
    }
    else if ((ch >= 'A') && (ch <= 'F'))
    {
        return ch - 'A' + 10;
    }
    else if ((ch >= 'a') && (ch <= 'f'))
    {
        return ch - 'a' + 10;
    }
    else
    {
        return (-1);
    }
}

QString Burning::hexStrCheckSum(const QString &str)
{
    QByteArray senddata;
    int hexdata, lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len / 2);
    char lstr, hstr;

    for (int i = 0; i < len;) {
        hstr = str.at(i).toLatin1();
        if (hstr == ' ') {
            i++;
            continue;
        }

        i++;
        if (i >= len) {
            break;
        }

        lstr = str.at(i).toLatin1();
        hexdata = convertHexChar(hstr);
        lowhexdata = convertHexChar(lstr);

        if ((hexdata == 16) || (lowhexdata == 16)) {
            break;
        } else {
            hexdata = hexdata * 16 + lowhexdata;
        }

        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }

    senddata.resize(hexdatalen);

    uint8_t checkSum = 0;
    const char *a = senddata.data();
    for (int i = 0; i < senddata.length(); i++)
    {
        checkSum += a[i];
    }

    return QString::number(checkSum, 16);
}
// 存储环境配置信息
void Burning::storeEnvInfo(QString ak, QString as, QString host)
{
    burnInfo.ak = ak;
    burnInfo.as = as;
    m_host = host;
}
// 清除烧录信息
void Burning::slot_clearBurnInfo(int portId)
{
    if (!availableMac_list.contains(m_newWiFiMac))
    {
        storeMac(m_newWiFiMac);
    }
    if (myId == portId)
    {
        // 清除配置信息
        this->qserialSendData("configpara write ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff b8");
        // 还原mac
        QTimer::singleShot(1 * 10, this, [=]()
        {
            this->qserialSendData("mac " + m_oldMac);
        });
        QTimer::singleShot(1 * 5, this, [=]()
        {
            this->qserialSendData("txevm -e 2");
        });
        if ("IOT-SBLWF-ACM" == authOc)// matter-授权码 OC
        {
            // 清除matter证书及二维码信息
            QTimer::singleShot(1 * 20, this, [=]()
            {
                this->qserialSendData("hello");
            });
        }
    }
}
// 串口关闭
void Burning::slot_closePort()
{
    burnTime->stop();
    disconnect(burnTime, &QTimer::timeout, this, &Burning::slot_burnTimeout);
    burnTime->deleteLater();
    burnTime = nullptr;
    qserial->SerialClose();
}
// 烧录超时槽
void Burning::slot_burnTimeout()
{
    this->qserialSendData("mac");
}
// 回复端口打开结果
void Burning::slot_replyStatus()
{
    if (isOpen)
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "serialport is open";
        emit signal_status(myId, tr("端口打开成功"));

    }
    else
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "open failed";
        emit signal_status(myId, tr("端口异常"));
    }
}
// 存储生产订单信息
void Burning::slot_proInfo(QJsonObject proInfo_object)
{
    if (!proInfo_object.isEmpty())
    {
        burnInfo.proInfoObject = proInfo_object;
        burnInfo.pk = burnInfo.proInfoObject.value("productKey").toString();
        pk_old = "p11qUM";
    }
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "burnInfo.proInfoObject:" << burnInfo.proInfoObject;
}

void Burning::slot_storeChecked(bool isChecked)
{
    this->qrChecked = isChecked;
}
// 检测模组是否无响应
void Burning::slot_serialTimeout()
{
    qInfo() << __FUNCTION__;
    if (recvAllData == olderData)
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("设备无响应");
        emit signal_status(myId, tr("设备无响应"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        serialTimer->stop();
    }
    else
    {
        olderData = recvAllData;
    }
}
// 串口数据接收槽
void Burning::slot_dataRead(QByteArray serialBuffer)
{
    this->addLogs(serialBuffer, true);
    switch(m_dataReadMode)
    {
    case BURN_MODE:
    {
        this->dataBurn(serialBuffer);
        break;
    }
    case CHECK_MODE:
    {
        this->dataCheck(serialBuffer);
        break;
    }
    default:
        break;
    }
}
// mac校验烧录
void Burning::mac_check_burn(QString recvData)
{
    QString macAddress = recvAllData.mid(recvData.indexOf("MAC address: ") + 13, 17);
    burnInfo.dks = macAddress.remove("-", Qt::CaseSensitive).toUpper();
    m_oldMac = burnInfo.dks;
    QString startMac = burnInfo.proInfoObject.value("startMac").toString();
    QString endMac = burnInfo.proInfoObject.value("endMac").toString();
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<
                "( startMac <= currentMac <= endMac ): " << (burnInfo.dks <= endMac && burnInfo.dks >= startMac);
    authOc = burnInfo.proInfoObject.value("authOc").toString();
    if (!(burnInfo.dks <= endMac && burnInfo.dks >= startMac))
    {
        m_newWiFiMac = generateMacInRange(burnInfo.proInfoObject.value("endMac").toString().toUtf8());
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "newMac: " << m_newWiFiMac;
        if (!m_newWiFiMac.isEmpty())
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("硬件信息写入");
            emit signal_status(myId, tr("硬件信息写入"));
            this->qserialSendData("mac " + m_newWiFiMac);
            burnInfo.dks = m_newWiFiMac;

            if ("IOT-SBLWF-ACF" == authOc)// 免开发-授权码 OC
            {
                qulonglong macNum = m_newWiFiMac.toULongLong(nullptr, 16);
                QByteArray newBtMac = QByteArray::number((macNum + 1), 16).toUpper();
                qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "newBtMac: " << newBtMac;
                QTimer::singleShot(1 * 1000, this, [=]()
                {
                    this->qserialSendData("btmac " + newBtMac);
                });
            }
            return;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("无可用授权数");
            emit signal_status(myId, tr("无可用授权数"));
            serialTimer->stop();
            return;
        }
    }
    //如果获取到mac地址的在availableMac_list中，移除掉
    if (availableMac_list.contains(burnInfo.dks.toUtf8()))
    {
        availableMac_list.removeOne(burnInfo.dks.toUtf8());
        this->refreshMacList();
    }
    devRegister();
}
// 生成mac号段范围内的mac
QByteArray Burning::generateMacInRange(const QByteArray &endMac)
{
    QWriteLocker writeLock(&g_readWriteLock);
    if (!availableMac_list.isEmpty() && !availableMac_list.contains("[\n]\n"))
    {
        // 移除并返回第一个可用的mac
        qDebug() << "availableMac_list: " << availableMac_list;
        QByteArray newMac = availableMac_list.takeFirst();
        refreshMacList();
        return newMac;
    }
    else if (currentMac >= endMac)
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "no mac available";
        emit signal_status(myId, tr("无可用MAC"));
        return ""; // 如果没有找到，返回空字节数组
    }
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "currentMac: " << currentMac;
    qulonglong macNum = currentMac.toULongLong(nullptr, 16);
    currentMac = QByteArray::number((macNum + 1), 16).toUpper();
    // 将当前mac记录到.ini文件中
    QSettings settings(vitalFilePath + "config.ini", QSettings::IniFormat);
    settings.setValue("currentMac", currentMac);
    return currentMac;
}
// 配置文件烧写
void Burning::configFile_burn()
{
    burnInfo.random = QRandomGenerator::global()->bounded(0, 999999);
    QByteArray info  = QString("pk=" + burnInfo.pk + "&random=" + QString::number(burnInfo.random)).toUtf8();
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"info: "<<info;
    QString sign = QMessageAuthenticationCode::hash(info, burnInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
    QString url;
    if (m_host.isEmpty())
    {
        url = m_defaultHost + "/v2/factory/product/deliverables/latest?pk="
                        + burnInfo.pk + "&ak=" + burnInfo.ak + "&random=" + QString::number(burnInfo.random)
                        + "&sign=" + sign;
    }
    else
    {
        url = m_host + "/v2/factory/product/deliverables/latest?pk="
                        + burnInfo.pk + "&ak=" + burnInfo.ak + "&random=" + QString::number(burnInfo.random)
                        + "&sign=" + sign;
    }
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"sign: "<<sign;
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"url: "<<url;
    QString strReply = httpClient->get_request(url);
    if (strReply.isEmpty())
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配置文件获取失败");
        emit signal_status(myId, tr("配置文件获取失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }

    QJsonDocument tojson;
    QJsonParseError ParseError;
    tojson = QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
    if(!tojson.isEmpty() && ParseError.error == QJsonParseError::NoError)
    {
        if(tojson.isObject())
        {
            QJsonObject Object = tojson.object();
            if(!Object.isEmpty())
            {
                if (Object.contains("code"))
                {
                    int code = Object.value("code").toInt();
                    if (code == 200 && Object.contains("data"))
                    {
                        QJsonObject JsonObject_data = Object.value("data").toObject();
                        if (!JsonObject_data.isEmpty())
                        {
                            if (JsonObject_data.contains("config"))
                            {
                                matterCertificateInfo.config = JsonObject_data.value("config").toString();
                            }
                        }
                    }
                    else
                    {
                        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配置文件获取失败");
                        emit signal_status(myId, tr("配置文件获取失败"));
                        if (!availableMac_list.contains(m_newWiFiMac))
                        {
                            storeMac(m_newWiFiMac);
                        }
                        return;
                    }
                }
            }
        }
    }

    if (matterCertificateInfo.config != "")
    {
        QString sendData = "configpara write " + matterCertificateInfo.config + " " + hexStrCheckSum(matterCertificateInfo.config) + "\r\n";
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配置文件获取成功");
        emit signal_status(myId, tr("配置文件获取成功"));
        this->qserialSendData(sendData);
    }
    else
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配置文件获取失败");
        emit signal_status(myId, tr("配置文件获取失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
    }
}
// 生成matter设备证书
void Burning::genCertInfo()
{
    burnInfo.random = QRandomGenerator::global()->bounded(0, 999999);
    QByteArray info  = QString("dks=[" + burnInfo.dks + "]&pk=" + burnInfo.pk + "&random=" + QString::number(burnInfo.random)).toUtf8();
    burnInfo.sign = QMessageAuthenticationCode::hash(info, burnInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "sign:" << burnInfo.sign;
    QString url;
    if (m_host.isEmpty())
    {
        url = m_defaultHost + "/v2/factory/matter/device/genCertInfo";
    }
    else
    {
        url = m_host + "/v2/factory/matter/device/genCertInfo";
    }
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"url:"<<url;

    QJsonObject jsonObject;
    jsonObject.insert("pk", burnInfo.pk);
    QJsonArray jsonArray_dks;
    jsonArray_dks.append(burnInfo.dks);
    jsonObject.insert("dks", QJsonValue(jsonArray_dks));
    jsonObject.insert("ak", burnInfo.ak);
    jsonObject.insert("random", burnInfo.random);
    jsonObject.insert("sign", burnInfo.sign);
    QJsonObject jsonObject_dac;
    jsonObject_dac.insert("dk", burnInfo.dks);
    jsonObject_dac.insert("dacPublicKeyHexStr", burnInfo.publicKey);
    QJsonArray jsonArray_dac;
    jsonArray_dac.append(jsonObject_dac);
    jsonObject.insert("dacCommands", QJsonValue(jsonArray_dac));
    QJsonDocument document;
    document.setObject(jsonObject);
    QByteArray byteArray = document.toJson(QJsonDocument::Compact);
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << byteArray;

    QString strReply = httpClient->post_request(url, byteArray);
    if (strReply.isEmpty())
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息获取失败");
        emit signal_status(myId, tr("matter信息获取失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }
    QJsonDocument tojson;
    QJsonParseError ParseError;
    tojson = QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
    if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
    {
        if(tojson.isObject())
        {
            QJsonObject Object = tojson.object();
            if(!Object.isEmpty())
            {
                if (Object.contains("code"))
                {
                    int code = Object.value("code").toInt();
                    if (code == 200 && Object.contains("data"))
                    {
                        QJsonObject JsonObject_data = Object.value("data").toObject();
                        if (!JsonObject_data.isEmpty())
                        {
                            if (JsonObject_data.contains("devCertInfos"))
                            {
                                QJsonArray jsonArray_devCert = JsonObject_data.value("devCertInfos").toArray();

                                if (jsonArray_devCert.count() > 0)
                                {
                                    QJsonObject JsonObject_devCertInfos = jsonArray_devCert.at(0).toObject();
                                    if (!JsonObject_devCertInfos.isEmpty())
                                    {
                                        if (JsonObject_devCertInfos.contains("discriminator"))
                                        {
                                            matterCertificateInfo.discriminator = JsonObject_devCertInfos.value("discriminator").toInt();
                                        }
                                        if (JsonObject_devCertInfos.contains("iterationCount"))
                                        {
                                            matterCertificateInfo.iterationCount = JsonObject_devCertInfos.value("iterationCount").toInt();
                                        }
                                        if (JsonObject_devCertInfos.contains("salt"))
                                        {
                                            matterCertificateInfo.salt = JsonObject_devCertInfos.value("salt").toString();
                                        }
                                        if (JsonObject_devCertInfos.contains("verifier"))
                                        {
                                            matterCertificateInfo.verifier = JsonObject_devCertInfos.value("verifier").toString();
                                        }
                                        if (JsonObject_devCertInfos.contains("passcode"))
                                        {
                                            matterCertificateInfo.passcode = JsonObject_devCertInfos.value("passcode").toInt();
                                        }
                                        if (JsonObject_devCertInfos.contains("dacCertHexStr"))
                                        {
                                            matterCertificateInfo.dacCertHexStr = JsonObject_devCertInfos.value("dacCertHexStr").toString();
                                        }
                                        if (JsonObject_devCertInfos.contains("failDes"))
                                        {
                                            if(!JsonObject_devCertInfos.value("failDes").toString().isEmpty())
                                            {
                                                qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<
                                                            "failDes:"<<JsonObject_devCertInfos.value("failDes").toString();
                                                emit signal_status(myId, tr("matter信息获取失败"));
                                                if (!availableMac_list.contains(m_newWiFiMac))
                                                {
                                                    storeMac(m_newWiFiMac);
                                                }
                                                return;
                                            }
                                        }
                                    }
                                }
                            }
                            if (JsonObject_data.contains("vendorId"))
                            {
                                matterCertificateInfo.vendorId = JsonObject_data.value("vendorId").toInt();
                            }
                            if (JsonObject_data.contains("productId"))
                            {
                                matterCertificateInfo.productId = JsonObject_data.value("productId").toInt();
                            }
                            if (JsonObject_data.contains("vendorName"))
                            {
                                matterCertificateInfo.vendorName = JsonObject_data.value("vendorName").toString();
                            }
                            if (JsonObject_data.contains("productName"))
                            {
                                matterCertificateInfo.productName = JsonObject_data.value("productName").toString();
                            }
                            if (JsonObject_data.contains("paiCerHexStr"))
                            {
                                matterCertificateInfo.paiCerHexStr = JsonObject_data.value("paiCerHexStr").toString();
                            }
                            if (JsonObject_data.contains("cdCertHexStr"))
                            {
                                matterCertificateInfo.cdCertHexStr = JsonObject_data.value("cdCertHexStr").toString();
                            }
                        }
                    }
                    else
                    {
                        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息获取失败");
                        emit signal_status(myId, tr("matter信息获取失败"));
                        if (!availableMac_list.contains(m_newWiFiMac))
                        {
                            storeMac(m_newWiFiMac);
                        }
                        return;
                    }
                }
            }
        }
    }

    if (matterCertificateInfo.discriminator != 0)
    {
        QString sendData = "matterpara discriminator write " + QString::number(matterCertificateInfo.discriminator) + "\r\n";
        this->qserialSendData(sendData);
        burnMode = 5;
    }
    else
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息获取失败");
        emit signal_status(myId, tr("matter信息获取失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }
}
// 设备注册接口
void Burning::devRegister()
{
    burnInfo.random = QRandomGenerator::global()->bounded(0, 999999);
    QByteArray info  = QString("pk=" + burnInfo.pk + "&random=" + QString::number(burnInfo.random)).toUtf8();
    burnInfo.sign = QMessageAuthenticationCode::hash(info, burnInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"sign:"<<burnInfo.sign;
    QString url;
    if (m_host.isEmpty())
    {
        url = m_defaultHost + "/v2/factory/devices/createOne";
    }
    else
    {
        url = m_host + "/v2/factory/devices/createOne";
    }
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"url:"<<url;

    QJsonObject jsonObject;
    jsonObject.insert("mac", burnInfo.dks);
    jsonObject.insert("pk", burnInfo.pk);
    jsonObject.insert("ak", burnInfo.ak);
    jsonObject.insert("random", burnInfo.random);
    jsonObject.insert("sign", burnInfo.sign);
    jsonObject.insert("authMode", burnInfo.authMode);

    QJsonDocument document;
    document.setObject(jsonObject);
    QByteArray byteArray = document.toJson(QJsonDocument::Compact);
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << byteArray;

    QString strReply = httpClient->post_request(url, byteArray);
    if (strReply.isEmpty())
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("设备注册失败");
        emit signal_status(myId, tr("设备注册失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }
    else if (strReply.contains("\"code\":200"))
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("设备注册成功");
        emit signal_status(myId, tr("设备注册成功"));
        QString authOc = burnInfo.proInfoObject.value("authOc").toString();
        if ("IOT-SBLWF-ACM" == authOc)// matter-授权码 OC
        {
            burnMode = 2;
            configFile_burn();
        }
        else if ("IOT-SBLWF-ACF" == authOc)// 免开发-授权码 OC
        {
            burnMode = 1;
            configFile_burn();
        }
    }
}
// 获取写入matter配网二维码相关参数
void Burning::qrInfoBurn()
{
    QByteArray info  = QString("dks=[" + burnInfo.dks + "]&pk=" + burnInfo.pk + "&random=" + QString::number(burnInfo.random)).toUtf8();
    burnInfo.sign = QMessageAuthenticationCode::hash(info, burnInfo.as.toUtf8(), QCryptographicHash::Sha256).toHex();
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"sign:"<<burnInfo.sign;
    QString url;
    if (m_host.isEmpty())
    {
        url = m_defaultHost + "/v2/factory/matter/device/genQrCodeStr";
    }
    else
    {
        url = m_host + "/v2/factory/matter/device/genQrCodeStr";
    }
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) <<"url:"<<url;

    QJsonObject jsonObject;
    jsonObject.insert("pk", burnInfo.pk);
    QJsonArray jsonArray_dks;
    jsonArray_dks.append(burnInfo.dks);
    jsonObject.insert("dks", QJsonValue(jsonArray_dks));
    jsonObject.insert("ak", burnInfo.ak);
    jsonObject.insert("random", burnInfo.random);
    jsonObject.insert("sign", burnInfo.sign);
    QJsonDocument document;
    document.setObject(jsonObject);
    QByteArray byteArray = document.toJson(QJsonDocument::Compact);
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << byteArray;

    QString strReply = httpClient->post_request(url, byteArray);
    if (strReply.isEmpty())
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配网码信息获取失败");
        emit signal_status(myId, tr("配网码信息获取失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }

    QJsonDocument tojson;
    QJsonParseError ParseError;
    tojson = QJsonDocument::fromJson(strReply.toUtf8(), &ParseError);
    if(!tojson.isNull() && ParseError.error == QJsonParseError::NoError)
    {
        if(tojson.isObject())
        {
            QJsonObject Object = tojson.object();
            if(!Object.isEmpty())
            {
                if (Object.contains("code"))
                {
                    int code = Object.value("code").toInt();
                    if (code == 200 && Object.contains("data"))
                    {
                        QJsonObject JsonObject_data = Object.value("data").toObject();
                        if (!JsonObject_data.isEmpty())
                        {
                            if (JsonObject_data.contains("detailDTOS"))
                            {
                                QJsonArray jsonArray_detail = JsonObject_data.value("detailDTOS").toArray();
                                if (jsonArray_detail.count() > 0)
                                {
                                    QJsonObject jsonArray_detailDTOS = jsonArray_detail.at(0).toObject();
                                    if (!jsonArray_detailDTOS.isEmpty())
                                    {
                                        if (jsonArray_detailDTOS.contains("qrCodeStr"))
                                        {
                                            matterCertificateInfo.qrCodeStr = jsonArray_detailDTOS.value("qrCodeStr").toString();
                                        }
                                        if (jsonArray_detailDTOS.contains("manualPairCode"))
                                        {
                                            matterCertificateInfo.manualPairCode = jsonArray_detailDTOS.value("manualPairCode").toString();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配网码信息获取失败");
                    emit signal_status(myId, tr("配网码信息获取失败"));
                    if (!availableMac_list.contains(m_newWiFiMac))
                    {
                        storeMac(m_newWiFiMac);
                    }
                    return;
                }
            }
        }
    }

    if (matterCertificateInfo.qrCodeStr != "" && matterCertificateInfo.manualPairCode != "")
    {
        QString sendData = "matterpara qrcode write [" + matterCertificateInfo.qrCodeStr + "][" + matterCertificateInfo.manualPairCode + "]\r\n";
        this->qserialSendData(sendData);
        burnMode = 17;
    }
    else
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配网码信息写入失败");
        emit signal_status(myId, tr("配网码信息写入失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }
}
// 数据烧录模式
void Burning::dataBurn(QByteArray serialBuffer)
{
    if (serialBuffer.contains("not found") || serialBuffer.contains("err") || serialBuffer.contains("fail"))
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("烧录失败");
        emit signal_status(myId, tr("烧录失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
        return;
    }
    else if (!serialBuffer.contains("#"))
    {
        recvAllData.append(serialBuffer);
        return;
    }
    recvAllData.append(serialBuffer);
    if (recvAllData.contains("#") && !recvAllData.contains("Set ") &&
             recvAllData.contains("MAC address: ") && !recvAllData.contains("configpara write "))
    {
        if (recvAllData.mid(0, 3) != "mac" || recvAllData.indexOf("#") < recvAllData.lastIndexOf("mac"))
        {
            recvAllData.clear();
            return;
        }
        burnTime->stop();
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("MAC 校验");
        emit signal_status(myId, tr("MAC 校验"));
        mac_check_burn(recvAllData);
    }
    else if (recvAllData.contains("#") && recvAllData.contains("Set "))
    {
        m_macSetCount += 1;
        if (m_macSetCount >= 2)
        {
            recvAllData.clear();
            return;
        }
        burnTime->stop();
        this->qserialSendData("txevm -e 2");
        recvAllData.clear();
        devRegister();
    }
    else if (recvAllData.contains("success") && (1 == burnMode || 2 == burnMode))
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配置文件写入成功");
        emit signal_status(myId, tr("配置文件写入成功"));
        if (2 == burnMode)
        {
            this->qserialSendData("hello");
            burnMode = 3;
        }
        else if (1 == burnMode)
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("产测校验");
            emit signal_status(myId, tr("产测校验"));
            m_dataReadMode = CHECK_MODE;
            this->qserialSendData("mac");
            burnTime->start(500);
        }
    }
    else if (recvAllData.contains("be ready"))
    {
        this->qserialSendData("dacpub");
        burnMode = 4;
    }
    else if (4 == burnMode)
    {
        if (!recvAllData.contains("#"))
        {
            dacpubBuf.append(recvAllData);
            recvAllData.clear();
            return;
        }
        dacpubBuf.append(recvAllData);
        QStringList strList = QString(dacpubBuf).split("\r\n");
        dacpubBuf.clear();
        if (strList.size() != 5)
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息获取失败");
            emit signal_status(myId, tr("matter信息获取失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }

        burnInfo.publicKey = "3059301306072a8648ce3d020106082a8648ce3d030107034200" + strList.at(2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "burnInfo.publicKey" << burnInfo.publicKey;
        genCertInfo();
    }
    else if (recvAllData.contains("success") && 5 == burnMode)
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息获取成功");
        emit signal_status(myId, tr("matter信息获取成功"));
        if (matterCertificateInfo.vendorId != 0)
        {
            QString vendorId = QString::number(matterCertificateInfo.vendorId, 16);
            if (vendorId.length() % 2)
            {
                vendorId = "0" + vendorId;
            }
            QString sendData = "matterpara venid write " + vendorId + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 6;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 6 == burnMode)
    {
        if (matterCertificateInfo.productId != 0)
        {
            QString productId = QString::number(matterCertificateInfo.productId, 16);
            if (productId.length() % 2)
            {
                productId = "0" + productId;
            }
            QString sendData = "matterpara proid write " + productId + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 7;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 7 == burnMode)
    {
        if (matterCertificateInfo.iterationCount != 0)
        {
            QString sendData = "matterpara itcount write " + QString::number(matterCertificateInfo.iterationCount) + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 8;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 8 == burnMode)
    {
        if (matterCertificateInfo.salt != "")
        {
            QString sendData = "matterpara salt write " + matterCertificateInfo.salt + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 9;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 9 == burnMode)
    {
        if (matterCertificateInfo.verifier != "")
        {
            QString sendData = "matterpara verifier write " + matterCertificateInfo.verifier + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 10;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 10 == burnMode)
    {
        if (matterCertificateInfo.passcode != 11)
        {
            QString sendData = "matterpara pincode write " + QString::number(matterCertificateInfo.passcode) + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 11;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 11 == burnMode)
    {
        if (matterCertificateInfo.vendorName != "")
        {
            QString sendData = "matterpara vename write " + matterCertificateInfo.vendorName + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 12;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 12 == burnMode)
    {
        if (matterCertificateInfo.productName != "")
        {
            QString sendData = "matterpara proname write " + matterCertificateInfo.productName + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 13;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 13 == burnMode)
    {
        if (matterCertificateInfo.paiCerHexStr != "")
        {
            QString sendData = "paicert write " + matterCertificateInfo.paiCerHexStr + " " + hexStrCheckSum(matterCertificateInfo.paiCerHexStr) + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 14;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 14 == burnMode)
    {
        if (matterCertificateInfo.cdCertHexStr != "")
        {
            QString sendData = "cdcert write " + matterCertificateInfo.cdCertHexStr + " " + hexStrCheckSum(matterCertificateInfo.cdCertHexStr) + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 15;
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 15 == burnMode)
    {
        if (matterCertificateInfo.dacCertHexStr != "")
        {
            QString sendData = "daccert write " + matterCertificateInfo.dacCertHexStr + " " + hexStrCheckSum(matterCertificateInfo.dacCertHexStr) + "\r\n";
            this->qserialSendData(sendData);
            burnMode = 16;
            emit getQrChecked();
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入失败");
            emit signal_status(myId, tr("matter信息写入失败"));
            if (!availableMac_list.contains(m_newWiFiMac))
            {
                storeMac(m_newWiFiMac);
            }
            recvAllData.clear();
            return;
        }
    }
    else if (recvAllData.contains("success") && 16 == burnMode)
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("matter信息写入成功");
        emit signal_status(myId, tr("matter信息写入成功"));
        if (this->qrChecked)
        {
            qrInfoBurn();
        }
        else
        {
            emit signal_status(myId, tr("产测校验"));
            m_dataReadMode = CHECK_MODE;
            this->qserialSendData("mac");
            burnTime->start(500);
        }
    }
    else if (recvAllData.contains("success") && 17 == burnMode)
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("配网码信息写入成功");
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("产测校验");
        emit signal_status(myId, tr("配网码信息写入成功"));
        emit signal_status(myId, tr("产测校验"));
        m_dataReadMode = CHECK_MODE;
        this->qserialSendData("mac");
        burnTime->start(500);
    }
    recvAllData.clear();
}
// 数据校验模式
void Burning::dataCheck(QByteArray serialBuffer)
{
    recvAllData = serialBuffer;
    if (recvAllData.contains("not found") || recvAllData.contains("para err") || recvAllData.contains("fail"))
    {
        qDebug() << QString("Port%1 ").arg(QString::number(myId))  << tr("校验失败");
        checkCount++;
        recvAllData = "";
        return;
    }
    else if (!serialBuffer.contains("#"))
    {
        buffer.append(serialBuffer);
        return;
    }
    buffer.append(serialBuffer);
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "buffer: " << buffer;
    if (buffer.contains("#") && !buffer.contains("Set ") &&
             buffer.contains("MAC address: ") && !buffer.contains("configpara write "))
    {
        if (buffer.mid(0, 3) != "mac" || buffer.indexOf("#") < buffer.lastIndexOf("mac"))
        {
            buffer = "";
            return;
        }
        burnTime->stop();
        QString macAddress = buffer.mid(buffer.indexOf("MAC address: ") + 13, 17);
        macAddress = macAddress.remove("-").toUpper();
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "macAddress: " << macAddress;
        if (!(macAddress == burnInfo.dks))
        {
            this->reburn();
        }
        else
        {
            this->qserialSendData("configpara read");
            checkMode = 1;
        }
    }
    else if (1 == checkMode)
    {
        QString configpara = buffer.split("\r\n").at(2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "configpara: " << configpara;
        if (!(configpara == matterCertificateInfo.config))
        {
            this->reburn();
        }
        else
        {
            if ("IOT-SBLWF-ACF" == authOc)
            {
                qDebug() << QString("Port%1 ").arg(QString::number(myId)) << tr("免开发产测校验成功");
                serialTimer->stop();
                m_dataReadMode = RESET_MODE;
                emit signal_status(myId, tr("产测校验成功"));
                emit signal_activation(myId , burnInfo.dks);
            }
            else if ("IOT-SBLWF-ACM" == authOc)
            {
                checkMode = 2;
                this->qserialSendData("matterpara discriminator read");
            }
        }
    }
    else if (2 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        int value = payload.mid(payload.indexOf("=") + 2).toInt();
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "discriminator value: " << value;
        if (!(value == matterCertificateInfo.discriminator))
        {
            this->reburn();
        }
        else
        {
            checkMode = 3;
            this->qserialSendData("matterpara venid read");
        }
    }
    else if (3 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        int value = payload.mid(payload.indexOf("=") + 2).toInt(nullptr, 16);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "venid value: " << value;
        if (!(value == matterCertificateInfo.vendorId))
        {
            this->reburn();
        }
        else
        {
            checkMode = 4;
            this->qserialSendData("matterpara proid read");
        }
    }
    else if (4 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        int value = payload.mid(payload.indexOf("=") + 2).toInt(nullptr, 16);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "proid value: " << value;
        if (!(value == matterCertificateInfo.productId))
        {
            this->reburn();
        }
        else
        {
            checkMode = 5;
            this->qserialSendData("matterpara itcount read");
        }
    }
    else if (5 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        int value = payload.mid(payload.indexOf("=") + 2).toInt();
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "itcount value: " << value;
        if (!(value == matterCertificateInfo.iterationCount))
        {
            this->reburn();
        }
        else
        {
            checkMode = 6;
            this->qserialSendData("matterpara salt read");
        }
    }
    else if (6 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        QString value = payload.mid(payload.indexOf("=") + 2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "salt value: " << value;
        if (!(value == matterCertificateInfo.salt))
        {
            this->reburn();
        }
        else
        {
            checkMode = 7;
            this->qserialSendData("matterpara verifier read");
        }
    }
    else if (7 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        QString value = payload.mid(payload.indexOf("=") + 2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "verifier value: " << value;
        if (!(value == matterCertificateInfo.verifier))
        {
            this->reburn();
        }
        else
        {
            checkMode = 8;
            this->qserialSendData("matterpara pincode read");
        }
    }
    else if (8 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        int value = payload.mid(payload.indexOf("=") + 2).toInt();
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "pincode value: " << value;
        if (!(value == matterCertificateInfo.passcode))
        {
            this->reburn();
        }
        else
        {
            checkMode = 9;
            this->qserialSendData("matterpara vename read");
        }
    }
    else if (9 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        QString value = payload.mid(payload.indexOf("=") + 2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "vename value: " << value;
        if (!(value == matterCertificateInfo.vendorName))
        {
            this->reburn();
        }
        else
        {
            checkMode = 10;
            this->qserialSendData("matterpara proname read");
        }
    }
    else if (10 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        QString value = payload.mid(payload.indexOf("=") + 2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "proname value: " << value;
        if (!(value == matterCertificateInfo.productName))
        {
            this->reburn();
        }
        else
        {
            checkMode = 11;
            this->qserialSendData("paicert read");
        }
    }
    else if (11 == checkMode)
    {
        QString value = buffer.split("\r\n").at(2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "paicert value: " << value;
        if (!(value == matterCertificateInfo.paiCerHexStr))
        {
            this->reburn();
        }
        else
        {
            checkMode = 12;
            this->qserialSendData("cdcert read");
        }
    }
    else if (12 == checkMode)
    {
        QString value = buffer.split("\r\n").at(2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "cdcert value: " << value;
        if (!(value == matterCertificateInfo.cdCertHexStr))
        {
            this->reburn();
        }
        else
        {
            checkMode = 13;
            this->qserialSendData("daccert read");
        }
    }
    else if (13 == checkMode)
    {
        QString value = buffer.split("\r\n").at(2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "daccert value: " << value;
        if (!(value == matterCertificateInfo.dacCertHexStr))
        {
            this->reburn();
        }
        else
        {
            checkMode = 14;
            if (!qrChecked)
            {
                qDebug() << QString("Port%1 ").arg(QString::number(myId))  << tr("matter产测校验成功");
                serialTimer->stop();
                m_dataReadMode = RESET_MODE;
                emit signal_status(myId, tr("产测校验成功"));
                emit signal_activation(myId , burnInfo.dks);
            }
            else
            {
                this->qserialSendData("matterpara qrcode read");
            }
        }
    }
    else if (14 == checkMode)
    {
        QString payload = buffer.split("\r\n").at(2);
        QString value = payload.mid(payload.indexOf("=") + 2);
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "matterCertificateInfo.qrCodeStr: "
                 << matterCertificateInfo.qrCodeStr << "matterCertificateInfo.manualPairCode: "
                 << matterCertificateInfo.manualPairCode;
        qDebug() << QString("Port%1 ").arg(QString::number(myId)) << "qrCodeStr: " << value;
        QString qrInfo = "[" + matterCertificateInfo.qrCodeStr + "][" + matterCertificateInfo.manualPairCode + "]";
        if (!(value == qrInfo))
        {
            this->reburn();
        }
        else
        {
            qDebug() << QString("Port%1 ").arg(QString::number(myId))  << tr("matter配网码产测校验成功");
            serialTimer->stop();
            m_dataReadMode = RESET_MODE;
            emit signal_status(myId, tr("产测校验成功"));
            emit signal_activation(myId , burnInfo.dks);
        }
    }

    if (3 == checkCount)
    {
        qDebug() << QString(tr("Port%1 产测校验失败")).arg(QString::number(myId));
        serialTimer->stop();
        m_dataReadMode = RESET_MODE;
        slot_clearBurnInfo(myId);
        emit signal_status(myId, tr("产测校验失败"));
        if (!availableMac_list.contains(m_newWiFiMac))
        {
            storeMac(m_newWiFiMac);
        }
    }
    buffer.clear();
}
// 存储没用掉的mac
void Burning::storeMac(QByteArray mac)
{
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << __FUNCTION__;
    if (mac.isEmpty())
    {
        return;
    }
    else
    {
        QWriteLocker writeLock(&g_readWriteLock);
        availableMac_list.append(mac);
        this->refreshMacList();
    }
}
// 刷新mac的json文件
void Burning::refreshMacList()
{
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << __FUNCTION__;
    jsonFile jsFile(vitalFilePath + "mac.json");
    QJsonArray jsArray;
    // 遍历availableMac_list存储到jsarray中
    for (int i = 0; i < availableMac_list.size(); i++)
    {
        jsArray.append(QString(availableMac_list.at(i)));
    }
    jsFile.writeJsonKeyValue("macList", jsArray);
    jsFile.writeFile();
}
// 重新烧录
void Burning::reburn()
{
    qDebug() << QString("Port%1 ").arg(QString::number(myId)) << __FUNCTION__;
    checkCount++;
    m_dataReadMode = BURN_MODE;
    burnMode = 0;
    burnTime->start(500);
    this->qserialSendData("mac");
}
