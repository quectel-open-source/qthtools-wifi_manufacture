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
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QByteArray>
#include <QFile>
#include <QCryptographicHash>

typedef struct
{
    int rOffset;
    int wOffset;
    int lineCount;
    int lineCnt;
}hexFile_st;

typedef struct
{
    int rOffset;
    int wOffset;
    int pieceCount;
    int pieceCnt;
    int pieceSize;
}binFile_st;

typedef struct
{
    QFile myFile;
    QByteArray filevaule;
    int len;
    QByteArray md5;
    QString type;
}fileInfo_st;

enum
{
    FileKeepMode = 0,
    FileBriefMode,
};

class fileManager
{
public:
    fileManager();
    fileInfo_st fileInfo;
    bool fileManagerOpen(QString url,int mode=FileBriefMode);
    qint64 fileManagerWrite(QByteArray data,int mode=FileBriefMode);
    qint64 fileManagerWrite(int startAddr,int len,QByteArray data,int mode=FileBriefMode);
    QString fileManagerReadLine(int mode=FileKeepMode);
    void fileManagerClose();
    bool fileManagerOpenAndRead(QString fileType);
    QByteArray fileManagerGetMd5(QString url);
    bool isDirExist(QString fullPath);
    bool copyFileToPath(QString sourceFile ,QString toDir);
private:
};

#endif // FILEMANAGER_H
