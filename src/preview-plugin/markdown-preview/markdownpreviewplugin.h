// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNPREVIEWPLUGIN_H
#define MARKDOWNPREVIEWPLUGIN_H

#include "markdownpreview_global.h"

#include "previewplugin.h"

namespace GrandSearch {
namespace markdown_preview {

class MarkdownView;
class MarkdownPreviewPlugin : public QObject, public PreviewPlugin
{
    Q_OBJECT
public:
    explicit MarkdownPreviewPlugin(QObject *parent = nullptr);
    ~MarkdownPreviewPlugin() Q_DECL_OVERRIDE;
    void init(QObject *proxyInter) Q_DECL_OVERRIDE;
    bool previewItem(const ItemInfo &item) Q_DECL_OVERRIDE;
    ItemInfo item() const Q_DECL_OVERRIDE;
    QWidget *contentWidget() const Q_DECL_OVERRIDE;
    bool stopPreview() Q_DECL_OVERRIDE;
    QWidget *toolBarWidget() const Q_DECL_OVERRIDE;
    bool showToolBar() const Q_DECL_OVERRIDE;
    DetailInfoList getAttributeDetailInfo() const Q_DECL_OVERRIDE;
    bool expandContent() const Q_DECL_OVERRIDE { return true; }
protected:
    ItemInfo m_item;
    MarkdownView *m_view = nullptr;
    DetailInfoList m_detailInfo;
};

}}

#endif // MARKDOWNPREVIEWPLUGIN_H
