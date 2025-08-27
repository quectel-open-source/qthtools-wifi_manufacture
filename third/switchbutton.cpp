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
//#pragma execution_character_set("utf-8")
#include "switchbutton.h"
#include <QPainter>
#include <QDebug>

SwitchButton::SwitchButton(QWidget *parent) : QWidget(parent)
{
    m_space = 2;
    m_radius = 5;
    m_checked = false;
    m_showText = true;
    m_showText = false;
    m_animation = true;

    m_bgColorOn = QColor(47, 105, 235);
    m_bgColorOff = QColor(107, 119, 140);

    m_sliderColorOn = QColor(255, 255, 255);
    m_sliderColorOff = QColor(255, 255, 255);

    m_textColor = QColor(255, 255, 255);

//    m_textOn = "开启";
//    m_textOff = "关闭";

    m_step = 0;
    m_startX = 0;
    m_endX = 0;

    useFlag = true;

    m_timer = new QTimer(this);
    m_timer->setInterval(30);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(UpdateValue()));
    connect(this,SIGNAL(checkedChanged(bool)),this,SLOT(buttonMove(bool)));
}


void SwitchButton::drawBackGround(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    QColor bgColor = m_checked ? m_bgColorOn : m_bgColorOff;
    if (isEnabled()) {
        bgColor.setAlpha(140);
    }

    painter->setBrush(bgColor);

    QRect rect(0, (height()/2)-10, 40, 20);
    int side = qMin(40, 20);

    //左侧半圆
    QPainterPath path1;
    path1.addEllipse(rect.x(), rect.y(), side, side);

    //右侧半圆
    QPainterPath path2;
    path2.addEllipse(rect.width() - side, rect.y(), side, side);

    //中间的矩形
    QPainterPath path3;
    path3.addRect(rect.x() + side / 2, rect.y(), rect.width() - side, 20);

    QPainterPath path = path1 + path2 + path3;
    painter->drawPath(path);

    //绘制文本

    //滑块半径
    int sliderWidth = qMin(40, 20) - m_space * 2 - 5;
    if (m_checked){
        QRect textRect(0, 0, 40 - sliderWidth, 20);
        painter->setPen(QPen(m_textColor));
        painter->drawText(textRect, Qt::AlignCenter, m_textOn);
    } else {
        QRect textRect(sliderWidth, 0, 40 - sliderWidth, 20);
        painter->setPen(QPen(m_textColor));
        painter->drawText(textRect, Qt::AlignCenter, m_textOff);
    }

    painter->restore();
}

void SwitchButton::drawSlider(QPainter *painter)
{
    painter->save();
    painter->setPen(Qt::NoPen);

    QColor color = m_checked ? m_sliderColorOn : m_sliderColorOff;

    painter->setBrush(QBrush(color));

    int sliderWidth = qMin(40, 20) - m_space * 2;
    QRect rect(m_space + m_startX, (height()/2)-8, sliderWidth, sliderWidth);
    painter->drawEllipse(rect);

    painter->restore();
}

void SwitchButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    //启用反锯齿
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    //绘制背景
    drawBackGround(&painter);

    //绘制滑块
    drawSlider(&painter);
}

void SwitchButton::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);

    if(useFlag)
    {
        m_checked = !m_checked;
        emit checkedChanged(m_checked);
    }
    else
    {
        return;
    }

    //计算步长
    m_step = 40 / 10;

    //计算滑块X轴终点坐标
    if (m_checked) {
        m_endX = 40 - 20;
    } else {
        m_endX = 0;
    }

    //判断是否使用动画
    if (m_animation) {
        m_timer->start();
    } else{
        m_startX = m_endX;
        update();
    }
}

void SwitchButton::buttonMove(bool checked)
{
    //计算步长
    m_step = 40 / 10;

    //计算滑块X轴终点坐标
    if (checked) {
        m_endX = 40 - 20;
    } else {
        m_endX = 0;
    }

    //判断是否使用动画
    if (m_animation) {
        m_timer->start();
    } else{
        m_startX = m_endX;
        update();
    }
}

