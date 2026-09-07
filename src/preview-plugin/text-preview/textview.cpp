// SPDX-FileCopyrightText: 2021 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "textpreview_global.h"
#include "textview.h"
#include "global/commontools.h"

#include <DTextEncoding>

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QScrollBar>
#include <QTextCodec>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QFile>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logTextPreview)

GRANDSEARCH_USE_NAMESPACE
using namespace GrandSearch::text_preview;

// 最多读取的文本大小，避免大文件阻塞主线程
static constexpr qint64 kMaxReadSize { 1024 * 1024 };

void PlainTextEdit::mouseMoveEvent(QMouseEvent *e)
{
    // 支持鼠标拖动框选文本，交回基类处理
    QPlainTextEdit::mouseMoveEvent(e);
}

// Qt5 无 QByteArray::isValidUtf8，统一用 QTextCodec 校验 UTF-8 合法性
static bool isValidUtf8(const QByteArray &data)
{
    if (QTextCodec *codec = QTextCodec::codecForName("UTF-8")) {
        QTextCodec::ConverterState state;
        codec->toUnicode(data.constData(), data.size(), &state);
        return state.invalidChars < 1;
    }
    return false;
}

QString TextView::toUnicode(const QByteArray &data)
{
    QString text;
    if (data.isEmpty())
        return text;

    // 参考文管 text-preview 的转码策略：
    // Step 1: 已是合法 UTF-8，直接使用（覆盖 ASCII 与真正的 UTF-8 文件）
    if (isValidUtf8(data)) {
        qCDebug(logTextPreview) << "Raw data is valid UTF-8, using directly";
        return QString::fromUtf8(data);
    }

    QByteArray rawData = data;
    QByteArray out;

    // Step 2: 尝试 GB18030 → UTF-8（GB18030 是 GBK 超集，覆盖绝大多数非 UTF-8 中文文本）
    if (Dtk::Core::DTextEncoding::convertTextEncoding(rawData, out, "utf-8", "gb18030")) {
        qCDebug(logTextPreview) << "GB18030 → UTF-8 conversion successful";
        return QString::fromUtf8(out);
    }

    // Step 3: 自动检测编码兜底（Big5、EUC-JP 等少见编码）
    QByteArray detected = Dtk::Core::DTextEncoding::detectTextEncoding(data);
    qCDebug(logTextPreview) << "Detected file encoding:" << detected;
    if (!detected.isEmpty() && detected.toLower() != "utf-8") {
        rawData = data;
        if (Dtk::Core::DTextEncoding::convertTextEncoding(rawData, out, "utf-8")) {
            qCDebug(logTextPreview) << "Encoding conversion successful from" << detected;
            return QString::fromUtf8(out);
        }
    }

    // 检测为 UTF-8 或转换失败时，按 UTF-8 解码，损坏字节显示为替换符而非整体乱码
    return QString::fromUtf8(data);
}

void TextView::showErrorPage()
{
    //重设边距
    layout()->setContentsMargins(10, 0, 0, 0);
    m_stackedWidget->setCurrentWidget(m_errLabel);

    int width = 360;
    int height = this->height() > 0 ? this->height() : 350;
    QImage errImg(":/icons/file_damaged.svg");
    errImg = errImg.scaled(70, 70);
    errImg = CommonTools::creatErrorImage({width, height}, errImg);

    QPixmap roundPixmap(width, height);
    roundPixmap.fill(Qt::transparent);
    QPainter painter(&roundPixmap);
    painter.setRenderHints(QPainter::Antialiasing, true);           // 抗锯齿
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);  // 平滑

    QPainterPath path;
    QRect rect(0, 0, width, height);
    path.addRoundedRect(rect, 8, 8);            // 圆角
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, width, height, QPixmap::fromImage(errImg));

    m_errLabel->setPixmap(roundPixmap);
}

void TextView::paintEvent(QPaintEvent *event)
{
    if (m_stackedWidget->currentWidget() == m_browser) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        //文本框的背景
        auto view = m_browser->viewport();
        painter.setBrush(view->palette().color(view->backgroundRole()));
        painter.setPen(Qt::NoPen);

        //画圆角背景,背景大小为去除左边距10的区域
        auto r = rect();
        r.setLeft(10);
        painter.drawRoundedRect(r, 8, 8);
    }

    QWidget::paintEvent(event);
}


TextView::TextView(QWidget *parent) : QWidget(parent)
{
}

void TextView::initUI()
{
    auto layout = new QHBoxLayout(this);
    this->setLayout(layout);

    layout->setSpacing(0);

    m_errLabel = new QLabel(this);
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setContentsMargins(0, 0, 0, 0);

    m_browser = new PlainTextEdit(this);

    //文本界面不绘制背景，自绘圆角背景
    m_browser->viewport()->setAutoFillBackground(false);
    m_browser->setFrameShape(QFrame::NoFrame);

    //内容超出一屏时可纵向滚动，横向始终禁止（自动换行）
    m_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_browser->horizontalScrollBar()->setDisabled(true);

    //只读，允许框选与复制，但不显示光标
    m_browser->setReadOnly(true);
    m_browser->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_browser->setCursorWidth(0);
    m_browser->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_browser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_browser->setFocusPolicy(Qt::ClickFocus);

    //样式
    //文本内容上边距是10,左右边距是20,通过DocumentMargin设置10的边距
    //再通过layout增加左右边距各10，来达到上边距与左右边距不一样的效果
    m_browser->document()->setDocumentMargin(10);
    //左边距文本背景与中线10,加上文本内容边距10;补充文本边距10
    //因此在绘制圆角背景时为只去除左边距离中线10的区域
    layout->setContentsMargins(10 + 10, 0, 0 + 10, 0);

    m_stackedWidget->addWidget(m_browser);
    m_stackedWidget->addWidget(m_errLabel);
    m_stackedWidget->setCurrentWidget(m_browser);
    layout->addWidget(m_stackedWidget);
}

void TextView::setSource(const QString &path)
{
    qCDebug(logTextPreview) << "Setting text source:" << path;
    m_browser->clear();

    //恢复边距
    layout()->setContentsMargins(10 + 10, 0, 0 + 10, 0);
    m_stackedWidget->setCurrentWidget(m_browser);

    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        // 读取更多内容以便完整预览，上限 1MB 避免大文件阻塞
        auto datas = file.read(kMaxReadSize);
        qCDebug(logTextPreview) << "Text file loaded successfully - Size:" << datas.size() << "bytes";
        m_browser->setPlainText(toUnicode(datas));
    } else {
        qCWarning(logTextPreview) << "Failed to open text file:" << path;
        showErrorPage();
    }
}
