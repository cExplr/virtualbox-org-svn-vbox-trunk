/* $Id$ */
/** @file
 * VBox Qt GUI - UICustomFileSystemModel class declaration.
 */

/*
 * Copyright (C) 2016-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UICustomFileSystemModel_h___
#define ___UICustomFileSystemModel_h___

/* Qt includes: */
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

/* COM includes: */
#include "COMEnums.h"

enum UICustomFileSystemModelColumn
{
    UICustomFileSystemModelColumn_Name = 0,
    UICustomFileSystemModelColumn_Size,
    UICustomFileSystemModelColumn_ChangeTime,
    UICustomFileSystemModelColumn_Owner,
    UICustomFileSystemModelColumn_Permissions,
    UICustomFileSystemModelColumn_Max
};

/** A UICustomFileSystemItem instance is a tree node representing a file object (file, directory, etc). The tree contructed
    by these instances is the data source for the UICustomFileSystemModel. */
class UICustomFileSystemItem
{
public:

    /** @p data contains values to be shown in table view's colums. data[0] is assumed to be
     *  the name of the file object which is the file name including extension or name of the
     *  directory */
    explicit UICustomFileSystemItem(const QVector<QVariant> &data,
                             UICustomFileSystemItem *parentItem, KFsObjType type);
    ~UICustomFileSystemItem();

    void appendChild(UICustomFileSystemItem *child);
    void reset();
    UICustomFileSystemItem *child(int row) const;
    /** Searches for the child by path and returns it if found. */
    UICustomFileSystemItem *child(const QString &path) const;
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    void setData(const QVariant &data, int index);
    int row() const;
    UICustomFileSystemItem *parentItem();

    bool isDirectory() const;
    bool isSymLink() const;
    bool isFile() const;

    bool isOpened() const;
    void setIsOpened(bool flag);

    const QString  &path() const;
    void setPath(const QString &path);

    /** Returns true if this is directory and name is ".." */
    bool isUpDirectory() const;
    void clearChildren();

    KFsObjType  type() const;

    const QString &targetPath() const;
    void setTargetPath(const QString &path);

    bool isSymLinkToADirectory() const;
    void setIsSymLinkToADirectory(bool flag);

    bool isSymLinkToAFile() const;

    const QString &owner() const;
    void setOwner(const QString &owner);

    QString name() const;

    void setIsDriveItem(bool flag);
    bool isDriveItem() const;

    static QVector<QVariant> createTreeItemData(const QString &strName, unsigned long long size, const QDateTime &changeTime,
                                                const QString &strOwner, const QString &strPermissions);

private:

    QList<UICustomFileSystemItem*>         m_childItems;
    /** Used to find children by name */
    QMap<QString, UICustomFileSystemItem*> m_childMap;
    /** It is required that m_itemData[0] is name (QString) of the file object */
    QVector<QVariant>  m_itemData;
    UICustomFileSystemItem *m_parentItem;
    bool             m_bIsOpened;
    /** Full absolute path of the item. Without the trailing '/' */
    QString          m_strPath;
    /** If this is a symlink m_targetPath keeps the absolute path of the target */
    QString          m_strTargetPath;
    /** True if this is a symlink and the target is a directory */
    bool             m_isTargetADirectory;
    KFsObjType  m_type;
    /** True if only this item represents a DOS style drive letter item */
    bool             m_isDriveItem;
};

/** A QSortFilterProxyModel extension used in file tables. Modifies some
 *  of the base class behavior like lessThan(..) */
class UICustomFileSystemProxyModel : public QSortFilterProxyModel
{

    Q_OBJECT;

public:

    UICustomFileSystemProxyModel(QObject *parent = 0);
    void setListDirectoriesOnTop(bool fListDirectoriesOnTop);
    bool listDirectoriesOnTop() const;

protected:

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const /* override */;

private:

    bool m_fListDirectoriesOnTop;
};

/** UICustomFileSystemModel serves as the model for a file structure.
 *  it supports a tree level hierarchy which can be displayed with
 *  QTableView and/or QTreeView.*/
class UICustomFileSystemModel : public QAbstractItemModel
{

    Q_OBJECT;

signals:

    void sigItemRenamed(UICustomFileSystemItem *pItem, QString strOldName, QString strNewName);

public:

    explicit UICustomFileSystemModel(QObject *parent = 0);
    ~UICustomFileSystemModel();

    QVariant       data(const QModelIndex &index, int role) const /* override */;
    bool           setData(const QModelIndex &index, const QVariant &value, int role);

    Qt::ItemFlags  flags(const QModelIndex &index) const /* override */;
    QVariant       headerData(int section, Qt::Orientation orientation,
                              int role = Qt::DisplayRole) const /* override */;
    QModelIndex    index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const /* override */;
    QModelIndex    index(UICustomFileSystemItem* item);
    QModelIndex    parent(const QModelIndex &index) const /* override */;
    int            rowCount(const QModelIndex &parent = QModelIndex()) const /* override */;
    int            columnCount(const QModelIndex &parent = QModelIndex()) const /* override */;
    void           signalUpdate();
    QModelIndex    rootIndex() const;
    void           beginReset();
    void           endReset();
    void           reset();

    void           setShowHumanReadableSizes(bool fShowHumanReadableSizes);
    bool           showHumanReadableSizes() const;
    UICustomFileSystemItem* rootItem();
    const UICustomFileSystemItem* rootItem() const;

    static const char* strUpDirectoryString;

private:
    void                initializeTree();
    UICustomFileSystemItem    *m_pRootItem;
    void setupModelData(const QStringList &lines, UICustomFileSystemItem *parent);
    bool                m_fShowHumanReadableSizes;
};


#endif /* !___UICustomFileSystemModel_h___ */