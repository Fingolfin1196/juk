/***************************************************************************
                          systray.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Daniel Molkentin <molkentin@kde.org>
                           (C) 2002, 2003 Scott Wheeler <wheeler@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>
#include <kiconloader.h>
#include <kpassivepopup.h>
#include <kiconeffect.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kglobalsettings.h>
#include <kdebug.h>

#include <qhbox.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qlayout.h>
#include <qfileinfo.h>

#include "tag.h"
#include "systemtray.h"
#include "actioncollection.h"
#include "playermanager.h"

using namespace ActionCollection;

static bool copyImage(QImage &dest, QImage &src, int x, int y);

class PassiveInfo : public KPassivePopup
{
public:
    PassiveInfo(QWidget *parent = 0, const char *name = 0) :
        KPassivePopup(parent, name) {}

protected:
    virtual void enterEvent(QEvent *)
    {
        setTimeout(3000000); // Make timeout damn near infinite
    }
    
    virtual void leaveEvent(QEvent *)
    {
        setTimeout(250); // Close quickly
    }
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(QWidget *parent, const char *name) : KSystemTray(parent, name),
                                                            m_popup(0)

{
    m_appPix = loadIcon("juk_dock");

    m_playPix = createPixmap("player_play");
    m_pausePix = createPixmap("player_pause");

    m_backPix = loadIcon("player_start");
    m_forwardPix = loadIcon("player_end");

    setPixmap(m_appPix);

    setToolTip();

    KPopupMenu *cm = contextMenu();

    connect(PlayerManager::instance(), SIGNAL(signalPlay()), this, SLOT(slotPlay()));
    connect(PlayerManager::instance(), SIGNAL(signalPause()), this, SLOT(slotPause()));
    connect(PlayerManager::instance(), SIGNAL(signalStop()), this, SLOT(slotStop()));

    action("play")->plug(cm);
    action("pause")->plug(cm);
    action("stop")->plug(cm);
    action("back")->plug(cm);
    action("forward")->plug(cm);

    cm->insertSeparator();

    action("randomPlay")->plug(cm);
    action("togglePopups")->plug(cm);

    if(PlayerManager::instance()->playing())
        slotPlay();
    else if(PlayerManager::instance()->paused())
        slotPause();
}

SystemTray::~SystemTray()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotPlay()
{
    setPixmap(m_playPix);
    setToolTip(PlayerManager::instance()->playingString());
    createPopup();
}

void SystemTray::slotStop()
{
    setPixmap(m_appPix);
    setToolTip();

    delete m_popup;
    m_popup = 0;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SystemTray::createPopup(bool addButtons)
{
    FileHandle playingFile = PlayerManager::instance()->playingFile();
    Tag *playingInfo = playingFile.tag();
    
    // If the action exists and it's checked, do our stuff
    if(action<KToggleAction>("togglePopups")->isChecked()) {

        delete m_popup;
        m_popup = new PassiveInfo(this);

        QHBox *box = new QHBox(m_popup);
        box->setSpacing(15); // Add space between text and buttons

        // See where to put the buttons
        bool onLeft = buttonsToLeft();
        QVBox *buttonBox;

        if(onLeft) {
            // They go to the left because JuK is on that side
            buttonBox = new QVBox(box);
            buttonBox->setSpacing(3);

            // Separator line
            QFrame *line = new QFrame(box);
            line->setFrameShape(QFrame::VLine);
        }

        QVBox *infoBox = new QVBox(box);

        infoBox->setSpacing(3);
        infoBox->setMargin(3);
        
        // We need to add QLabels to replace any missing labels.
        int numSpacers = 0;
        
        QString titleStr = playingInfo->title();
        QLabel *title = new QLabel(titleStr, infoBox);
        title->setAlignment(AlignRight | AlignVCenter);

        // Give the title a bold, bigger font to make it look better.
        QFont f = title->font();
        f.setBold(true);
        f.setPointSize(f.pointSize() + 4);
        title->setFont(f);
        
        // Artist info
        QString artistStr = playingInfo->artist();
        QLabel *artist = 0;
        if(!artistStr.isEmpty()) {
            artist = new QLabel(infoBox);
            artist->setAlignment(AlignRight | AlignVCenter);
        }
        else
            ++numSpacers;
                
        // Album info
        QString albumStr = playingInfo->album();
        QLabel *album = 0;
        if(!albumStr.isEmpty()) {
            int year = playingInfo->year();
            
            // See if there is a year.
            if(year > 0)
                albumStr = i18n("%1 (%2)").arg(albumStr).arg(year);
            
            album = new QLabel(infoBox);
            album->setAlignment(AlignRight | AlignVCenter);
        }
        else
            ++numSpacers;

        for( ; numSpacers != 0; --numSpacers) {
            // Add an empty label.  QSpacerItems weren't working for
            // me for some reason.  QBoxLayout->addSpacing() wasn't
            // either. mpyne :-(
            (void) new QLabel(" ", infoBox);
        }

        if(addButtons) {
            if(!onLeft) {
                // Add separator line
                QFrame *line = new QFrame(box);
                line->setFrameShape(QFrame::VLine);

                buttonBox = new QVBox(box);
                buttonBox->setSpacing(3);
            }
            
            QPushButton *backButton = new QPushButton(m_backPix, 0, buttonBox, "popup_back");
            backButton->setFlat(true);
            connect(backButton, SIGNAL(clicked()), action("back"), SLOT(activate()));

            QPushButton *forwardButton = new QPushButton(m_forwardPix, 0, buttonBox, "popup_forward");
            forwardButton->setFlat(true);
            connect(forwardButton, SIGNAL(clicked()), action("forward"), SLOT(activate()));
        }

        // We don't want an autodelete popup.  There are times when it will need
        // to be hidden before the timeout.
        m_popup->setAutoDelete(false);

        // We have to set the text of the labels after all of the
        // widgets have been added in order for the width to be calculated
        // correctly.
        title->setText(playingInfo->title());
        if(artist)
            artist->setText(artistStr);
        if(album)
            album->setText(albumStr);
        
        m_popup->setView(box);
        m_popup->show();
    }
}

bool SystemTray::buttonsToLeft() const
{
    QPoint center = mapToGlobal(geometry().center());
    QRect bounds = KGlobalSettings::desktopGeometry(center);
    int middle = bounds.center().x();

    // This seems to accurately guess what side of the icon that
    // KPassivePopup will popup on.
    return((center.x() - (width() / 2)) < middle);
}

QPixmap SystemTray::createPixmap(const QString &pixName)
{
    QPixmap bgPix = m_appPix;
    QPixmap fgPix = SmallIcon(pixName);

    QImage bgImage = bgPix.convertToImage(); // Probably 22x22
    QImage fgImage = fgPix.convertToImage(); // Should be 16x16

    KIconEffect::semiTransparent(bgImage);
    KIconEffect::semiTransparent(bgImage);
    copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
              (bgImage.height() - fgImage.height()) / 2);

    bgPix.convertFromImage(bgImage);
    return bgPix;
}

void SystemTray::setToolTip(const QString &tip)
{
    QToolTip::remove(this);

    if(tip.isNull())
        QToolTip::add(this, "JuK");
    else
        QToolTip::add(this, tip);
}

void SystemTray::wheelEvent(QWheelEvent *e)
{
    if(e->orientation() == Horizontal)
        return;

    // I already know the type here, but this file doesn't (and I don't want it
    // to) know about the JuK class, so a static_cast won't work, and I was told
    // that a reinterpret_cast isn't portable when combined with multiple
    // inheritance.  (This is why I don't check the result.)

    switch(e->state()) {
    case ShiftButton:
        if(e->delta() > 0)
            action("volumeUp")->activate();
        else
            action("volumeDown")->activate();
        break;
    default:
        if(e->delta() > 0)
            action("forward")->activate();
        else
            action("back")->activate();
        break;
    }
    e->accept();
}

/*
 * Reimplemented this in order to use the middle mouse button
 */
