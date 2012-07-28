/***************************************************************************
    begin                : Sat Feb 16 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2008 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "playlist.h"
#include "juk-exception.h"

#include <q3header.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <k3urldrag.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <k3popupmenu.h>
#include <klocale.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <kurl.h>
#include <kio/netaccess.h>
#include <kio/copyjob.h>
#include <kmenu.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>
#include <ktoolbarpopupaction.h>
#include <kactionmenu.h>
#include <ktoggleaction.h>
#include <kselectaction.h>

#include <QCursor>
#include <QDir>
#include <QToolTip>
#include <QFile>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QTimer>
#include <QClipboard>
#include <QTextStream>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QPixmap>
#include <QStackedWidget>
#include <id3v1genres.h>

#include <time.h>
#include <cmath>
#include <dirent.h>

#include "playlist/playlistitem.h"
#include "playlist/playlistcollection.h"
#include "playlist/playlistsearch.h"
#include "mediafiles.h"
#include "collectionlist.h"
#include "filerenamer.h"
#include "actioncollection.h"
#include "tracksequencemanager.h"
#include "tag.h"
#include "k3bexporter.h"
#include "upcomingplaylist.h"
#include "deletedialog.h"
#include "webimagefetcher.h"
#include "coverinfo.h"
#include "coverdialog.h"
#include "tagtransactionmanager.h"
#include "cache.h"

using namespace ActionCollection;

/**
 * Used to give every track added in the program a unique identifier. See
 * PlaylistItem
 */
quint32 g_trackID = 0;

/**
 * Just a shortcut of sorts.
 */