void SwitchButton::setEnable(bool flag)
{
    if(useFlag != flag)
    {
        useFlag = flag;
        if(useFlag)
        {
            this->setStyleSheet("background-color: white;background:transparent;");
        }
        else
        {
            this->setStyleSheet("background-color: yellow;background:transparent;");
        }
    }
}

void SwitchButton::UpdateValue()
{
    if (m_checked) {
        if (m_startX < m_endX) {
            m_startX += m_step;
        } else {
            m_startX = m_endX;
            m_timer->stop();
        }
    } else {
        if (m_startX > m_endX) {
            m_startX -= m_step;
        } else {
            m_startX = m_endX;
            m_timer->stop();
        }
    }

    update();
}

int SwitchButton::space() const
{
    return m_space;
}

int SwitchButton::radius() const
{
    return m_radius;
}

bool SwitchButton::checked() const
{
    return m_checked;
}

bool SwitchButton::showText() const
{
    return m_showText;
}

bool SwitchButton::showCircle() const
{
    return m_showCircle;
}

bool SwitchButton::animation() const
{
    return m_animation;
}

QColor SwitchButton::bgColorOn() const
{
    return m_bgColorOn;
}

QColor SwitchButton::bgColorOff() const
{
    return m_bgColorOff;
}

QColor SwitchButton::sliderColorOn() const
{
    return m_sliderColorOn;
}

QColor SwitchButton::sliderColorOff() const
{
    return m_sliderColorOff;
}

QColor SwitchButton::textColor() const
{
    return m_textColor;
}

QString SwitchButton::textOn() const
{
    return m_textOn;
}

QString SwitchButton::textOff() const
{
    return m_textOff;
}

int SwitchButton::step() const
{
    return m_step;
}

int SwitchButton::startX() const
{
    return m_startX;
}

int SwitchButton::endX() const
{
    return m_endX;
}

void SwitchButton::setSpace(int space)
{
    if (m_space != space) {
        m_space = space;
        update();
    }
}

void SwitchButton::setRadius(int radius)
{
    if (m_radius != radius) {
        m_radius = radius;
        update();
    }
}

void SwitchButton::setChecked(bool checked)
{
    if (m_checked != checked) {
        m_checked = checked;
        emit checkedChanged(checked);
        update();
    }
}

void SwitchButton::setShowText(bool show)
{
    if (m_showText != show) {
        m_showText = show;
        update();
    }
}

void SwitchButton::setShowCircle(bool show)
{
    if (m_showCircle != show) {
        m_showCircle = show;
        update();
    }
}

void SwitchButton::setAnimation(bool ok)
{
    if (m_animation != ok) {
        m_animation = ok;
        update();
    }
}

void SwitchButton::setBgColorOn(const QColor &color)
{
    if (m_bgColorOn != color) {
        m_bgColorOn = color;
        update();
    }
}

void SwitchButton::setBgColorOff(const QColor &color)
{
    if (m_bgColorOff != color) {
        m_bgColorOff = color;
        update();
    }
}

void SwitchButton::setSliderColorOn(const QColor &color)
{
    if (m_sliderColorOn != color) {
        m_sliderColorOn = color;
        update();
    }
}

void SwitchButton::setSliderColorOff(const QColor &color)
{
    if (m_sliderColorOff != color) {
        m_sliderColorOff = color;
        update();
    }
}

void SwitchButton::setTextColor(const QColor &color)
{
    if (m_textColor != color) {
        m_textColor = color;
        update();
    }
}

void SwitchButton::setText(const QString &textOn, const QString &textOff)
{
    if (m_textOn != textOn) {
        m_textOn = textOn;
        update();
    }
    if (m_textOff != textOff) {
        m_textOff = textOff;
        update();
    }
}

