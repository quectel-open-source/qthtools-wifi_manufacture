#ifndef BURNING_H
#define BURNING_H

#include <QObject>

#include "kernel/serial.h"
#include "kernel/httpclient.h"
#include "kernel/jsonFile.h"

typedef struct{
    QString pk;
    QString dks;
#if 0
//  old:
    QString ak_old = "24b9rucZxRLf2WaWBXagrvFh";
    QString as_old = "6A3XYyTqqfPKSSurG1fd6sYM7VQGpjcvkV1gMC9w";
// develop:
//    QString ak = "1B8V5qU82AusQN443wnB8qxj6hnPUaKGbMcGCDUb";
//    QString as = "69Hj8BFq7Pp2F9Hzy8FwCmuvqVADGG8KzByR6rdV";
    // fat2:
    QString ak = "24b9rucZxRdsN3eNQvSM3YWb";
    QString as = "6ANNJvabgvz7wCa36wvswPLd4XkxMG4QCDddMZXx";

#endif
    // production:
    QString ak = "24b9rucZxRLf2WaWBXagrvFh";
    QString as = "6A3XYyTqqfPKSSurG1fd6sYM7VQGpjcvkV1gMC9w";
    int random;
    int authMode = 0;
    QString sign;
    QString publicKey;
    QString authCode = "";
    QJsonObject proInfoObject;
}MatterBurningInfo;

typedef struct{
    int discriminator;
    int vendorId;
    int productId;
    int iterationCount;
    QString salt;
    QString verifier;
    int passcode; //pincode
    QString vendorName;
    QString productName;
    QString paiCerHexStr;
    QString cdCertHexStr;
    QString dacCertHexStr;
    //免开发信息
    QString config;
    //二维码信息
    QString qrCodeStr;
    QString manualPairCode;
}MatterCertificateInfo;

enum
{
  BURN_MODE = 0,
  CHECK_MODE,
  RESET_MODE
};

class Burning : public QObject
{
    Q_OBJECT
public:
    explicit Burning(QString comName, int baud,  int tid, QObject *parent = nullptr);
    ~Burning();
    void addLogs(QString log, bool isrecv);
    void qserialSendData(QString sendData);
    char convertHexChar(char ch);
    QString hexStrCheckSum(const QString &str);
    void storeEnvInfo(QString ak, QString as, QString host);

signals:
    void signal_status(int tid, QString status);
    void signal_activation(int tid, QString dks);
    void getQrChecked();

public slots:
    void slot_closePort();
    void slot_burnTimeout();
    void slot_replyStatus();
    void slot_proInfo(QJsonObject proInfo_object);
    void slot_storeChecked(bool isChecked);
    void slot_serialTimeout();
    void slot_clearBurnInfo(int portId);

private slots:
    void slot_dataRead(QByteArray serialBuffer);

private:
    qSerial *qserial = nullptr;
    HttpClient *httpClient = nullptr;
    QTimer *burnTime = nullptr;
    QTimer *serialTimer = nullptr;
    int burnMode = 0;
    int checkMode = 0;
    bool qrChecked = false;
    QString olderData = "";
    QString recvAllData = "";
    int myId;
    int checkCount = 0;
    bool isOpen;
    bool isCheck = false;
    QString pk_old;
    QString dacpubBuf;
    QString buffer = "";
    QString authOc;
    MatterBurningInfo burnInfo;
    MatterCertificateInfo matterCertificateInfo;
    QString m_host = "";
    QByteArray m_newWiFiMac = "";
    qint8 m_macSetCount = 0;
    int m_dataReadMode = 0;
    QString m_oldMac;
    QString m_defaultHost = "https://iot-api.quectelcn.com";

    void mac_check_burn(QString recvData);
    QByteArray generateMacInRange(const QByteArray &endMac);
    void configFile_burn();
    void genCertInfo();
    void devRegister();
    void qrInfoBurn();
    void dataBurn(QByteArray serialBuffer);
    void dataCheck(QByteArray serialBuffer);
    void storeMac(QByteArray mac);
    void refreshMacList(void);
    void reburn(void);
};

#endif // BURNING_H
