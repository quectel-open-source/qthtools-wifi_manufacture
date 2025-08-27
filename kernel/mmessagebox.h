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
#ifndef MMESSAGEBOX_H
#define MMESSAGEBOX_H

#include <QDialog>

namespace Ui {
class MMessageBox;
}

class MMessageBox : public QDialog
{
    Q_OBJECT

public:
    enum Icon {
        NoIcon = 0,
        Information = 1,
        Warning = 2,
        Critical = 3,
        Question = 4
    };

    Q_ENUM(Icon)

    enum StandardButton{
        Yes = 1,
        No = -1,
        Null = 0,
    };
    typedef StandardButton Button;

    explicit MMessageBox(QWidget *parent = nullptr);
    MMessageBox(Icon icon, const QString &title, const QString &text, const QString& bottonYesText = nullptr, const QString& bottonNoText = nullptr);
    ~MMessageBox();
    void setIcon(Icon icon);
    void setWindowTitle(const QString &title);
    void setText(const QString &text);
    void setButtonText(int button, const QString &text);

public:
    static int information(QWidget *parent, const QString &title, const QString& text, const QString& bottonText);
    static int information(QWidget *parent, const QString &title, const QString& text, const QString& botton0Text, const QString& botton1Text, const QString& botton2Text);
    static int critical(QWidget *parent, const QString &title, const QString& text, const QString& bottonText);
    static int warning(QWidget *parent, const QString &title, const QString& text, const QString& bottonText);

private slots:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void on_pushButton_Cancel_clicked();
    void on_pushButton_OK_clicked();
    void on_pushButton_NULL_clicked();
    void closeEvent(QCloseEvent *event);

private:
    Ui::MMessageBox *ui;
    bool m_bDrag = false;
    QPoint mouseStartPoint;
    QPoint windowTopLeftPoint;
};

#endif // MMESSAGEBOX_H
