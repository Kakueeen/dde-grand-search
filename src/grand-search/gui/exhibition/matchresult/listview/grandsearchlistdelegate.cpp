// SPDX-FileCopyrightText: 2021 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "grandsearchlistdelegate.h"
#include "grandsearchlistview.h"
#include "global/builtinsearch.h"
#include "global/widgets/highlightutils.h"

#include <DGuiApplicationHelper>
#include <DFontSizeManager>

#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionViewItem>
#include <QTextDocument>
#include <QTextCursor>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QToolTip>

#define ListItemSpace 10   // 列表图标与文本间间距
#define ListItemHeight 36   // 列表行高（单行）
#define ListIconSize 24   // 列表图标大小
#define ListRowWidth 740   // 列表行宽
#define ListIconMargin 10   // 列表图标边距
#define TailDataMargin 10   // 拖尾信息显示间隔
#define TailMaxWidth 150   // 拖尾信息最大显示宽度
#define ContextLineSpacing 1   // 文件名与匹配内容行间距
#define ListLineMargin 8   // 行上下边距

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
using namespace GrandSearch;

GrandSearchListDelegate::GrandSearchListDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
{
}

GrandSearchListDelegate::~GrandSearchListDelegate()
{
}

void GrandSearchListDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    DStyledItemDelegate::initStyleOption(option, index);
}

bool GrandSearchListDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(event)
    Q_UNUSED(model)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return true;
}

void GrandSearchListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->setRenderHint(QPainter::Antialiasing);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
#endif

    // 让基类绘制背景和选中状态（不绘制图标和文本）
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    viewOption.text = "";
    viewOption.icon = QIcon();
    QStyle *pStyle = viewOption.widget ? viewOption.widget->style() : QApplication::style();
    pStyle->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, viewOption.widget);

    // 绘制选中状态
    drawSelectState(painter, viewOption, index);

    // 绘制图标（普通图标或缩略图）
    drawIcon(painter, viewOption, index);

    // 绘制匹配结果文本
    drawSearchResultText(painter, viewOption, index);
}

QSize GrandSearchListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    bool hasSecondLine = false;
    if (index.isValid()) {
        QVariant extra = index.data(DATA_ROLE).value<MatchedItem>().extra;
        if (extra.isValid()) {
            const QVariantHash &hash = extra.toHash();
            hasSecondLine = hash.contains(GRANDSEARCH_PROPERTY_ITEM_MATCHEDCONTEXT)
                    || hash.contains(GRANDSEARCH_PROPERTY_ITEM_TAILER);
        }
    }

    if (!hasSecondLine)
        return QSize(ListRowWidth, ListItemHeight);

    QFontMetrics nameFm(DFontSizeManager::instance()->get(DFontSizeManager::T6));
    QFontMetrics contextFm(DFontSizeManager::instance()->get(DFontSizeManager::T8));
    int totalHeight = ListLineMargin + nameFm.height() + ContextLineSpacing + contextFm.height() + ListLineMargin;
    totalHeight = qMax(totalHeight, ListItemHeight);

    return QSize(ListRowWidth, totalHeight);
}

static void hideTooltipImmediately()
{
    QWidgetList qwl = QApplication::topLevelWidgets();
    for (QWidget *qw : qwl) {
        if (QStringLiteral("QTipLabel") == qw->metaObject()->className()) {
            qw->close();
        }
    }
}

bool GrandSearchListDelegate::helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (event->type() == QEvent::ToolTip) {
        const QString tooltip = index.data(Qt::ToolTipRole).toString();

        if (tooltip.isEmpty()) {
            hideTooltipImmediately();
        } else {
            int tooltipsize = tooltip.size();
            const int nlong = 32;
            int lines = tooltipsize / nlong + 1;
            QString strtooltip;
            for (int i = 0; i < lines; ++i) {
                strtooltip.append(tooltip.mid(i * nlong, nlong));
                strtooltip.append("\n");
            }
            strtooltip.chop(1);
            QToolTip::showText(event->globalPos(), strtooltip, view);
        }

        return true;
    }

    return DStyledItemDelegate::helpEvent(event, view, option, index);
}

