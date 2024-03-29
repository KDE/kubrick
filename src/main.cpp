/*
    SPDX-FileCopyrightText: 2008 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include "kubrick.h"
#include "kubrick_version.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kubrick"));

    KAboutData about (QStringLiteral("kubrick"), i18n ("Kubrick"),
            QStringLiteral(KUBRICK_VERSION_STRING),
            i18n ("A game based on Rubik's Cube (TM)"),
            KAboutLicense::GPL,
            i18n ("&copy; 2008 Ian Wadham"),
            QString(),
            QStringLiteral("https://apps.kde.org/kubrick") );
    about.addAuthor (i18n ("Ian Wadham"), i18n ("Author"),
                             QStringLiteral("iandw.au@gmail.com"));

    KAboutData::setApplicationData(about);
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("kubrick")));

    KCrash::initialize();

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    KDBusService service;

    Kubrick * mainWindow = nullptr;

    if (app.isSessionRestored ()) {
        kRestoreMainWindows<Kubrick>();
    }
    else {
        // No interrupted session ... just start up normally.

        /// @todo do something with the command line args here

        mainWindow = new Kubrick ();
        mainWindow->show ();
    }

    // MainWindow has WDestructiveClose flag by default, so
    // it will delete itself.
    return app.exec ();
}
