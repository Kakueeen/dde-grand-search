// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "markdownpreview_global.h"
#include "markdownpreviewinterface.h"
#include "markdownpreviewplugin.h"

GRANDSEARCH_USE_NAMESPACE
using namespace GrandSearch::markdown_preview;

MarkdownPreviewInterface::MarkdownPreviewInterface(QObject *parent)
    : QObject(parent)
    , PreviewPluginInterface()
{

}

PreviewPlugin *MarkdownPreviewInterface::create(const QString &mimetype)
{
    Q_UNUSED(mimetype)

    return new MarkdownPreviewPlugin();
}
