/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <kuniqueapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <dcopclient.h>

#include "juk.h"

static const char description[] = I18N_NOOP("Jukebox and music manager for KDE");
static const char scott[]       = I18N_NOOP("Author, chief dork and keeper of the funk");
static const char daniel[]      = I18N_NOOP("System tray docking, \"inline\" tag editing,\nbug fixes, evangelism, moral support");
static const char tim[]         = I18N_NOOP("GStreamer port");
static const char stefan[]      = I18N_NOOP("Global keybindings support");
static const char stephan[]     = I18N_NOOP("Track announcement popups");
static const char frerich[]     = I18N_NOOP("Automagic track data guessing, bugfixes");
static const char zack[]        = I18N_NOOP("More automagical things, now using MusicBrainz");

static KCmdLineOptions options[] =
{
#ifndef NO_DEBUG
    { "norestore", I18N_NOOP("Restore playlists.  Use --norestore for debugging."), 0 },
#endif
    { "+[file(s)]", I18N_NOOP("File(s) to open"), 0 },
    { 0, 0, 0 }
};

int main(int argc, char *argv[])
{
    KAboutData aboutData("juk", I18N_NOOP("JuK"),
                         "1.90 (2.0 Pre-Alpha)", description, KAboutData::License_GPL,
                         "(c) 2002, 2003, Scott Wheeler", 0, "http://www.slackorama.net/oss/juk/");

    aboutData.addAuthor("Scott Wheeler", scott, "wheeler@kde.org");
    aboutData.addCredit("Daniel Molkentin", daniel, "molkentin@kde.org");
    aboutData.addCredit("Tim Jansen", tim, "tim@tjansen.de");
    aboutData.addCredit("Stefan Asserhäll", stefan, "stefan.asserhall@telia.com");
    aboutData.addCredit("Stephan Douglas", stephan, "stephen_douglas@yahoo.com");
    aboutData.addCredit("Frerich Raabe", frerich, "raabe@kde.org");
    aboutData.addCredit("Zack Rusin", zack, "zack@kde.org");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addCmdLineOptions(options);

    KUniqueApplication::addCmdLineOptions();

    KUniqueApplication a;

    // Here we do some DCOP locking of sorts to prevent incoming DCOP calls
    // before JuK has finished its initialization.
    
    a.dcopClient()->suspend();
    JuK *juk = new JuK();
    a.dcopClient()->resume();

    a.setMainWidget(juk);
    juk->show();

    return a.exec();
}
