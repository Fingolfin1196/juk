/***************************************************************************
    begin                : Sun Mar 6 2003
    copyright            : (C) 2003 by Richard L�rk�ng
    email                : nouseforaname@home.se

    copyright            : (C) 2003 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <ktoolbar.h>

#include <qhbox.h>

#include "playlistsearch.h"

class QCheckBox;

class KComboBox;

class Playlist;

class SearchLine : public QHBox
{
    Q_OBJECT

public:
    enum Mode { Default = 0, CaseSensitive = 1, Pattern = 2 };

    SearchLine(QWidget *parent, bool simple = false, const char *name = 0);
    virtual ~SearchLine() {}

    PlaylistSearch::Component searchComponent() const;
    void setSearchComponent(const PlaylistSearch::Component &component);

    void updateColumns();

public slots:
    void clear();

signals:
    void signalQueryChanged();

private slots:
    void slotActivate();

private:
    bool m_simple;
    KLineEdit *m_lineEdit;
    KComboBox *m_searchFieldsBox;
    KComboBox *m_caseSensitive;
    QValueList<int> m_columnList;
};

class SearchWidget : public KToolBar
{
    Q_OBJECT

public:
    SearchWidget(QWidget *parent, const char *name = 0);
    virtual ~SearchWidget();

    PlaylistSearch search(const PlaylistList &playlists) const;
    void setSearch(const PlaylistSearch &search);

public slots:
    void clear();
    void setEnabled(bool enable);

signals:
    void signalQueryChanged();
    void signalAdvancedSearchClicked();

    // This signal is only emitted when the Show/Hide action is triggered.
    // Minimizing/closing the JuK window will not trigger this signal.

    void signalShown(bool shown);

private:
    void updateColumns();
    void setupLayout();

private:
    SearchLine *m_searchLine;
    QStringList m_columnHeaders;
};

#endif
