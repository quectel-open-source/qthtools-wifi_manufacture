#ifndef QRWIDGET_H
#define QRWIDGET_H

#include <QWidget>

#include "switchbutton.h"
#include "QLabel"

namespace Ui {
class QrWidget;
}

class QrWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QrWidget(QWidget *parent = nullptr);
    ~QrWidget();
    bool getChecked(void);

private:
    Ui::QrWidget *ui;
    SwitchButton *QRBtn = nullptr;
    QLabel *qrLabel = nullptr;
};

#endif // QRWIDGET_H
