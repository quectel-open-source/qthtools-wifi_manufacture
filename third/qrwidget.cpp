#include "qrwidget.h"
#include "ui_qrwidget.h"

#include <QHBoxLayout>

QrWidget::QrWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QrWidget)
{
    ui->setupUi(this);
    this->QRBtn = new SwitchButton();
    this->QRBtn->resize(80,35);
    this->qrLabel = new QLabel(tr("配网码写入："));
    //创建布局
    QHBoxLayout* hlayout = new QHBoxLayout(this);
    hlayout->addWidget(qrLabel);
    hlayout->addWidget(this->QRBtn);
    this->setLayout(hlayout);
    this->setWindowTitle(tr("配网码设置"));
}

QrWidget::~QrWidget()
{
    delete ui;
}

bool QrWidget::getChecked()
{
    return this->QRBtn->getChecked();
}
