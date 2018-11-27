/* $Id$ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class declaration.
 */

/*
 * Copyright (C) 2010-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIGuestControlFileManagerOptionsPanel_h___
#define ___UIGuestControlFileManagerOptionsPanel_h___

/* GUI includes: */
#include "UIGuestControlFileManagerPanel.h"

/* Forward declarations: */
class QCheckBox;
class QSpinBox;
class QLabel;
class QIToolButton;
class UIGuestControlFileManagerOptions;

/** UIGuestControlFileManagerPanel extension to change file manager options. It directly
 *  modifies the options through the passed UIGuestControlFileManagerOptions instance. */
class UIGuestControlFileManagerOptionsPanel : public UIGuestControlFileManagerPanel
{
    Q_OBJECT;

public:

    UIGuestControlFileManagerOptionsPanel(UIGuestControlFileManager *pManagerWidget,
                                           QWidget *pParent, UIGuestControlFileManagerOptions *pFileManagerOptions);
    virtual QString panelName() const /* override */;
    /** Reads the file manager options and updates the widget accordingly. This functions is typically called
     *  when file manager options have been changed by other means and this panel needs to adapt. */
    void update();

signals:

    void sigOptionsChanged();

protected:

    virtual void prepareWidgets() /* override */;
    virtual void prepareConnections() /* override */;

    /** Handles the translation event. */
    void retranslateUi();

private slots:

    void sltListDirectoryCheckBoxToogled(bool bChecked);
    void sltDeleteConfirmationCheckBoxToogled(bool bChecked);
    void sltHumanReabableSizesCheckBoxToogled(bool bChecked);

private:

    QCheckBox  *m_pListDirectoriesOnTopCheckBox;
    QCheckBox  *m_pDeleteConfirmationCheckBox;
    QCheckBox  *m_pHumanReabableSizesCheckBox;
    UIGuestControlFileManagerOptions *m_pFileManagerOptions;
};

#endif /* !___UIGuestControlFileManagerOptionsPanel_h___ */