// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNVIEW_H
#define MARKDOWNVIEW_H

#include "markdownpreview_global.h"

#include <QWidget>
#include <QTextBrowser>

class QLabel;
class QStackedWidget;

namespace GrandSearch {
namespace markdown_preview {

class MarkdownBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    explicit MarkdownBrowser(QWidget *parent = nullptr);
};

class MarkdownView : public QWidget
{
    Q_OBJECT
public:
    explicit MarkdownView(QWidget *parent = nullptr);
    void initUI();
    void setSource(const QString &path);
    void showErrorPage();
protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
private:
    MarkdownBrowser *m_browser = nullptr;
    QLabel *m_errLabel = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
};

}}

#endif // MARKDOWNVIEW_H
