#include "envsetting.h"
#include "ui_envsetting.h"

EnvSetting::EnvSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EnvSetting)
{
    ui->setupUi(this);
}

EnvSetting::~EnvSetting()
{
    delete ui;
}

void EnvSetting::on_pushButton_reset_clicked()
{
    ui->lineEdit_as->clear();
    ui->lineEdit_ak->clear();
    ui->lineEdit_host->clear();
}

void EnvSetting::on_pushButton_confirm_clicked()
{
    emit signal_envInfo(ui->lineEdit_ak->text(), ui->lineEdit_as->text(), ui->lineEdit_host->text());
    this->close();
}
