#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class VirtualXboxController final : public QObject
{
public:
    explicit VirtualXboxController(QObject *parent = nullptr);
    ~VirtualXboxController() override;

    bool isReady() const;
    QString errorString() const;

    bool tapButton(int buttonCode, int holdMs = 80);
    bool tapCombo(const QVector<int> &buttons, int holdMs = 80);
    bool setLeftStick(int x, int y);
    bool setRightStick(int x, int y);
    bool setDPad(int x, int y);
    bool tapTrigger(int axisCode, int value = 255, int holdMs = 80);

private:
    bool initialize();
    bool setAxisValue(int axisCode, int value);
    bool pressButtons(const QVector<int> &buttons);
    bool releaseButtons(const QVector<int> &buttons);
    bool sendEvent(unsigned short type, unsigned short code, int value);
    bool sync();

    int fd_ = -1;
    QString errorString_;
};
