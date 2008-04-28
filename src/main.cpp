/*******************************************************************************
  Copyright 2008 Ian Wadham <ianw2@optusnet.com.au>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*******************************************************************************/

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocale>

#include "kubrick.h"

static const char description [] =
    I18N_NOOP ("A game based on Rubik's Cube (TM)");

static const char version [] = "0.3";

int main(int argc, char **argv)
{
    KAboutData about ("kubrick", 0, ki18n ("Kubrick"),
		      version, ki18n (description),
		      KAboutData::License_GPL,
		      ki18n ("(C) 2008 Ian Wadham"), KLocalizedString(),
				"http://games.kde.org/kubrick" );
    about.addAuthor  (ki18n ("Ian Wadham"), ki18n ("Author"),
                             "ianw2@optusnet.com.au");

    KCmdLineArgs::init (argc, argv, &about);

    KApplication app;
    Kubrick * mainWindow = 0;

    // Get access to KDE Games library string translations.
    KGlobal::locale()->insertCatalog("libkdegames");

    if (app.isSessionRestored ()) {
        RESTORE (Kubrick);
    }
    else {
        // No interrupted session ... just start up normally.
        // KCmdLineArgs *args = KCmdLineArgs::parsedArgs ();

        /// @todo do something with the command line args here

        mainWindow = new Kubrick ();
        mainWindow->show ();

        // if (args->count() > 0) {
            // args->clear();
        // }
    }

    // MainWindow has WDestructiveClose flag by default, so
    // it will delete itself.
    return app.exec ();
}
