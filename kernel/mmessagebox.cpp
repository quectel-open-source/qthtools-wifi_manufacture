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
#include "mmessagebox.h"
#include "ui_mmessagebox.h"
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QDebug>

MMessageBox::MMessageBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MMessageBox)
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

    ui->pushButton_NULL->hide();
    this->setAttribute(Qt::WA_DeleteOnClose); //关闭时释放窗口
    ui->label_messBoxText->setWordWrap(true);
}

MMessageBox::MMessageBox(Icon icon, const QString &title, const QString &text, const QString& bottonYesText, const QString& bottonNoText) :
    ui(new Ui::MMessageBox)
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

    this->setWindowTitle(title);
    ui->label_windowTitle->setText(title);
    ui->label_messBoxText->setWordWrap(true);
    ui->label_messBoxText->setText("<p style='line-height:20px;'>" + text + "</p>");
    ui->pushButton_NULL->hide();

    if (bottonYesText == NULL)
    {
        ui->pushButton_OK->hide();
    }
    else
    {
        ui->pushButton_OK->setText(bottonYesText);
    }

    if (bottonNoText == NULL)
    {
        ui->pushButton_Cancel->hide();
    }
    else
    {
        ui->pushButton_Cancel->setText(bottonNoText);
    }

    if (icon == Icon::NoIcon)
    {
        ui->label_messBoxPng->hide();
    }
    else if (icon == Icon::Warning)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_warning.png);");
    }
    else if (icon == Icon::Critical)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_error.png);");
    }
    else if (icon == Icon::Question)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_question.png);");
    }
    else if (icon == Icon::Information)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_information.png);");
    }
}

MMessageBox::~MMessageBox()
{
    delete ui;
}

void MMessageBox::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    done(StandardButton::No);
}
/**************************************************************************
** 功能	@brief :  鼠标按下事件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
void MMessageBox::mousePressEvent(QMouseEvent *event)
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

void MMessageBox::mouseMoveEvent(QMouseEvent *event)
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

void MMessageBox::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_bDrag = false;
    }
}

void MMessageBox::on_pushButton_Cancel_clicked()
{
    done(StandardButton::No);
}

void MMessageBox::on_pushButton_OK_clicked()
{
    done(StandardButton::Yes);
}

void MMessageBox::on_pushButton_NULL_clicked()
{
    done(StandardButton::Null);
}

int MMessageBox::information(QWidget *parent, const QString &title, const QString& text, const QString& bottonText)
{
    MMessageBox *newMessageBox = new MMessageBox(parent);
    newMessageBox->ui->label_windowTitle->setText(title);
    newMessageBox->setWindowTitle(title);
    newMessageBox->ui->label_messBoxText->setText("<p style='line-height:20px;'>" + text + "</p>");
    newMessageBox->ui->pushButton_OK->setText(bottonText);
    newMessageBox->ui->pushButton_Cancel->hide();
    newMessageBox->ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_information.png);");
    newMessageBox->exec();
    return 1;
}

int MMessageBox::information(QWidget *parent, const QString &title, const QString& text, const QString& botton0Text, const QString& botton1Text, const QString& botton2Text)
{
    MMessageBox *newMessageBox = new MMessageBox(parent);
    newMessageBox->ui->label_windowTitle->setText(title);
    newMessageBox->setWindowTitle(title);
    newMessageBox->ui->label_messBoxText->setText("<p style='line-height:20px;'>" + text + "</p>");
    newMessageBox->ui->pushButton_OK->setText(botton0Text);
    newMessageBox->ui->pushButton_NULL->show();
    newMessageBox->ui->pushButton_NULL->setText(botton1Text);
    newMessageBox->ui->pushButton_Cancel->setText(botton2Text);
    newMessageBox->ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_information.png);");
    int execNum = newMessageBox->exec();
    if (execNum == MMessageBox::Yes)
    {
        return 0;
    }
    else if (execNum == MMessageBox::Null)
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

int MMessageBox::critical(QWidget *parent, const QString &title, const QString& text, const QString& bottonText)
{
    MMessageBox *newMessageBox = new MMessageBox(parent);
    newMessageBox->ui->label_windowTitle->setText(title);
    newMessageBox->setWindowTitle(title);
    newMessageBox->ui->label_messBoxText->setText("<p style='line-height:20px;'>" + text + "</p>");
    newMessageBox->ui->pushButton_OK->setText(bottonText);
    newMessageBox->ui->pushButton_Cancel->hide();
    newMessageBox->ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_error.png);");
    newMessageBox->exec();
    return 1;
}

int MMessageBox::warning(QWidget *parent, const QString &title, const QString& text, const QString& bottonText)
{
    MMessageBox *newMessageBox = new MMessageBox(parent);
    newMessageBox->ui->label_windowTitle->setText(title);
    newMessageBox->setWindowTitle(title);
    newMessageBox->ui->label_messBoxText->setText("<p style='line-height:20px;'>" + text + "</p>");
    newMessageBox->ui->pushButton_OK->setText(bottonText);
    newMessageBox->ui->pushButton_Cancel->hide();
    newMessageBox->ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_warning.png);");
    newMessageBox->exec();
    return 1;
}

void MMessageBox::setIcon(Icon icon)
{
    if (icon == Icon::NoIcon)
    {
        ui->label_messBoxPng->hide();
    }
    else if (icon == Icon::Warning)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_warning.png);");
    }
    else if (icon == Icon::Critical)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_error.png);");
    }
    else if (icon == Icon::Question)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_question.png);");
    }
    else if (icon == Icon::Information)
    {
        ui->label_messBoxPng->setStyleSheet("image: url(:/png/m_information.png);");
    }
}

void MMessageBox::setWindowTitle(const QString &title)
{
    ui->label_windowTitle->setText(title);
//    this->setWindowTitle(title);
}

void MMessageBox::setText(const QString &text)
{
    ui->label_messBoxText->setText("<p style='line-height:20px;'>" + text + "</p>");
}

void MMessageBox::setButtonText(int button, const QString &text)
{
    if (button == MMessageBox::Yes)
    {
        ui->pushButton_OK->setText(text);
    }
    else if (button == MMessageBox::No)
    {
        ui->pushButton_Cancel->setText(text);
    }
}