void GrandSearchListDelegate::drawIcon(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant thumbnailData = index.data(THUMBNAIL_ROLE);
    if (thumbnailData.isValid() && !thumbnailData.value<QPixmap>().isNull()) {
        QPixmap thumbnail = thumbnailData.value<QPixmap>();
        // 按实际比例缩放，宽高不超过 ListIconSize
        QSize scaledSize = thumbnail.size().scaled(ListIconSize, ListIconSize, Qt::KeepAspectRatio);
        QRect iconRect(QPoint(), scaledSize);
        iconRect.moveCenter(QPoint(option.rect.x() + ListIconMargin + ListIconSize / 2,
                                   option.rect.y() + option.rect.height() / 2));

        painter->save();

        // 圆角裁剪
        QPainterPath clipPath;
        clipPath.addRoundedRect(iconRect, 2, 2);
        painter->setClipPath(clipPath);
        painter->drawPixmap(iconRect, thumbnail);

        // 内描边
        painter->setClipping(false);
        QColor borderColor = option.palette.color(QPalette::BrightText);
        borderColor.setAlphaF(0.1);

        QPen borderPen(borderColor, 1);
        painter->setPen(borderPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(iconRect, 2, 2);
        painter->restore();
    } else {
        // 绘制普通图标
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (icon.isNull())
            return;

        QRect iconRect(option.rect.x() + ListIconMargin, option.rect.y() + (option.rect.height() - ListIconSize) / 2,
                       ListIconSize, ListIconSize);
        painter->save();
        icon.paint(painter, iconRect);
        painter->restore();
    }
}

void GrandSearchListDelegate::drawSelectState(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);

    if (option.state & QStyle::State_Selected)
        return;

    if (option.state & QStyle::State_MouseOver) {
        painter->save();
        painter->setPen(Qt::NoPen);

        QColor hovertColor;
        const GrandSearchListView *listview = qobject_cast<const GrandSearchListView *>(option.widget);
        if ((listview->getThemeType() == DGuiApplicationHelper::DarkType)
            || (index.isValid() && index == listview->currentIndex())) {
            hovertColor = QColor(255, 255, 255, static_cast<int>(255 * 0.05));
        } else {
            hovertColor = QColor(0, 0, 0, static_cast<int>(255 * 0.05));
        }
        painter->setBrush(hovertColor);
        QRect selecteColorRect = option.rect.adjusted(0, 0, 0, 0);
        painter->drawRoundedRect(selecteColorRect, 8, 8);
        painter->restore();
    }
}

void GrandSearchListDelegate::drawSearchResultText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const MatchedItem &item = index.data(DATA_ROLE).value<MatchedItem>();
    const QVariantHash &extraHash = item.extra.toHash();

    const GrandSearchListView *listview = qobject_cast<const GrandSearchListView *>(option.widget);
    bool isDark = (listview->getThemeType() == DGuiApplicationHelper::DarkType)
            || (index.isValid() && index == listview->currentIndex());
    QColor nameTextColor = isDark ? QColor("#FFFFFF") : QColor("#000000");
    nameTextColor.setAlpha(255 * 0.9);

    QStringList keywords = extraHash.value(GRANDSEARCH_PROPERTY_ITEM_KEYWORDS, QStringList()).toStringList();
    QFont nameFont = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    QFontMetrics nameFontMetrics(nameFont);

    QFont contextFont = DFontSizeManager::instance()->get(DFontSizeManager::T8);
    QFontMetrics contextFontMetrics(contextFont);
    int textStartX = ListIconMargin + ListIconSize + ListItemSpace;
    int firstLineY = option.rect.y() + ListLineMargin;
    int secondLineY = firstLineY + nameFontMetrics.height() + ContextLineSpacing;

    // 绘制文件名（第一行）
    drawItemName(painter, option, item.name, item.searcher, keywords,
                 nameFont, nameFontMetrics, nameTextColor, isDark, firstLineY);

    // 第二行：修改时间 + 上级目录 + 摘要（预览时只显示修改时间）
    bool isPreview = listview->isPreviewItem();
    bool hasContext = !extraHash.value(GRANDSEARCH_PROPERTY_ITEM_MATCHEDCONTEXT).toString().isEmpty();
    int currentX = textStartX;
    int remainingWidth = option.rect.width() - currentX;

    // 绘制拖尾数据（预览时只显示修改时间，非预览时显示全部）
    currentX = drawTailText(painter, option, index, remainingWidth, currentX, secondLineY, isPreview ? 1 : 0);
    remainingWidth = option.rect.width() - currentX;

    // 再绘制匹配摘要
    if (!isPreview && hasContext && remainingWidth > TailDataMargin) {
        currentX += TailDataMargin;
        remainingWidth = option.rect.width() - currentX;
        drawMatchedContext(painter, option,
                           extraHash.value(GRANDSEARCH_PROPERTY_ITEM_MATCHEDCONTEXT).toString(),
                           keywords, contextFont, contextFontMetrics, isDark, secondLineY, currentX);
    }
}

