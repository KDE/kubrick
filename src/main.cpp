/*******************************************************************************
  Copyright 2008 Ian Wadham <iandw.au@gmail.com>

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

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>
#include <Kdelibs4ConfigMigrator>
#include <KLocalizedString>

#include "kubrick.h"

static const char description [] =
    I18N_NOOP ("A game based on Rubik's Cube (TM)");

static const char version [] = "1.1";

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kubrick");

    KAboutData about ("kubrick", i18n ("Kubrick"),
            version, i18n (description),
            KAboutLicense::GPL,
            i18n ("&copy; 2008 Ian Wadham"),
            "http://kde.org/applications/games/kubrick/" );
    about.addAuthor (i18n ("Ian Wadham"), i18n ("Author"),
                             "iandw.au@gmail.com");
    Kdelibs4ConfigMigrator migrate(QStringLiteral("kubrick"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kubrickrc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("kubrickui.rc"));
    migrate.migrate();

    QCommandLineParser parser;
    KAboutData::setApplicationData(about);
    KCrash::initialize();
    parser.addVersionOption();
    parser.addHelpOption();
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    Kubrick * mainWindow = 0;

    if (app.isSessionRestored ()) {
        RESTORE (Kubrick);
    }
    else {
        // No interrupted session ... just start up normally.

        /// @todo do something with the command line args here

        mainWindow = new Kubrick ();
        mainWindow->show ();

        // if (args->count() > 0) {
            // args->clear();
        // }
    }

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kubrick")));

    // MainWindow has WDestructiveClose flag by default, so
    // it will delete itself.
    return app.exec ();
}
