// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MODELMANAGEBUTTON_H
#define MODELMANAGEBUTTON_H

#include <DPushButton>

namespace GrandSearch {

enum ModelStatus {
  None = 0,
  Install,
  Uninstall,
  InstallAndUpdate
};

class ModelManageButton : public DTK_WIDGET_NAMESPACE::DPushButton
{
    Q_OBJECT
public:
    ModelManageButton(QWidget * parent = nullptr);
    ModelManageButton(const QString& text, QWidget * parent = nullptr);

    void updateRectSize();

protected:
    void paintEvent(QPaintEvent* e) Q_DECL_OVERRIDE;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    bool m_isHover = false;
};
}

#endif // MODELMANAGEBUTTON_H