void GrandSearchListDelegate::drawItemName(QPainter *painter, const QStyleOptionViewItem &option,
                                           const QString &name, const QString &searcher,
                                           const QStringList &keywords, const QFont &font,
                                           const QFontMetrics &fontMetrics, const QColor &textColor,
                                           bool isDark, int startY) const
{
    int textStartX = ListIconMargin + ListIconSize + ListItemSpace;
    int nameTextMaxWidth = option.rect.width() - textStartX;

    QString elidedName = fontMetrics.elidedText(name, Qt::ElideRight, nameTextMaxWidth);
    if (elidedName != name && GRANDSEARCH_CLASS_WEB_STATICTEXT == searcher) {
        static const QString markStr = name.right(1);
        static const int markWidth = fontMetrics.size(Qt::TextSingleLine, markStr).width();
        elidedName = fontMetrics.elidedText(name.left(name.size() - 1), Qt::ElideRight, nameTextMaxWidth - markWidth);
        elidedName.append(markStr);
    }

    QTextDocument nameDocument;
    nameDocument.setDocumentMargin(0);
    nameDocument.setPlainText(elidedName);
    QTextCursor nameCursor(&nameDocument);

    nameCursor.beginEditBlock();
    nameCursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat nameFmt;
    nameFmt.setForeground(textColor);
    nameFmt.setFont(font);
    nameCursor.mergeCharFormat(nameFmt);

    if (!keywords.isEmpty()) {
        QTextCharFormat hlFmt;
        hlFmt.setFontWeight(QFont::DemiBold);
        hlFmt.setForeground(isDark ? QColor("#FFFFFF") : QColor("#000000"));
        for (const QString &kw : keywords) {
            if (kw.isEmpty()) continue;
            QTextCursor searchCursor(&nameDocument);
            while (!searchCursor.isNull() && !searchCursor.atEnd()) {
                searchCursor = nameDocument.find(kw, searchCursor,
                                                 QTextDocument::FindCaseSensitively);
                if (!searchCursor.isNull())
                    searchCursor.mergeCharFormat(hlFmt);
            }
        }
    }

    nameCursor.clearSelection();
    nameCursor.movePosition(QTextCursor::EndOfLine);
    nameCursor.endEditBlock();

    QAbstractTextDocumentLayout::PaintContext paintContext;
    int actualNameWidth = fontMetrics.size(Qt::TextSingleLine, elidedName).width();
    QRect drawRect(textStartX, startY, actualNameWidth, fontMetrics.height());

    painter->save();
    painter->translate(drawRect.topLeft());
    painter->setClipRect(drawRect.translated(-drawRect.topLeft()));
    nameDocument.documentLayout()->draw(painter, paintContext);
    painter->restore();
}

void GrandSearchListDelegate::drawMatchedContext(QPainter *painter, const QStyleOptionViewItem &option,
                                                 const QString &matchedContext, const QStringList &keywords,
                                                 const QFont &font, const QFontMetrics &fontMetrics,
                                                 bool isDark, int startY, int startX) const
{
    if (startX <= 0)
        startX = ListIconMargin + ListIconSize + ListItemSpace;
    int maxWidth = option.rect.width() - startX;

    QColor textColor = isDark ? QColor(255, 255, 255, int(255 * 0.5)) : QColor(0, 0, 0, int(255 * 0.5));
    QColor highlightColor = isDark ? QColor(255, 255, 255, int(255 * 0.8)) : QColor(0, 0, 0, int(255 * 0.85));

    // 以最左边的关键词匹配为锚点进行省略
    auto allRanges = HighlightUtils::findMatchRanges(matchedContext, keywords);
    QVector<HighlightUtils::MatchRange> leftmostOnly;
    if (!allRanges.isEmpty())
        leftmostOnly.append(allRanges.constFirst());

    HighlightUtils::ElideResult elideResult;
    if (!leftmostOnly.isEmpty())
        elideResult = HighlightUtils::smartElideWithTracking(matchedContext, maxWidth, fontMetrics, leftmostOnly);
    if (elideResult.text.isEmpty())
        elideResult = HighlightUtils::elideWithTracking(matchedContext, Qt::ElideRight, maxWidth, fontMetrics);

    // 构建高亮格式
    QTextCharFormat hlFmt;
    hlFmt.setFont(font);
    hlFmt.setFontWeight(QFont::DemiBold);
    hlFmt.setForeground(highlightColor);
    auto formats = HighlightUtils::buildFormatRanges(
            elideResult.text, elideResult.origPositions, matchedContext, keywords, hlFmt);

    // 绘制
    QTextLayout layout;
    layout.setText(elideResult.text);
    layout.setFont(font);
    layout.setFormats(formats);
    layout.beginLayout();
    QTextLine line = layout.createLine();
    if (line.isValid())
        line.setLineWidth(maxWidth);
    layout.endLayout();

    painter->save();
    painter->setPen(textColor);
    painter->translate(startX, startY);
    layout.draw(painter, QPointF(0, 0));
    painter->restore();
}

int GrandSearchListDelegate::drawTailText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index, int maxWidth, int startX, int startY, int maxCount) const
{
    QFont tailFont = DFontSizeManager::instance()->get(DFontSizeManager::T8);
    QFontMetrics tailFontMetrics(tailFont);

    const QVariant &extra = index.data(DATA_ROLE).value<MatchedItem>().extra;
    QStringList tailStringList = extra.toHash().value(GRANDSEARCH_PROPERTY_ITEM_TAILER, QStringList()).toStringList();
    if (tailStringList.isEmpty())
        return startX;

    // 预览时只显示修改时间（第一条）
    if (maxCount > 0 && tailStringList.count() > maxCount)
        tailStringList = tailStringList.mid(0, maxCount);

    int totalTailWidth = maxWidth;

    int tailCount = tailStringList.count();
    int tailAverageWidth = totalTailWidth;
    calcTailShowInfo(totalTailWidth, tailCount, tailAverageWidth);
    const QMap<int, QString> &tailMap = calcTailShowData(tailStringList, tailCount, tailAverageWidth, tailFontMetrics);
    if (tailMap.isEmpty())
        return startX;

    QColor tailTextColor;
    const GrandSearchListView *listview = qobject_cast<const GrandSearchListView *>(option.widget);
    if ((listview->getThemeType() == DGuiApplicationHelper::DarkType)
        || (index.isValid() && index == listview->currentIndex())) {
        tailTextColor = QColor("#FFFFFF");
    } else {
        tailTextColor = QColor("#000000");
    }
    tailTextColor.setAlphaF(0.5);

    int currentX = startX;
    for (auto tailIndex : tailMap.keys()) {
        if (tailIndex != 0)
            currentX += TailDataMargin;
        drawTailDetailedInfo(painter, tailMap.value(tailIndex), tailTextColor, tailFont, tailFontMetrics, currentX, startY);
    }

    return currentX;
}

void GrandSearchListDelegate::drawTailDetailedInfo(QPainter *painter, const QString &text, const QColor &color, const QFont &font, const QFontMetrics &fontMetrics, int &startX, int startY) const
{
    QTextDocument document;
    document.setDocumentMargin(0);
    document.setPlainText(text);
    QTextCursor cursor(&document);

    cursor.beginEditBlock();
    cursor.select(QTextCursor::LineUnderCursor);
    QTextCharFormat fmt;
    fmt.setForeground(color);
    fmt.setFont(font);
    cursor.mergeCharFormat(fmt);
    cursor.clearSelection();
    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.endEditBlock();

    int showWidth = fontMetrics.size(Qt::TextSingleLine, text).width();
    QRect rect(startX, startY, showWidth, fontMetrics.height());

    QAbstractTextDocumentLayout::PaintContext paintContext;
    painter->save();
    painter->translate(rect.topLeft());
    painter->setClipRect(rect.translated(-rect.topLeft()));
    document.documentLayout()->draw(painter, paintContext);
    painter->restore();
    startX += showWidth;
}

