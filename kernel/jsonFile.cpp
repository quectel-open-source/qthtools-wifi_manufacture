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
#include "jsonFile.h"
#include "QFileInfo"
#include "QFile"
#include "QDir"

jsonFile::jsonFile(QString fileUrl)
{
    qInfo() << "jsonfileUrl: " << fileUrl;
    if (!fileUrl.isEmpty())
    {
        this->fileUrl = fileUrl;
    }
}

jsonFile::~jsonFile()
{

}

/**************************************************************************
** 功能	@brief : 读取配置文件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
bool jsonFile::readFile(void)
{
    qInfo()<<__FUNCTION__;
    QFile file(this->fileUrl);
    if (true == file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        QByteArray readData = file.readAll();
        file.close();
        return isJsonObject(readData);
    }
    qCritical()<<"read <"<<this->fileUrl<<"> fail";
    return false;
}


bool jsonFile::isJsonObject(const QByteArray &readData)
{
    if (readData.isEmpty())
    {
        return false;
    }
    QJsonParseError jsonErr;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(readData,&jsonErr);
    if (jsonErr.error == QJsonParseError::NoError)
    {
        if (jsonDoc.isObject())
        {
            this->jsonObj = jsonDoc.object();
            return true;
        }
    }
    return false;
}

/**************************************************************************
** 功能	@brief : 写配置文件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
bool jsonFile::writeFile(void)
{
    qInfo()<<__FUNCTION__;
    QFile file(fileUrl);
    if(true == file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate))
    {
        QJsonDocument rootDoc(this->jsonObj);
        QByteArray writeData = rootDoc.toJson();
        file.write(writeData);
        file.close();
        return true;
    }
    qCritical()<<"write <"<<file.fileName()<<"> fail";
    return false;
}


/**************************************************************************
** 功能	@brief : 读json值
** 输入	@param :
** 输出	@retval:
***************************************************************************/
void jsonFile::readJsonKeyValue(const QString &key, QJsonValue *value)
{
    *value = this->jsonObj.value(key);
}
/**************************************************************************
** 功能	@brief : 写json值
** 输入	@param :
** 输出	@retval:
***************************************************************************/
void jsonFile::writeJsonKeyValue(const QString &key, const QJsonValue &value)
{
    this->jsonObj.insert(key, value);
}

