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
#include "serial.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
qSerial::qSerial(QObject *parent) : QSerialPort(parent)
{
    connect(&this->portGetTimer, &QTimer::timeout, this, &qSerial::portListGetHandle);
    connect(&this->serial, &QIODevice::readyRead, this, &qSerial::dataReadHandle);
    this->isFirst = true;
}

qSerial::~qSerial()
{
    if(this->serial.isOpen())
    {
        this->serial.close();
    }
}

void qSerial::portListPeriodSet(int32_t period)
{
    qInfo()<<__FUNCTION__;
    if(this->portGetTimer.isActive())
    {
        this->portGetTimer.stop();
    }
    this->portGetTimer.start(period);
}

void qSerial::portListGetHandle(void)
{
//    qInfo()<<__FUNCTION__;
    QList<QString> portList;
    for(const QSerialPortInfo &info:this->availablePorts())
    {
        portList.append(info.portName()+"-"+info.description());
    }
    if(this->oldPortList != portList || this->isFirst == true)
    {
        this->isFirst = false;
        this->oldPortList = portList;
        emit portListNoticSignal(portList);
    }
}

void qSerial::dataReadHandle()
{
    QByteArray readData = this->serial.readAll();
    emit dataReadNoticSignal(readData);
}

bool qSerial::serialOpen(QString portName,int baundrate,int dataBits,int parity,int stopBits,int control)
{
    qInfo()<<__FUNCTION__;
    this->serial.setParent(this);
    bool isOpen = false;
    if(!portName.isNull())
    {
        this->serial.setPortName(portName.split("-").at(0));
        this->serial.setBaudRate(baundrate);
        this->serial.setDataBits(DataBits(dataBits));
        this->serial.setParity(Parity(parity));
        this->serial.setStopBits(StopBits(stopBits));
        this->serial.setFlowControl(FlowControl(control));
        isOpen =  this->serial.open(QIODevice::ReadWrite);
    }
    return isOpen;
}
void qSerial::SerialClose()
{
    qInfo()<<__FUNCTION__;
    if(this->serial.isOpen())
    {
        this->serial.close();
    }
}

bool qSerial::serialIsOpen()
{
    return this->serial.isOpen();
}

qint64 qSerial::SerialSend(QByteArray data)
{
    if(this->serial.isOpen())
    {
        return this->serial.write(data, data.length());
    }
    return -1;
}

