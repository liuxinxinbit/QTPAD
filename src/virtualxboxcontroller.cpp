#include "virtualxboxcontroller.h"

#include <QTimer>

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <sys/time.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>

namespace
{
constexpr const char kDeviceName[] = "QtPad Xbox Virtual Controller";

void setError(QString *target, const char *context)
{
    const QString message = QString::fromUtf8("%1: %2").arg(QLatin1String(context), QLatin1String(std::strerror(errno)));
    if (target != nullptr) {
        *target = message;
    }
}
} // namespace

VirtualXboxController::VirtualXboxController(QObject *parent)
    : QObject(parent)
{
    initialize();
}

VirtualXboxController::~VirtualXboxController()
{
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
    }
}

bool VirtualXboxController::isReady() const
{
    return fd_ >= 0;
}

QString VirtualXboxController::errorString() const
{
    return errorString_;
}

bool VirtualXboxController::initialize()
{
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        setError(&errorString_, "open(/dev/uinput)");
        return false;
    }

    const auto setBit = [this](unsigned long request, int code, QString *error) -> bool {
        if (ioctl(fd_, request, code) < 0) {
            setError(error, "ioctl(uinput bit)");
            return false;
        }
        return true;
    };

    if (!setBit(UI_SET_EVBIT, EV_KEY, &errorString_) ||
        !setBit(UI_SET_EVBIT, EV_SYN, &errorString_) ||
        !setBit(UI_SET_EVBIT, EV_ABS, &errorString_)) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    const std::array<int, 11> keyCodes = {
        BTN_SOUTH,
        BTN_EAST,
        BTN_WEST,
        BTN_NORTH,
        BTN_TL,
        BTN_TR,
        BTN_SELECT,
        BTN_START,
        BTN_THUMBL,
        BTN_THUMBR,
        BTN_MODE,
    };

    for (int code : keyCodes) {
        if (!setBit(UI_SET_KEYBIT, code, &errorString_)) {
            close(fd_);
            fd_ = -1;
            return false;
        }
    }

    const std::array<int, 8> absCodes = {ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ, ABS_HAT0X, ABS_HAT0Y};
    for (int code : absCodes) {
        if (!setBit(UI_SET_ABSBIT, code, &errorString_)) {
            close(fd_);
            fd_ = -1;
            return false;
        }
    }

    uinput_user_dev device {};
    std::snprintf(device.name, UINPUT_MAX_NAME_SIZE, "%s", kDeviceName);
    device.id.bustype = BUS_USB;
    device.id.vendor = 0x045e;
    device.id.product = 0x028e;
    device.id.version = 0x0110;

    device.absmin[ABS_X] = -32768;
    device.absmax[ABS_X] = 32767;
    device.absmin[ABS_Y] = -32768;
    device.absmax[ABS_Y] = 32767;
    device.absmin[ABS_RX] = -32768;
    device.absmax[ABS_RX] = 32767;
    device.absmin[ABS_RY] = -32768;
    device.absmax[ABS_RY] = 32767;
    device.absmin[ABS_Z] = 0;
    device.absmax[ABS_Z] = 255;
    device.absmin[ABS_RZ] = 0;
    device.absmax[ABS_RZ] = 255;
    device.absmin[ABS_HAT0X] = -1;
    device.absmax[ABS_HAT0X] = 1;
    device.absmin[ABS_HAT0Y] = -1;
    device.absmax[ABS_HAT0Y] = 1;

    if (write(fd_, &device, sizeof(device)) < 0) {
        setError(&errorString_, "write(uinput_user_dev)");
        close(fd_);
        fd_ = -1;
        return false;
    }

    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        setError(&errorString_, "ioctl(UI_DEV_CREATE)");
        close(fd_);
        fd_ = -1;
        return false;
    }

    errorString_.clear();
    return true;
}

bool VirtualXboxController::setAxisValue(int axisCode, int value)
{
    if (!sendEvent(EV_ABS, static_cast<unsigned short>(axisCode), value)) {
        return false;
    }
    return sync();
}

bool VirtualXboxController::tapButton(int buttonCode, int holdMs)
{
    return tapCombo(QVector<int>{buttonCode}, holdMs);
}

bool VirtualXboxController::sendEvent(unsigned short type, unsigned short code, int value)
{
    if (fd_ < 0) {
        errorString_ = QStringLiteral("virtual controller is not ready");
        return false;
    }

    input_event event {};
    gettimeofday(&event.time, nullptr);
    event.type = type;
    event.code = code;
    event.value = value;

    if (write(fd_, &event, sizeof(event)) < 0) {
        setError(&errorString_, "write(input_event)");
        return false;
    }

    return true;
}

bool VirtualXboxController::sync()
{
    return sendEvent(EV_SYN, SYN_REPORT, 0);
}

bool VirtualXboxController::setLeftStick(int x, int y)
{
    if (!sendEvent(EV_ABS, ABS_X, x) || !sendEvent(EV_ABS, ABS_Y, y)) {
        return false;
    }
    return sync();
}

bool VirtualXboxController::setRightStick(int x, int y)
{
    if (!sendEvent(EV_ABS, ABS_RX, x) || !sendEvent(EV_ABS, ABS_RY, y)) {
        return false;
    }
    return sync();
}

bool VirtualXboxController::setDPad(int x, int y)
{
    if (!sendEvent(EV_ABS, ABS_HAT0X, x) || !sendEvent(EV_ABS, ABS_HAT0Y, y)) {
        return false;
    }
    return sync();
}

bool VirtualXboxController::tapTrigger(int axisCode, int value, int holdMs)
{
    if (!setAxisValue(axisCode, value)) {
        return false;
    }

    QTimer::singleShot(holdMs, this, [this, axisCode]() {
        setAxisValue(axisCode, 0);
    });

    return true;
}

bool VirtualXboxController::pressButtons(const QVector<int> &buttons)
{
    for (int code : buttons) {
        if (!sendEvent(EV_KEY, static_cast<unsigned short>(code), 1)) {
            return false;
        }
    }

    return sync();
}

bool VirtualXboxController::releaseButtons(const QVector<int> &buttons)
{
    for (int code : buttons) {
        if (!sendEvent(EV_KEY, static_cast<unsigned short>(code), 0)) {
            return false;
        }
    }

    return sync();
}

bool VirtualXboxController::tapCombo(const QVector<int> &buttons, int holdMs)
{
    if (!pressButtons(buttons)) {
        return false;
    }

    QTimer::singleShot(holdMs, this, [this, buttons]() {
        releaseButtons(buttons);
    });

    return true;
}
