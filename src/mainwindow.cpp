#include "mainwindow.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

#include <linux/input-event-codes.h>

#include <cmath>
#include <functional>

namespace
{
constexpr int kStickMin = -32768;
constexpr int kStickMax = 32767;

QVector<int> lbYCombo()
{
    return {BTN_TL, BTN_WEST};
}

QVector<int> lbRbCombo()
{
    return {BTN_TL, BTN_TR};
}

class JoystickPad final : public QWidget
{
public:
    explicit JoystickPad(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(220, 220);
        setMouseTracking(true);
    }

    void setOnValueChanged(const std::function<void(int, int)> &callback)
    {
        callback_ = callback;
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QPointF center = rect().center();
        const qreal radius = static_cast<qreal>(qMin(width(), height())) * 0.42;
        const QPointF knobCenter = center + QPointF(normalizedX_ * radius, normalizedY_ * radius);

        painter.setPen(QPen(QColor("#334155"), 2));
        painter.setBrush(QColor("#0b1222"));
        painter.drawEllipse(center, radius, radius);

        painter.setPen(QPen(QColor("#1f2a44"), 1));
        painter.drawLine(QPointF(center.x() - radius, center.y()), QPointF(center.x() + radius, center.y()));
        painter.drawLine(QPointF(center.x(), center.y() - radius), QPointF(center.x(), center.y() + radius));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#38bdf8"));
        painter.drawEllipse(knobCenter, radius * 0.22, radius * 0.22);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        dragging_ = true;
        updateFromPos(event->pos());
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!dragging_) {
            return;
        }
        updateFromPos(event->pos());
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        dragging_ = false;
        normalizedX_ = 0.0;
        normalizedY_ = 0.0;
        emitValueChanged();
        update();
    }

private:
    void updateFromPos(const QPoint &pos)
    {
        const QPointF center = rect().center();
        const qreal radius = static_cast<qreal>(qMin(width(), height())) * 0.42;
        if (radius <= 1.0) {
            return;
        }

        qreal nx = (static_cast<qreal>(pos.x()) - center.x()) / radius;
        qreal ny = (static_cast<qreal>(pos.y()) - center.y()) / radius;
        const qreal length = std::sqrt(nx * nx + ny * ny);
        if (length > 1.0) {
            nx /= length;
            ny /= length;
        }

        normalizedX_ = nx;
        normalizedY_ = ny;
        emitValueChanged();
        update();
    }

    void emitValueChanged()
    {
        if (!callback_) {
            return;
        }

        const int x = static_cast<int>(normalizedX_ * static_cast<qreal>(kStickMax));
        const int y = static_cast<int>(normalizedY_ * static_cast<qreal>(kStickMax));
        callback_(x, y);
    }