static bool manualResize()
{
    return action<KToggleAction>("resizeColumnsManually")->isChecked();
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings definition
////////////////////////////////////////////////////////////////////////////////

bool Playlist::m_visibleChanged = false;
bool Playlist::m_shuttingDown = false;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlaylistItemList Playlist::m_history;
QMap<int, PlaylistItem *> Playlist::m_backMenuItems;
int Playlist::m_leftColumn = 0;

Playlist::Playlist(PlaylistCollection *collection, const QString &name,
                   const QString &iconName) :
    QAbstractTableModel(/*collection->playlistStack()*/0),
    m_collection(collection),
    m_fetcher(new WebImageFetcher(this)),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_applySharedSettings(true),
    m_columnWidthModeChanged(false),
    m_disableColumnWidthUpdates(true),
    m_time(0),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_playlistName(name),
    m_rmbMenu(0),
    m_toolTip(0),
    m_blockDataChanged(false)
{
//     setup();
    collection->setupPlaylist(this, iconName);
}

Playlist::Playlist(PlaylistCollection *collection, const PlaylistItemList &items,
                   const QString &name, const QString &iconName) :
    QAbstractTableModel(/*collection->playlistStack()*/0),
    m_collection(collection),
    m_fetcher(new WebImageFetcher(this)),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_applySharedSettings(true),
    m_columnWidthModeChanged(false),
    m_disableColumnWidthUpdates(true),
    m_time(0),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_playlistName(name),
    m_rmbMenu(0),
    m_toolTip(0),
    m_blockDataChanged(false)
{
//     setup();
    collection->setupPlaylist(this, iconName);
    createItems(items);
}

Playlist::Playlist(PlaylistCollection *collection, const QFileInfo &playlistFile,
                   const QString &iconName) :
    QAbstractTableModel(/*collection->playlistStack()*/0),
    m_collection(collection),
    m_fetcher(new WebImageFetcher(this)),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_applySharedSettings(true),
    m_columnWidthModeChanged(false),
    m_disableColumnWidthUpdates(true),
    m_time(0),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_fileName(playlistFile.canonicalFilePath()),
    m_rmbMenu(0),
    m_toolTip(0),
    m_blockDataChanged(false)
{
//     setup();
    loadFile(m_fileName, playlistFile);
    collection->setupPlaylist(this, iconName);
}

Playlist::Playlist(PlaylistCollection *collection, bool delaySetup, int extraColumns) :
    QAbstractTableModel(/*collection->playlistStack()*/0),
    m_collection(collection),
    m_fetcher(new WebImageFetcher(this)),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_applySharedSettings(true),
    m_columnWidthModeChanged(false),
    m_disableColumnWidthUpdates(true),
    m_time(0),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_rmbMenu(0),
    m_toolTip(0),
    m_blockDataChanged(false)
{
    for(int i = 0; i < extraColumns; ++i) {
        setHeaderData(i, Qt::Horizontal, i18n("JuK")); // Placeholder text!
    }

//     setup();

    if(!delaySetup)
        collection->setupPlaylist(this, "view-media-playlist");
}

Playlist::~Playlist()
{
    // In some situations the dataChanged signal from clearItems will cause observers to
    // subsequently try to access a deleted item.  Since we're going away just remove all
    // observers.

    clearObservers();

    // clearItem() will take care of removing the items from the history,
    // so call clearItems() to make sure it happens.

    clearItems(items());

    /* delete m_toolTip; */

    if(!m_shuttingDown)
        m_collection->removePlaylist(this);
}

QString Playlist::name() const
{
    if(m_playlistName.isEmpty())
        return m_fileName.section(QDir::separator(), -1).section('.', 0, -2);
    else
        return m_playlistName;
}

FileHandle Playlist::currentFile() const
{
    return playingItem() ? playingItem()->file() : FileHandle::null();
}

int Playlist::time() const
{
    // Since this method gets a lot of traffic, let's optimize for such.

    if(!m_addTime.isEmpty()) {
        foreach(const PlaylistItem *item, m_addTime) {
            if(item)
                m_time += item->file().tag()->seconds();
        }

        m_addTime.clear();
    }

    if(!m_subtractTime.isEmpty()) {
        foreach(const PlaylistItem *item, m_subtractTime) {
            if(item)
                m_time -= item->file().tag()->seconds();
        }

        m_subtractTime.clear();
    }

    return m_time;
}

void Playlist::playFirst()
{
    TrackSequenceManager::instance()->setNextItem(m_items.first());
    action("forward")->trigger();
}

void Playlist::playNextAlbum()
{
    PlaylistItem *current = TrackSequenceManager::instance()->currentItem();
    if(!current)
        return; // No next album if we're not already playing.

    QString currentAlbum = current->file().tag()->album();
    current = TrackSequenceManager::instance()->nextItem();

    while(current && current->file().tag()->album() == currentAlbum)
        current = TrackSequenceManager::instance()->nextItem();

    TrackSequenceManager::instance()->setNextItem(current);
    action("forward")->trigger();
}

void Playlist::playNext()
{
    TrackSequenceManager::instance()->setCurrentPlaylist(this);
    setPlaying(TrackSequenceManager::instance()->nextItem());
}

void Playlist::stop()
{
    m_history.clear();
    setPlaying(0);
}

void Playlist::playPrevious()
{
    if(!playingItem())
        return;

    bool random = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();

    PlaylistItem *previous = 0;

    if(random && !m_history.isEmpty()) {
        PlaylistItemList::Iterator last = --m_history.end();
        previous = *last;
        m_history.erase(last);
    }
    else {
        m_history.clear();
        previous = TrackSequenceManager::instance()->previousItem();
    }

    
    if(!previous) {
        int index = m_items.indexOf(playingItem()) - 1;
        if (index < 0) index = m_items.size();
        previous = m_items[index];
    }

    setPlaying(previous, false);
}

void Playlist::setName(const QString &n)
{
    m_collection->addNameToDict(n);
    m_collection->removeNameFromDict(m_playlistName);

    m_playlistName = n;
    emit signalNameChanged(m_playlistName);
}

// ### TODO: View
void Playlist::save()
{
    if(m_fileName.isEmpty())
        return saveAs();

    QFile file(m_fileName);

    if(!file.open(QIODevice::WriteOnly))
        return KMessageBox::error(/*this*/0, i18n("Could not save to file %1.", m_fileName));

    QTextStream stream(&file);

    QStringList fileList = files();

    foreach(const QString &file, fileList)
        stream << file << endl;

    file.close();
}

void Playlist::saveAs()
{
    m_collection->removeFileFromDict(m_fileName);

    m_fileName = MediaFiles::savePlaylistDialog(name()/*, this*/);

    if(!m_fileName.isEmpty()) {
        m_collection->addFileToDict(m_fileName);

        // If there's no playlist name set, use the file name.
        if(m_playlistName.isEmpty())
            emit signalNameChanged(name());
        save();
    }
}

void Playlist::updateDeletedItem(PlaylistItem *item)
{
    m_members.remove(item->file().absFilePath());
    m_search.clearItem(item);

    m_history.removeAll(item);
    m_addTime.removeAll(item);
    m_subtractTime.removeAll(item);
}

void Playlist::clearItem(PlaylistItem *item, bool emitChanged)
{
    // Automatically updates internal structs via updateDeletedItem
    delete item;

    dataChanged();
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    foreach(PlaylistItem *item, items) {
        m_items.removeAll(item);
        delete item;
    }

    
    dataChanged();
}

PlaylistItem *Playlist::playingItem() // static
{
    return PlaylistItem::playingItems().isEmpty() ? 0 : PlaylistItem::playingItems().front();
}

QStringList Playlist::files() const
{
    QStringList list;

    foreach(PlaylistItem *item, m_items) {
        list.append(item->file().absFilePath());
    }

    return list;
}

void Playlist::synchronizePlayingItems(const PlaylistList &sources, bool setMaster)
{
    foreach(const Playlist *p, sources) {
        if(p->playing()) {
            CollectionListItem *base = playingItem()->collectionItem();
            foreach(PlaylistItem *item, m_items) {
                if(base == item->collectionItem()) {
                    item->setPlaying(true, setMaster);
                    PlaylistItemList playing = PlaylistItem::playingItems();
                    TrackSequenceManager::instance()->setCurrent(item);
                    return;
                }
            }
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////
void Playlist::slotReload()
{
    QFileInfo fileInfo(m_fileName);
    if(!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
        return;

    clearItems(items());
    loadFile(m_fileName, fileInfo);
}

//### TODO: Nuke me
void Playlist::dataChanged()
{
    if(m_blockDataChanged)
        return;
    PlaylistInterface::dataChanged();
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////
void Playlist::applySharedSettings()
{
    m_applySharedSettings = true;
}

void Playlist::read(QDataStream &s)
{
    s >> m_playlistName
      >> m_fileName;

    // m_fileName is probably empty.
    if(m_playlistName.isEmpty())
        throw BICStreamException();

    // Do not sort. Add the files in the order they were saved.
//     setSorting(columns() + 1);

    QStringList files;
    s >> files;

    PlaylistItem *after = 0;

    m_blockDataChanged = true;

    foreach(const QString &file, files) {
        if(file.isEmpty())
            throw BICStreamException();

        after = createItem(FileHandle(file), after, false);
    }

    m_blockDataChanged = false;

    dataChanged();
    m_collection->setupPlaylist(this, "view-media-playlist");
}

PlaylistItem *Playlist::createItem(const FileHandle &file,
                                   PlaylistItem *after, bool emitChanged)
{
    return createItem<PlaylistItem>(file, after, emitChanged);
}

void Playlist::createItems(const PlaylistItemList &siblings, PlaylistItem *after)
{
    createItems<PlaylistItem, PlaylistItem>(siblings, after);
}

void Playlist::addFiles(const QStringList &files, PlaylistItem *after)
{
    if(!after && !m_items.isEmpty())
        after = m_items.last();

    KApplication::setOverrideCursor(Qt::waitCursor);

    m_blockDataChanged = true;

    FileHandleList queue;

    foreach(const QString &file, files)
        addFile(file, queue, true, &after);

    addFileHelper(queue, &after, true);

    m_blockDataChanged = false;

//     slotWeightDirty();
    dataChanged();

    KApplication::restoreOverrideCursor();
}

void Playlist::refreshAlbums(const PlaylistItemList &items, coverKey id)
{
    QList< QPair<QString, QString> > albums;
    bool setAlbumCovers = items.count() == 1;

    foreach(const PlaylistItem *item, items) {
        QString artist = item->file().tag()->artist();
        QString album = item->file().tag()->album();

        if(!albums.contains(qMakePair(artist, album)))
            albums.append(qMakePair(artist, album));

        item->file().coverInfo()->setCoverId(id);
        if(setAlbumCovers)
            item->file().coverInfo()->applyCoverToWholeAlbum(true);
    }

    for(QList< QPair<QString, QString> >::ConstIterator it = albums.constBegin();
        it != albums.constEnd(); ++it)
    {
        refreshAlbum((*it).first, (*it).second);
    }
}

void Playlist::refreshAlbum(const QString &artist, const QString &album)
{
    ColumnList columns;
    columns.append(ArtistColumn);
    PlaylistSearch::Component artistComponent(artist, false, columns,
                                              PlaylistSearch::Component::Exact);

    columns.clear();
    columns.append(AlbumColumn);
    PlaylistSearch::Component albumComponent(album, false, columns,
                                             PlaylistSearch::Component::Exact);

    PlaylistSearch::ComponentList components;
    components.append(artist);
    components.append(album);

    PlaylistList playlists;
    playlists.append(CollectionList::instance());

    PlaylistSearch search(playlists, components);
    const PlaylistItemList matches = search.matchedItems();

    foreach(PlaylistItem *item, matches)
        item->refresh();
}

void Playlist::setupItem(PlaylistItem *item)
{
    item->setTrackId(g_trackID);
    g_trackID++;

    //### TODO: View
//     if(!m_search.isEmpty())
//         item->setVisible(m_search.checkItem(item));

//     if(childCount() <= 2 && !manualResize()) {
//         slotWeightDirty();
//         slotUpdateColumnWidths();
//         triggerUpdate();
//     }
}

void Playlist::setDynamicListsFrozen(bool frozen)
{
    m_collection->setDynamicListsFrozen(frozen);
}

CollectionListItem *Playlist::collectionListItem(const FileHandle &file)
{
    if(!QFile::exists(file.absFilePath())) {
        kError() << "File" << file.absFilePath() << "does not exist.";
        return 0;
    }

    CollectionListItem *item = CollectionList::instance()->lookup(file.absFilePath());

    if(!item) {
        item = CollectionList::instance()->createItem(file);
    }

    return item;
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotPopulateBackMenu() const
{
    if(!playingItem())
        return;

    QMenu *menu = action<KToolBarPopupAction>("back")->menu();
    menu->clear();
    m_backMenuItems.clear();

    int count = 0;
    PlaylistItemList::ConstIterator it = m_history.constEnd();

    QAction *action;

    while(it != m_history.constBegin() && count < 10) {
        ++count;
        --it;
        action = new QAction((*it)->file().tag()->title(), menu);
        action->setData(count);
        menu->addAction(action);
        m_backMenuItems[count] = *it;
    }
}

void Playlist::slotPlayFromBackMenu(QAction *backAction) const
{
    int number = backAction->data().toInt();

    if(!m_backMenuItems.contains(number))
        return;

    TrackSequenceManager::instance()->setNextItem(m_backMenuItems[number]);
    action("forward")->trigger();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::loadFile(const QString &fileName, const QFileInfo &fileInfo)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&file);

    // Turn off non-explicit sorting.

//     setSorting(PlaylistItem::lastColumn() + columnOffset() + 1);

    PlaylistItem *after = 0;

    m_disableColumnWidthUpdates = true;

    m_blockDataChanged = true;

    while(!stream.atEnd()) {
        QString itemName = stream.readLine().trimmed();

        QFileInfo item(itemName);

        if(item.isRelative())
            item.setFile(QDir::cleanPath(fileInfo.absolutePath() + '/' + itemName));

        if(item.exists() && item.isFile() && item.isReadable() &&
           MediaFiles::isMediaFile(item.fileName()))
        {
//             if(after)
                after = createItem(FileHandle(item, item.absoluteFilePath()), after, false);
//             else
//                 after = createItem(FileHandle(item, item.absoluteFilePath()), 0, false);
        }
    }

    m_blockDataChanged = false;

    file.close();

    dataChanged();

    m_disableColumnWidthUpdates = false;
}

void Playlist::setPlaying(PlaylistItem *item, bool addToHistory)
{
    if(playingItem() == item)
        return;

    if(playingItem()) {
        if(addToHistory) {
            if(playingItem()->playlist() ==
               playingItem()->playlist()->m_collection->upcomingPlaylist())
                m_history.append(playingItem()->collectionItem());
            else
                m_history.append(playingItem());
        }
        playingItem()->setPlaying(false);
    }

    TrackSequenceManager::instance()->setCurrent(item);
#ifdef __GNUC__
#warning "kde4: port it"
#endif
    //kapp->dcopClient()->emitDCOPSignal("Player", "trackChanged()", data);

    if(!item)
        return;

    item->setPlaying(true);

    bool enableBack = !m_history.isEmpty();
    action<KToolBarPopupAction>("back")->menu()->setEnabled(enableBack);
}

bool Playlist::playing() const
{
    return playingItem() && this == playingItem()->playlist();
}

void Playlist::addFile(const QString &file, FileHandleList &files, bool importPlaylists,
                       PlaylistItem **after)
{
    if(hasItem(file) && !m_allowDuplicates)
        return;

    processEvents();
    addFileHelper(files, after);

    // Our biggest thing that we're fighting during startup is too many stats
    // of files.  Make sure that we don't do one here if it's not needed.

    FileHandle cached = Cache::instance()->value(file);

    if(!cached.isNull()) {
        cached.tag();
        files.append(cached);
        return;
    }


    const QFileInfo fileInfo = QDir::cleanPath(file);
    if(!fileInfo.exists())
        return;

    if(fileInfo.isFile() && fileInfo.isReadable()) {
        if(MediaFiles::isMediaFile(file)) {
            FileHandle f(fileInfo, fileInfo.absoluteFilePath());
            f.tag();
            files.append(f);
        }
    }

    if(importPlaylists && MediaFiles::isPlaylistFile(file) &&
       !m_collection->containsPlaylistFile(fileInfo.canonicalFilePath()))
    {
        new Playlist(m_collection, fileInfo);
        return;
    }

    if(fileInfo.isDir()) {

        // Resorting to the POSIX API because QDir::listEntries() stats every
        // file and blocks while it's doing so.

        DIR *dir = ::opendir(QFile::encodeName(fileInfo.filePath()));

        if(dir) {
            struct dirent *dirEntry;

            for(dirEntry = ::readdir(dir); dirEntry; dirEntry = ::readdir(dir)) {
                if(strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) {

                    // We set importPlaylists to the value from the add directories
                    // dialog as we want to load all of the ones that the user has
                    // explicitly asked for, but not those that we find in toLower
                    // directories.

                    addFile(fileInfo.filePath() + QDir::separator() + QFile::decodeName(dirEntry->d_name),
                            files, m_collection->importPlaylists(), after);
                }
            }
            ::closedir(dir);
        }
        else {
            kWarning() << "Unable to open directory "
                            << fileInfo.filePath()
                            << ", make sure it is readable.\n";
        }
    }
}

void Playlist::addFileHelper(FileHandleList &files, PlaylistItem **after, bool ignoreTimer)
{
    static QTime time = QTime::currentTime();

    // Process new items every 10 seconds, when we've loaded 1000 items, or when
    // it's been requested in the API.

    if(ignoreTimer || time.elapsed() > 10000 ||
       (files.count() >= 1000 && time.elapsed() > 1000))
    {
        time.restart();

        foreach(const FileHandle &fileHandle, files)
            *after = createItem(fileHandle, *after, false);

        files.clear();


        processEvents();
    }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

bool Playlist::editTag(PlaylistItem *item, const QString &text, int column)
{
    Tag *newTag = TagTransactionManager::duplicateTag(item->file().tag());

    switch(column - columnOffset())
    {
    case TrackColumn:
        newTag->setTitle(text);
        break;
    case ArtistColumn:
        newTag->setArtist(text);
        break;
    case AlbumColumn:
        newTag->setAlbum(text);
        break;
    case TrackNumberColumn:
    {
        bool ok;
        int value = text.toInt(&ok);
        if(ok)
            newTag->setTrack(value);
        break;
    }
    case GenreColumn:
        newTag->setGenre(text);
        break;
    case YearColumn:
    {
        bool ok;
        int value = text.toInt(&ok);
        if(ok)
            newTag->setYear(value);
        break;
    }
    }

    TagTransactionManager::instance()->changeTagOnItem(item, newTag);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Playlist &p)
{
    s << p.name();
    s << p.fileName();
    s << p.files();

    return s;
}

QDataStream &operator>>(QDataStream &s, Playlist &p)
{
    p.read(s);
    return s;
}

bool processEvents()
{
    static QTime time = QTime::currentTime();

    if(time.elapsed() > 100) {
        time.restart();
        kapp->processEvents();
        return true;
    }
    return false;
}


// ---- QAbstractListModel API
int Playlist::columnCount(const QModelIndex& parent) const
{
    return 11;
}

QVariant Playlist::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole) {
        // ### TODO: FIXME
//         return *(m_items[index.row()]->pixmap(index.column()));
    }
    
    if (role != Qt::DisplayRole)
        return QVariant();
    
    FileHandle fileHandle = m_items[index.row()]->file();
    switch(index.column()) {
    case TrackColumn:
        return fileHandle.tag()->title();
    case ArtistColumn:
        return fileHandle.tag()->artist();
    case AlbumColumn:
        return fileHandle.tag()->album();
    case CoverColumn:
        return QString();
    case TrackNumberColumn:
        return fileHandle.tag()->track() > 0
            ? QString::number(fileHandle.tag()->track())
            : QString();
    case GenreColumn:
        return fileHandle.tag()->genre();
    case YearColumn:
        return fileHandle.tag()->year() > 0
            ? QString::number(fileHandle.tag()->year())
            : QString();
    case LengthColumn:
        return fileHandle.tag()->lengthString();
    case BitrateColumn:
        return QString::number(fileHandle.tag()->bitrate());
    case CommentColumn:
        return fileHandle.tag()->comment();
    case FileNameColumn:
        return fileHandle.fileInfo().fileName();
    case FullPathColumn:
        return fileHandle.fileInfo().absoluteFilePath();
    default:
        return QString();
    }
}

int Playlist::rowCount(const QModelIndex& parent) const
{
    return m_items.count();
}

Qt::ItemFlags Playlist::flags(const QModelIndex& index) const
{
    return QAbstractItemModel::flags(index);
}

bool Playlist::setData(const QModelIndex& index, const QVariant& value, int role)
{
    return QAbstractItemModel::setData(index, value, role);
}

bool Playlist::insertRows(int row, int count, const QModelIndex& parent)
{
    return QAbstractItemModel::insertRows(row, count, parent);
}

bool Playlist::removeRows(int row, int count, const QModelIndex& parent)
{
    return QAbstractItemModel::removeRows(row, count, parent);
}

QVariant Playlist::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    
    switch(section) {
    case TrackColumn:
        return i18n("Track Name");
    case ArtistColumn:
        return i18n("Artist");
    case AlbumColumn:
        return i18n("Album");
    case CoverColumn:
        return i18n("Cover");
    case TrackNumberColumn:
        return i18nc("cd track number", "Track");
    case GenreColumn:
        return i18n("Genre");
    case YearColumn:
        return i18n("Year");
    case LengthColumn:
        return i18n("Length");
    case BitrateColumn:
        return i18n("Bitrate");
    case CommentColumn:
        return i18n("Comment");
    case FileNameColumn:
        return i18n("File Name");
    case FullPathColumn:
        return i18n("File Name (full path)");
    default:
        return QVariant();
    }
}

bool Playlist::hasChildren(const QModelIndex &index) const
{
    return (!index.isValid());
}


#include "playlist.moc"

// vim: set et sw=4 tw=0 sta: