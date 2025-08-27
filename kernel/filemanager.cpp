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
#include "filemanager.h"
#include "kernel/mmessagebox.h"
#include <QFileDialog>
#include <QDebug>

fileManager::fileManager()
{
    this->fileInfo.filevaule.clear();
    this->fileInfo.len = 0;
}
/**************************************************************************
** 功能	@brief : 是否存在文件夹
** 输入	@param :
** 输出	@retval:
***************************************************************************/
bool fileManager::isDirExist(QString fullPath)
{
    qInfo()<<__FUNCTION__;
    QDir dir(fullPath);
    if(dir.exists())
    {
      return true;
    }
    else
    {
       bool ok = dir.mkdir(fullPath);
       return ok;
    }
}
/**************************************************************************
** 功能	@brief : 打开文件
** 输入	@param :
**              url:文件路径
**              mode：
**                  FileKeepMode：保持打开
**                  FileBriefMode:使用完关闭
** 输出	@retval:
***************************************************************************/
bool fileManager::fileManagerOpen(QString url,int mode)
{
    qInfo()<<__FUNCTION__;
    QStringList urlList = url.split("/");
    QString urlDir;
    for (int i=0;i<urlList.size()-1;i++)//检查文件夹是否存在，不存在则创建
    {
        urlDir += urlList.at(i)+"/";
        this->isDirExist(urlDir);
    }
    this->fileInfo.myFile.setFileName(url);
    if(false == this->fileInfo.myFile.open(QIODevice::ReadWrite | QIODevice::Append))
    {
//        MMessageBox::information(0,QObject::tr("文件管理"),QObject::tr("无法打开文件"),QObject::tr("确认"));
        return false;
    }
    if(mode == FileBriefMode)
        this->fileManagerClose();
    return true;
}
/**************************************************************************
** 功能	@brief : 写入文件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
qint64 fileManager::fileManagerWrite(QByteArray data,int mode)
{
    qDebug()<< "fileManagerWrite1:";
    qDebug()<< "data、mode:" << data << mode;
    if(mode == FileBriefMode)
        this->fileInfo.myFile.open(QIODevice::ReadWrite | QIODevice::Append);
//    this->fileInfo.filevaule.append(data);
    bool retWrite = this->fileInfo.myFile.write(data);
    qDebug()<< "write end:" <<retWrite;
    bool ret =this->fileInfo.myFile.flush();
    if(mode == FileBriefMode)
        this->fileManagerClose();
    return ret;
}

QString fileManager::fileManagerReadLine(int mode)
{
    Q_UNUSED(mode);
    QByteArray data;
    if(!(data = this->fileInfo.myFile.readLine()).isEmpty())
    {
        return QString(data);
    }
    else
        return NULL;
}

/**************************************************************************
** 功能	@brief : 写入文件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
qint64 fileManager::fileManagerWrite(int startAddr,int len,QByteArray data,int mode)
{
    qDebug()<< "fileManagerWrite2:";
//    this->fileInfo.filevaule.append(data);
    if(mode == FileBriefMode)
        this->fileInfo.myFile.open(QIODevice::ReadWrite | QIODevice::Append);
    if(this->fileInfo.myFile.seek(startAddr) == false)
    {
        return 0;
    }
    this->fileInfo.myFile.write(data,len);
    bool ret =this->fileInfo.myFile.flush();
    if(mode == FileBriefMode)
        this->fileManagerClose();
    return ret;
}
/**************************************************************************
** 功能	@brief : 获取文件md5
** 输入	@param :
** 输出	@retval:
***************************************************************************/
QByteArray fileManager::fileManagerGetMd5(QString url)
{
    qInfo()<<__FUNCTION__;
    QCryptographicHash md(QCryptographicHash::Md5);
    QFile newFile(url);
    newFile.open(QIODevice::ReadOnly);
    char fileData[4096];
    qint64 fileReadRet = 0;
    qint64 filecurrentSum = 0;
    qint64 fileSize = newFile.size();
    while(fileSize > filecurrentSum)
    {
        fileReadRet = newFile.read(fileData,4096);
        md.addData(fileData,fileReadRet);
        filecurrentSum += fileReadRet;
        newFile.seek(filecurrentSum);
        memset(fileData,0,4096);
    }
//    qDebug()<<"md.result():"<<md.result();

    newFile.close();
//    md.addData(this->fileInfo.filevaule);
    return md.result();
}
/**************************************************************************
** 功能	@brief : 关闭文件
** 输入	@param :
** 输出	@retval:
***************************************************************************/
void fileManager::fileManagerClose()
{
    qInfo()<<__FUNCTION__;
//    if(!this->fileInfo)
//    {
        this->fileInfo.myFile.close();
//    }
}
/**************************************************************************
** 功能	@brief : 打开json文件并读取
** 输入	@param :
** 输出	@retval:
***************************************************************************/
bool fileManager::fileManagerOpenAndRead(QString fileType)
{
    qInfo()<<__FUNCTION__;
    QString fileName;
    if(fileType.isEmpty())
    {
        fileName = QFileDialog::getOpenFileName(NULL,QObject::tr("选取文件"),".","");
    }
    else
    {
        fileName = QFileDialog::getOpenFileName(NULL,QObject::tr("选取文件"),".",fileType+" (*."+fileType+")");
    }
    if(fileName.isEmpty())
        return false;
    this->fileInfo.myFile.setFileName(fileName);
    if(false == this->fileInfo.myFile.open(QIODevice::ReadOnly))
    {
        MMessageBox::information(0,QObject::tr("文件管理"),QObject::tr("无法打开文件"),QObject::tr("确认"));
        return false;
    }

    this->fileInfo.filevaule = this->fileInfo.myFile.readAll();
    this->fileInfo.myFile.close();
    fileInfo.len = fileInfo.filevaule.length();
    QCryptographicHash md(QCryptographicHash::Md5);
    md.addData(this->fileInfo.filevaule);
    this->fileInfo.md5 = md.result();
    this->fileInfo.type = fileType;
    return true;
}

/**************************************************************************
** 功能	@brief : 文件拷贝
** 输入	@param :
** 输出	@retval:
***************************************************************************/
bool fileManager::copyFileToPath(QString sourceFile ,QString toDir)
{
    qInfo()<<__FUNCTION__;
    QStringList strList = sourceFile.split("/");
    if(!QFile::copy(sourceFile, toDir+"/"+strList.constLast()))
    {
        return false;
    }
    return true;
}