void SystemTray::mousePressEvent(QMouseEvent *e)
{
    switch(e->button()) {
    case LeftButton:
    case RightButton:
    default:
        KSystemTray::mousePressEvent(e);
        break;
    case MidButton:
        if(!rect().contains(e->pos()))
            return;
        if(action("pause")->isEnabled())
            action("pause")->activate();
        else
            action("play")->activate();
        break;
    }
}

/*
 * This function copies the entirety of src into dest, starting in
 * dest at x and y.  This function exists because I was unable to find
 * a function like it in either QImage or kdefx
 */

static bool copyImage(QImage &dest, QImage &src, int x, int y)
{
    if(dest.depth() != src.depth())
        return false;
    if((x + src.width()) >= dest.width())
        return false;
    if((y + src.height()) >= dest.height())
        return false;

    // We want to use KIconEffect::overlay to do this, since it handles
    // alpha, but the images need to be the same size.  We can handle that.

    QImage large_src(dest);

    // It would perhaps be better to create large_src based on a size, but
    // this is the easiest way to make a new image with the same depth, size,
    // etc.

    large_src.detach();

    // However, we do have to specifically ensure that setAlphaBuffer is set
    // to false

    large_src.setAlphaBuffer(false);
    large_src.fill(0); // All transparent pixels
    large_src.setAlphaBuffer(true);

    int w = src.width();
    int h = src.height();
    for(int dx = 0; dx < w; dx++)
        for(int dy = 0; dy < h; dy++)
            large_src.setPixel(dx + x, dy + y, src.pixel(dx, dy));

    // Apply effect to image

    KIconEffect::overlay(dest, large_src);

    return true;
}

#include "systemtray.moc"

// vim: ts=8
