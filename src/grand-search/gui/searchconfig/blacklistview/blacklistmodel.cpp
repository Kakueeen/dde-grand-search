// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "blacklistmodel.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logGrandSearch)

using namespace GrandSearch;

BlackListModel::BlackListModel(int rows, int columns, QObject *parent)
    : QStandardItemModel(rows, columns, parent)
{
    qCDebug(logGrandSearch) << "BlackListModel initialized successfully";
}

BlackListModel::~BlackListModel()
{
    qCDebug(logGrandSearch) << "BlackListModel destructor called - Final row count:" << rowCount();
}

Qt::ItemFlags BlackListModel::flags(const QModelIndex &index) const
{
    auto path = index.data(DATAROLE);
    auto flags = QStandardItemModel::flags(index);

    if (path.toString().isEmpty())
        flags &= ~Qt::ItemIsSelectable;
    else
        flags |= Qt::ItemIsSelectable;
    return flags;
}

QStringList BlackListModel::mimeTypes() const
{
    return QStandardItemModel::mimeTypes() << QLatin1String("text/uri-list");
}

