// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "markdownpreview_global.h"
#include "markdownpreviewplugin.h"
#include "markdownview.h"

#include <QFileInfo>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logMarkdownPreview, "org.deepin.dde.grandsearch.plugin.markdown")

GRANDSEARCH_USE_NAMESPACE
using namespace GrandSearch::markdown_preview;

MarkdownPreviewPlugin::MarkdownPreviewPlugin(QObject *parent)
    : QObject(parent)
    , PreviewPlugin()
{
    qCDebug(logMarkdownPreview) << "MarkdownPreviewPlugin created";
}

MarkdownPreviewPlugin::~MarkdownPreviewPlugin()
{
    qCDebug(logMarkdownPreview) << "MarkdownPreviewPlugin destroyed";
    if (m_view) {
        m_view->deleteLater();
        m_view = nullptr;
    }
}

void MarkdownPreviewPlugin::init(QObject *proxyInter)
{
    Q_UNUSED(proxyInter)
    qCDebug(logMarkdownPreview) << "Initializing MarkdownPreviewPlugin";

    if (!m_view) {
        m_view = new MarkdownView();
        m_view->initUI();
        qCDebug(logMarkdownPreview) << "MarkdownView created and initialized";
    }
}

bool MarkdownPreviewPlugin::previewItem(const ItemInfo &item)
{
    const QString path = item.value(PREVIEW_ITEMINFO_ITEM);
    if (path.isEmpty()) {
        qCWarning(logMarkdownPreview) << "Markdown file path is empty - Cannot preview";
        return false;
    }

    qCDebug(logMarkdownPreview) << "Previewing markdown file - Path:" << path;

    if (!m_view) {
        init(nullptr);
    }

    m_item = item;
    m_view->setSource(path);
    return true;
}

ItemInfo MarkdownPreviewPlugin::item() const
{
    return m_item;
}

QWidget *MarkdownPreviewPlugin::contentWidget() const
{
    return m_view;
}

bool MarkdownPreviewPlugin::stopPreview()
{
    qCDebug(logMarkdownPreview) << "Stopping markdown preview";
    return true;
}

QWidget *MarkdownPreviewPlugin::toolBarWidget() const
{
    return nullptr;
}

bool MarkdownPreviewPlugin::showToolBar() const
{
    return true;
}

DetailInfoList MarkdownPreviewPlugin::getAttributeDetailInfo() const
{
    return {};
}
