/*
Copyright (C) 2023  Quectel Wireless Solutions Co.,Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "upgradeprompts.h"
#include "ui_upgradeprompts.h"
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>

upgradePrompts::upgradePrompts(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::upgradePrompts)
{
    ui->setupUi(this);
    //去除窗体边框
    this->setWindowFlags(Qt::Window|Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    //窗体关闭信号
    connect(ui->pushButton_close, &QPushButton::clicked, [=]() {close(); });
    this->setWindowModality(Qt::ApplicationModal);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor("#C0C0C0"));
    shadow->setBlurRadius(15);
    shadow->setOffset(5);
    ui->widget->setGraphicsEffect(shadow);
    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时释放窗口
    ui->label_force->hide();
    ui->textEdit->setReadOnly(true);
    ui->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    ui->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

upgradePrompts::~upgradePrompts()
{
    delete ui;
}
void upgradePrompts::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    done(StandardButton::No);
}
/**************************************************************************
** 功能	@brief :  鼠标按下事件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
void upgradePrompts::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    //判断是否是鼠标左键信号，是否处于最大化状态
    if(event->button() == Qt::LeftButton &&  !this->isMaximized())
    {
        if (ui->widget_windowTitle->geometry().contains(this->mapFromGlobal(QCursor::pos())))
        {
            m_bDrag = true;
            mouseStartPoint = event->globalPos();
            windowTopLeftPoint = this->frameGeometry().topLeft();
        }
    }
}

void upgradePrompts::mouseMoveEvent(QMouseEvent *event)
{
    if(m_bDrag)
    {
        //获得鼠标移动的距离
        QPoint distance = event->globalPos() - mouseStartPoint;
        //QPoint distance = event->pos() - mouseStartPoint;
        //改变窗口的位置
        this->move(windowTopLeftPoint + distance);
    }
}

void upgradePrompts::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_bDrag = false;
    }
}

void upgradePrompts::on_pushButton_cancel_clicked()
{
    done(StandardButton::No);
}

void upgradePrompts::on_pushButton_ok_clicked()
{
    done(StandardButton::Yes);
}

void upgradePrompts::setSoftwareVersionInfo(QString text)
{
    ui->label_info->setText(text);
}

void upgradePrompts::setUpdateInfo(QString text)
{
    QString newText = "<p style='line-height:20px; width:100% ; white-space: pre-wrap; '>" + text + "</p>";
    ui->textEdit->setText(newText);
}

void upgradePrompts::isForceDownload()
{
    ui->label_force->show();
}
