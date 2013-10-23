/***************************************************************************
    begin                : Wed Oct 16 2013
    copyright            : (C) 2013 by Shubham Chaudhary
    email                : shubhamchaudhary92@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <KListWidget>

#include <KAction>
#include <KActionCollection>
#include <KConfigGroup>
#include <KDebug>
#include <KLocalizedString>
#include <KToggleAction>
#include <KSqueezedTextLabel>

#include <kpushbutton.h>
#include <kiconloader.h>

#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/OpticalDisc>
#include <Solid/StorageAccess>
#include <Solid/StorageVolume>
#include <Solid/PortableMediaPlayer>

#include "sync/synclist.h"
#include "sync/syncplayer.h"
#include "actioncollection.h"
#include "filehandle.h"
#include "playlist.h"
#include "playlistitem.h"
#include "playlistinterface.h"

SyncList::SyncList(QWidget* parent): KVBox(parent)
{
    //setMinimumWidth(200);
/*
    KToggleAction *showPlayers = new KToggleAction(KIcon(QLatin1String("view-media-players")),
                                            i18n("Show &Players"), this);
    ActionCollection::actions()->addAction("showPlayers", showPlayers);
    connect(showPlayers, SIGNAL(toggled(bool)), this, SLOT(setShown(bool)));

    KConfigGroup config(KGlobal::config(), "showPlayers");
    bool shown = config.readEntry("showPlayers", true);
    showPlayers->setChecked(shown);
    setVisible(shown);
*/

    QFrame *deviceFrame = new QFrame(this);
    deviceFrame->setFrameStyle(Box | Sunken);
    deviceFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


    // Make sure that we have enough of a margin to suffice for the borders,
    // hence the "lineWidth() * 2"
    QVBoxLayout *deviceLayout = new QVBoxLayout( deviceFrame );
    deviceLayout->setMargin( deviceFrame->lineWidth() * 2 );
    deviceLayout->setSpacing( 5 );
    deviceLayout->setObjectName( QLatin1String( "deviceFrame" ));
    deviceLayout->addSpacing(5);

    //m_deviceLabel = new KSqueezedTextLabel(deviceFrame);
    //deviceLayout->addWidget(m_deviceLabel);
    //m_deviceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    //m_deviceLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //m_deviceLabel->setTextFormat(Qt::PlainText);

    deviceLayout->addSpacing(5);

    KVBox *deviceBox = new KVBox(this);
    deviceBox->setFrameStyle(Box | Sunken);
    deviceBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    QPushButton *deviceButton = new QPushButton(deviceBox);
    deviceButton->setIcon(SmallIcon("go-home"));
    deviceButton->setFlat(true);

    m_player = new SyncPlayer(this);

    deviceButton->setToolTip( i18n("Call copyPlayingToTmp"));
    connect(deviceButton, SIGNAL(clicked()), this, SLOT(m_player->callCopy()));

}

SyncList::~SyncList()
{
    saveConfig();
}

void SyncList::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "ShowPlayers");
    config.writeEntry("ShowPlayers", ActionCollection::action<KToggleAction>("showPlayers")->isChecked());
}

void SyncList::togglePlayer(bool show)
{
    if(show)
    {
        ActionCollection::action<KToggleAction>("showPlayers")->setChecked(true);
    }
}


void SyncList::initializeDevice(const QString& udi)
{
//    //m_device = Solid::Device(udi);
//    if (!m_device.isValid()) {
//        return;
//    }

//    m_access = m_device.as<Solid::StorageAccess>();
//    m_volume = m_device.as<Solid::StorageVolume>();
//    //m_disc = m_device.as<Solid::OpticalDisc>();
//    m_mtp = m_device.as<Solid::PortableMediaPlayer>();

//    //setText(m_device.description());
//    //setIcon(m_device.icon());
//    //setIconOverlays(m_device.emblems());
//    setUdi(udi);

//    if (m_access) {
//        //setUrl(m_access->filePath());
//        //QObject::connect(m_access, SIGNAL(accessibilityChanged(bool,QString)),
//        //                 m_signalHandler, SLOT(onAccessibilityChanged()));
//    } else if (m_disc) {
//        Solid::Block *block = m_device.as<Solid::Block>();
//        if (block) {
//            const QString device = block->device();
//            setUrl(QString("audiocd:/?device=%1").arg(device));
//        } else {
//            setUrl(QString("audiocd:/"));
//        }
//    } else if (m_mtp) {
//        setUrl(QString("mtp:udi=%1").arg(m_device.udi()));
//    }
}


void SyncList::setUdi(const QString& udi)
{
    //setDataValue("udi", udi);
}

QString SyncList::udi() const
{
    //return dataValue("udi").toString();
    return 0;
}


/*
Solid::Device SyncList::device() const
{
    return m_device;
}
*/


// vim: set et sw=4 tw=0 sta:
