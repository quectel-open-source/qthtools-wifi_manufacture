#ifndef ENVSETTING_H
#define ENVSETTING_H

#include <QWidget>

namespace Ui {
class EnvSetting;
}

class EnvSetting : public QWidget
{
    Q_OBJECT

public:
    explicit EnvSetting(QWidget *parent = nullptr);
    ~EnvSetting();

signals:
    void signal_envInfo(QString ak, QString as, QString host);

private slots:
    void on_pushButton_reset_clicked();
    void on_pushButton_confirm_clicked();

private:
    Ui::EnvSetting *ui;
};

#endif // ENVSETTING_H
