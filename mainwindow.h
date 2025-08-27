#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "kernel/kernel_include.h"
#include "LogModule/openlog.h"
#include "QSimpleUpdater/toolupdate.h"
#include "third/qrwidget.h"
#include "app/envsetting.h"
#include "app/burning.h"

#include <QMainWindow>
#include <QMenu>
#include <QAction>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QStandardPaths>

#define ENABLE_ENV_SET 0

enum
{
    Order_Auth_Code_Not_Exist = 14051,
    Order_Auth_Info_Missing_Info = 14052,
    Change_Authorization_Num_Fail = 14053,
    Param_Check_Error = 50005,
    DK_Not_Legal = 8912009,
    Device_Activation_Exception = 8912010,
    Authorizations_Num_Insufficient = 8912011
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
    bool isOnline();
    void apiMessage(int errorCode);

signals:
    void closePort();
    void queryStatus();
    void signal_proInfo(QJsonObject proInfo);
    void replyQrChecked(bool  isChecked);
    void signal_clearBurnInfo(int portId);

public slots:
    void slot_status(int tid, QString stauts);
    void slot_activateLicense(int tid, QString dks);
    void slot_storeEnvInfo(QString ak, QString as, QString host);

private:
    Ui::MainWindow *ui;
    qSerial *qserial = nullptr;
    QMenu *m_menuSetting = nullptr;
    QMenu *m_menuLog     = nullptr;
    QMenu *m_menuHelp    = nullptr;
    OpenLog *m_openLog = nullptr;
    toolUpdate *m_updateVersion = nullptr;
    QrWidget *m_qrWidget = nullptr;
    QThread *m_thread1 = nullptr;
    Burning *m_burning1 = nullptr;
    QThread *m_thread2 = nullptr;
    Burning *m_burning2 = nullptr;
    QThread *m_thread3 = nullptr;
    Burning *m_burning3 = nullptr;
    QThread *m_thread4 = nullptr;
    Burning *m_burning4 = nullptr;
    QElapsedTimer *m_timer = nullptr;
    EnvSetting *m_envSetting = nullptr;
    QString m_host = "";
    int m_authorizableNum = 0;
    QString m_defaultHost = "https://iot-api.quectelcn.com";

    void runToStop();
    void addMenuBar();
    void uiInit();
    bool getProInfo();
    void startMultiPort();
    void stopBurning();
    void clearLineEdit();
    void readMac(void);
    void readAuthCode(void);
    void writeAuthCode(const QString &authCode);

private slots:
    void port_list_noticSlot(QList<QString> portList);
    void on_pushButton_authCode_clicked();
    void on_pushButton_run_clicked();
    void action_handle(QAction *action);
};
#endif // MAINWINDOW_H
