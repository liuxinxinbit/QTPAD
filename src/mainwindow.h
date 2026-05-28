#pragma once

#include "virtualxboxcontroller.h"

#include <QWidget>

class QLabel;
class QPushButton;

class MainWindow final : public QWidget
{
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void triggerCombo(const QString &label, const QVector<int> &buttons);
    void setLeftStick(int x, int y);
    void setRightStick(int x, int y);

    VirtualXboxController controller_;
    QLabel *statusLabel_ = nullptr;
    QPushButton *lbYButton_ = nullptr;
    QPushButton *lbRbButton_ = nullptr;
    QLabel *leftValueLabel_ = nullptr;
    QLabel *rightValueLabel_ = nullptr;
};