void GrandSearchListDelegate::calcTailShowInfo(const int totalTailWidth, int &tailCount, int &averageWidth) const
{
    Q_ASSERT(tailCount >= 1);

    while (1 != tailCount) {
        // 计算每个拖尾信息平均可显示宽度(每个拖尾信息之间显示间隔10个像素)
        averageWidth = (totalTailWidth - (tailCount - 1) * TailDataMargin) / tailCount;
        if (averageWidth > 0)
            break;
        // 宽度不够显示 (tailCount - 1) * TailDataMargin 个间隔，则减少拖尾数量继续计算
        tailCount--;
        averageWidth = totalTailWidth;
    }
}

QMap<int, QString> GrandSearchListDelegate::calcTailShowData(QStringList &strings, const int &tailCount, int averageWidth, const QFontMetrics &fontMetrics) const
{
    if (averageWidth >= TailMaxWidth) {
        averageWidth = TailMaxWidth;

        // 平均宽度大于最大限制，则每个项可用的最大宽度均为限制宽度，此时不需要计算剩余空间，再累加给截断显示项进行使用
        return calcTailShowDataByMaxWidth(strings, tailCount, averageWidth, fontMetrics);
    }

    // 平均宽度小于最大限制，则需要针对每个项，计算使用后的剩余空间，并累加后提供给被截断显示的项使用
    return calcTailShowDataByOptimalWidth(strings, tailCount, averageWidth, fontMetrics);
}

QMap<int, QString> GrandSearchListDelegate::calcTailShowDataByMaxWidth(QStringList &strings, const int &tailCount, int averageWidth, const QFontMetrics &fontMetrics) const
{
    QString tailString;   // 拖尾数据
    QString elidedTailText;   // 实际显示的拖尾数据
    QMap<int, QString> tailMap;   // 最终显示的数据--<拖尾序号，拖尾数据>

    for (int i = 0; i < tailCount; i++) {
        tailString = strings.takeFirst();
        elidedTailText = fontMetrics.elidedText(tailString, Qt::ElideLeft, averageWidth);
        if (!elidedTailText.isEmpty())
            tailMap.insert(i, elidedTailText);
    }

    return tailMap;
}

QMap<int, QString> GrandSearchListDelegate::calcTailShowDataByOptimalWidth(QStringList &strings, const int &tailCount, int averageWidth, const QFontMetrics &fontMetrics) const
{
    QString tailString;   // 拖尾数据
    QMap<int, QString> tailMap;   // 最终显示的数据--<拖尾序号，拖尾数据>
    QMap<int, QString> pendMap;   // 推迟计算的拖尾数据(仅使用平均空间时会发生截断的拖尾数据)
    int unUsedWidth = 0;   // 未使用空间
    // 提取能够全量显示的拖尾信息
    for (int i = 0; i < tailCount; i++) {
        // 从前向后，逐个取出数据进行计算
        tailString = strings.takeFirst();
        int currentTailWidth = fontMetrics.size(Qt::TextSingleLine, tailString).width();
        if (currentTailWidth < averageWidth) {
            // 该项拖尾数据能够全量显示，则保存，并累加剩余空间
            unUsedWidth += averageWidth - currentTailWidth;
            if (!tailString.isEmpty())
                tailMap.insert(i, tailString);
            continue;
        }
        pendMap.insert(i, tailString);
    }

    // 提取被截断显示的拖尾信息
    int availableWidth;   // 当前项可利用的最大宽度
    QString pendString;   // 待处理的原始信息
    QString elidedTailText;   // 截断显示的拖尾数据
    for (auto index : pendMap.keys()) {

        availableWidth = averageWidth;
        if (unUsedWidth > 0) {
            availableWidth += unUsedWidth;
            unUsedWidth = 0;

            if (availableWidth > TailMaxWidth) {
                // 退还超出限制的空间
                unUsedWidth = availableWidth - TailMaxWidth;
                availableWidth = TailMaxWidth;
            }
        }

        pendString = pendMap.value(index);
        elidedTailText = fontMetrics.elidedText(pendString, Qt::ElideLeft, availableWidth);
        if (elidedTailText == pendString) {
            // 未截断显示，说明设置的可用空间还有剩余，需要退还给未使用空间
            int elidedTailWidth = fontMetrics.size(Qt::TextSingleLine, pendString).width();
            unUsedWidth += availableWidth - elidedTailWidth;
        }

        if (!elidedTailText.isEmpty())
            tailMap.insert(index, elidedTailText);
        pendMap.remove(index);
    }

    return tailMap;
}
