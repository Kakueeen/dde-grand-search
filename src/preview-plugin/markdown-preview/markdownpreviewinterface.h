// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNPREVIEWINTERFACE_H
#define MARKDOWNPREVIEWINTERFACE_H

#include "markdownpreview_global.h"

#include "previewplugininterface.h"

namespace GrandSearch {
namespace markdown_preview {

class MarkdownPreviewInterface : public QObject, public PreviewPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(GrandSearch::PreviewPluginInterface)
    Q_PLUGIN_METADATA(IID FilePreviewInterface_iid)
public:
    explicit MarkdownPreviewInterface(QObject *parent = nullptr);
    virtual PreviewPlugin *create(const QString &mimetype);
};

}}

#endif // MARKDOWNPREVIEWINTERFACE_H
