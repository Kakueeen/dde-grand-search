// SPDX-FileCopyrightText: 2021 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GROUPWIDGET_H
#define GROUPWIDGET_H

#include "global/matcheditem.h"
#include "global/builtinsearch.h"

#include <DWidget>

#include <QScopedPointer>


class GroupWidgetPrivate;
class GrandSearchListView;
class ViewMoreButton;

DWIDGET_BEGIN_NAMESPACE
class DLabel;
class DHorizontalLine;
DWIDGET_END_NAMESPACE
DWIDGET_USE_NAMESPACE

class QVBoxLayout;
class QHBoxLayout;
class GroupWidget : public DWidget
{
    Q_OBJECT
public:
    explicit GroupWidget(QWidget *parent = nullptr);
    ~GroupWidget() override;

    virtual void appendMatchedItems(const GrandSearch::MatchedItems &newItems, const QString& searchGroupName);
    virtual void clear();

    void setSearchGroupName(const QString &searchGroupName);
    QString searchGroupName() const;

    void setGroupName(const QString &groupName);
    QString groupName() const;

    void showHorLine(bool bShow = true);
    bool isHorLineVisilbe();
    GrandSearchListView *getListView();
    ViewMoreButton *getViewMoreButton();
    int itemCount();
    // 获取当前选中行在类目中的高度
    int getCurSelectHeight();
    void reLayout();

    static QString convertDisplayName(const QString &searchGroupName);

public slots:
    virtual void onMoreBtnClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUi();
    void initConnect();
    void updateShowItems(GrandSearch::MatchedItems &items);

signals:
    void showMore();
    void sigCurrentItemChanged(const QString &groupName, const GrandSearch::MatchedItem &item);

protected:
    GrandSearchListView *m_listView = nullptr;
    ViewMoreButton* m_viewMoreButton = nullptr;

    bool m_bListExpanded = false;                   // 结果列表是否已展开
    GrandSearch::MatchedItems m_firstFiveItems;     // 前5行正在显示的匹配结果
    GrandSearch::MatchedItems m_restShowItems;      // 剩余正在显示的匹配结果
    GrandSearch::MatchedItems m_cacheItems;         // 缓存中的匹配结果
    GrandSearch::MatchedItems m_cacheWeightItems;   // 缓存经过权重排序的匹配结果

    QString m_searchGroupName;                      // 所属类目

private:
    QScopedPointer<GroupWidgetPrivate> d_p;

    QVBoxLayout *m_vLayout = nullptr;               // 主体垂直布局
    QHBoxLayout *m_hTitelLayout = nullptr;          // 顶部标题水平布局
    QVBoxLayout *m_vContentLayout = nullptr;        // 内容垂直布局，用于限定列表和横线间距10个像素间隙

    DLabel *m_groupLabel = nullptr;
    DHorizontalLine *m_line = nullptr;
};

typedef QMap<QString, GroupWidget *> GroupWidgetMap;
typedef QList<GroupWidget *> GroupWidgets;

#endif // GROUPWIDGET_H