    std::function<void(int, int)> callback_;
    bool dragging_ = false;
    qreal normalizedX_ = 0.0;
    qreal normalizedY_ = 0.0;
};
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("QtPad Virtual Xbox 360 Controller"));
    setMinimumSize(760, 520);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("QtPad Stick Panel"));
    title->setStyleSheet(QStringLiteral("font-size: 28px; font-weight: 800;"));

    auto *subtitle = new QLabel(QStringLiteral("简化模式：左右摇杆连续模拟 + 组合键 LB+Y / LB+RB。X=水平轴，Y=垂直轴。"));
    subtitle->setWordWrap(true);

    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    statusLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QVector<QWidget *> controls;

    auto *sticksRow = new QHBoxLayout();
    sticksRow->setSpacing(12);

    auto *leftStickBox = new QGroupBox(QStringLiteral("Left Stick"));
    auto *leftStickLayout = new QGridLayout(leftStickBox);
    auto *leftPad = new JoystickPad();
    leftStickLayout->addWidget(leftPad, 0, 0);
    leftValueLabel_ = new QLabel(QStringLiteral("X: 0  Y: 0"));
    leftValueLabel_->setAlignment(Qt::AlignCenter);
    leftStickLayout->addWidget(leftValueLabel_, 1, 0);

    auto *rightStickBox = new QGroupBox(QStringLiteral("Right Stick"));
    auto *rightStickLayout = new QGridLayout(rightStickBox);
    auto *rightPad = new JoystickPad();
    rightStickLayout->addWidget(rightPad, 0, 0);
    rightValueLabel_ = new QLabel(QStringLiteral("X: 0  Y: 0"));
    rightValueLabel_->setAlignment(Qt::AlignCenter);
    rightStickLayout->addWidget(rightValueLabel_, 1, 0);

    sticksRow->addWidget(leftStickBox, 1);
    sticksRow->addWidget(rightStickBox, 1);

    auto *comboBox = new QGroupBox(QStringLiteral("Combos"));
    auto *comboLayout = new QHBoxLayout(comboBox);
    lbYButton_ = new QPushButton(QStringLiteral("LB + Y"));
    lbRbButton_ = new QPushButton(QStringLiteral("LB + RB"));
    lbYButton_->setMinimumHeight(56);
    lbRbButton_->setMinimumHeight(56);
    lbYButton_->setShortcut(QKeySequence(QStringLiteral("Ctrl+1")));
    lbRbButton_->setShortcut(QKeySequence(QStringLiteral("Ctrl+2")));
    comboLayout->addWidget(lbYButton_);
    comboLayout->addWidget(lbRbButton_);

    controls.push_back(leftPad);
    controls.push_back(rightPad);
    controls.push_back(lbYButton_);
    controls.push_back(lbRbButton_);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(statusLabel_);
    layout->addLayout(sticksRow);
    layout->addWidget(comboBox);
    layout->addStretch(1);

    const QString baseStyle = QStringLiteral(R"(
        QWidget {
            background: #0f172a;
            color: #e2e8f0;
            font-family: "Noto Sans", "DejaVu Sans", sans-serif;
            font-size: 14px;
        }
        QLabel#title {
            color: #f8fafc;
        }
        QGroupBox {
            border: 1px solid #334155;
            border-radius: 12px;
            margin-top: 18px;
            padding-top: 12px;
            background: rgba(15, 23, 42, 0.55);
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            color: #cbd5e1;
            font-weight: 700;
        }
        QPushButton {
            background: #1e293b;
            border: 1px solid #334155;
            border-radius: 10px;
            padding: 12px 14px;
            font-size: 15px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #273449;
        }
        QPushButton:pressed {
            background: #0f172a;
        }
        QPushButton:disabled {
            color: #64748b;
            background: #111827;
        }
    )");
    setStyleSheet(baseStyle);
    title->setObjectName(QStringLiteral("title"));

    leftPad->setOnValueChanged([this](int x, int y) {
        setLeftStick(x, y);
    });
    rightPad->setOnValueChanged([this](int x, int y) {
        setRightStick(x, y);
    });

    connect(lbYButton_, &QPushButton::clicked, this, [this]() {
        triggerCombo(QStringLiteral("LB + Y"), lbYCombo());
    });
    connect(lbRbButton_, &QPushButton::clicked, this, [this]() {
        triggerCombo(QStringLiteral("LB + RB"), lbRbCombo());
    });

    if (controller_.isReady()) {
        statusLabel_->setText(QStringLiteral("虚拟手柄已创建，摇杆轴映射已修正：X=水平，Y=垂直。"));
    } else {
        statusLabel_->setText(QStringLiteral("虚拟手柄创建失败：%1").arg(controller_.errorString()));
        for (QWidget *control : controls) {
            control->setEnabled(false);
        }
    }
}

void MainWindow::triggerCombo(const QString &label, const QVector<int> &buttons)
{
    if (!controller_.isReady()) {
        statusLabel_->setText(QStringLiteral("%1 触发失败：控制器未就绪。").arg(label));
        return;
    }

    if (controller_.tapCombo(buttons)) {
        statusLabel_->setText(QStringLiteral("已发送组合：%1").arg(label));
    } else {
        statusLabel_->setText(QStringLiteral("发送组合失败：%1（%2）").arg(label, controller_.errorString()));
    }
}

void MainWindow::setLeftStick(int x, int y)
{
    leftValueLabel_->setText(QStringLiteral("X: %1  Y: %2").arg(x).arg(y));

    if (!controller_.isReady()) {
        return;
    }

    if (!controller_.setLeftStick(x, y)) {
        statusLabel_->setText(QStringLiteral("左摇杆写入失败：%1").arg(controller_.errorString()));
    }
}

void MainWindow::setRightStick(int x, int y)
{
    rightValueLabel_->setText(QStringLiteral("X: %1  Y: %2").arg(x).arg(y));

    if (!controller_.isReady()) {
        return;
    }

    if (!controller_.setRightStick(x, y)) {
        statusLabel_->setText(QStringLiteral("右摇杆写入失败：%1").arg(controller_.errorString()));
    }
}
