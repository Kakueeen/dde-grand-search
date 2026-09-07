// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "markdownpreview_global.h"
#include "markdownview.h"
#include "global/commontools.h"

#include <QHBoxLayout>
#include <QStackedWidget>
#include <QScrollBar>
#include <QPainter>
#include <QPainterPath>
#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextOption>
#include <QUrl>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logMarkdownPreview)

GRANDSEARCH_USE_NAMESPACE
using namespace GrandSearch::markdown_preview;

// 5MB maximum file size
static constexpr qint64 kMaxReadSize { 1024 * 1024 * 5 };

MarkdownBrowser::MarkdownBrowser(QWidget *parent)
    : QTextBrowser(parent)
{
    setReadOnly(true);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse);
    setOpenExternalLinks(true);
    // 内容自动适配当前宽度，允许在任意位置换行（含代码块/长 URL），避免横向滚动
    setLineWrapMode(QTextBrowser::WidgetWidth);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    QTextOption option = document()->defaultTextOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultTextOption(option);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MarkdownView::showErrorPage()
{
    // 重设边距
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

void MarkdownView::paintEvent(QPaintEvent *event)
{
    if (m_stackedWidget->currentWidget() == m_browser) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        // 文本框的背景
        auto view = m_browser->viewport();
        painter.setBrush(view->palette().color(view->backgroundRole()));
        painter.setPen(Qt::NoPen);

        // 画圆角背景,背景大小为去除左边距10的区域
        auto r = rect();
        r.setLeft(10);
        painter.drawRoundedRect(r, 8, 8);
    }

    QWidget::paintEvent(event);
}

MarkdownView::MarkdownView(QWidget *parent) : QWidget(parent)
{
}

void MarkdownView::initUI()
{
    auto layout = new QHBoxLayout(this);
    this->setLayout(layout);

    layout->setSpacing(0);

    m_errLabel = new QLabel(this);
    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setContentsMargins(0, 0, 0, 0);

    m_browser = new MarkdownBrowser(this);

    // 文本界面不绘制背景，自绘圆角背景
    m_browser->viewport()->setAutoFillBackground(false);
    m_browser->setFrameShape(QFrame::NoFrame);

    // 内容超出一屏时可滚动
    m_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 只读、自动换行
    m_browser->setReadOnly(true);
    m_browser->setFocusPolicy(Qt::ClickFocus);

    // 样式
    m_browser->document()->setDocumentMargin(10);
    layout->setContentsMargins(10 + 10, 0, 0 + 10, 0);

    m_stackedWidget->addWidget(m_browser);
    m_stackedWidget->addWidget(m_errLabel);
    m_stackedWidget->setCurrentWidget(m_browser);
    layout->addWidget(m_stackedWidget);
}

void MarkdownView::setSource(const QString &path)
{
    qCDebug(logMarkdownPreview) << "Setting markdown source:" << path;
    m_browser->clear();

    // 恢复边距
    layout()->setContentsMargins(10 + 10, 0, 0 + 10, 0);
    m_stackedWidget->setCurrentWidget(m_browser);

    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        qCWarning(logMarkdownPreview) << "Markdown file not exists or not readable:" << path;
        showErrorPage();
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(logMarkdownPreview) << "Failed to open markdown file:" << path << "error:" << file.errorString();
        showErrorPage();
        return;
    }

    // 限制读取大小到 5MB
    qint64 readSize = qMin(fileInfo.size(), kMaxReadSize);
    QByteArray data = file.read(readSize);
    file.close();

    if (data.isEmpty()) {
        qCWarning(logMarkdownPreview) << "Markdown file is empty:" << path;
        showErrorPage();
        return;
    }

    const QString markdown = QString::fromUtf8(data);

    // 设置相对路径图片解析基准
    const QString basePath = fileInfo.absolutePath();
    if (!basePath.isEmpty()) {
        m_browser->document()->setMetaInformation(QTextDocument::DocumentUrl, QUrl::fromLocalFile(basePath).toString());
        m_browser->setSearchPaths(QStringList() << basePath);
    }

    m_browser->document()->setMarkdown(markdown);

    // 代码块等段落会被 markdown 解析器标记为不可换行，强制清除以适配预览宽度，避免横向滚动
    for (QTextBlock block = m_browser->document()->begin(); block.isValid(); block = block.next()) {
        QTextBlockFormat format = block.blockFormat();
        if (format.nonBreakableLines()) {
            format.setNonBreakableLines(false);
            QTextCursor cursor(block);
            cursor.setBlockFormat(format);
        }
    }

    // 移动到文档顶部
    m_browser->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);

    qCDebug(logMarkdownPreview) << "Markdown file loaded successfully - Size:" << data.size() << "bytes";
}
