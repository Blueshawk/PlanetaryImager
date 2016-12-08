/*
 * Copyright (C) 2016  Marco Gulino <marco@gulinux.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <QApplication>
#include "planetaryimager_mainwindow.h"
#include "commons/version.h"
#include <iostream>
#include <QDebug>
#include "network/client/remotedriver.h"
#include "network/client/networkclient.h"
#include "network/networkdispatcher.h"
#include "commons/loghandler.h"
#include "commons/crashhandler.h"
#include <QMenuBar>
#include <QMenu>

#include "network/client/remotesaveimages.h"
using namespace std;


int main(int argc, char** argv)
{
    qRegisterMetaType<Frame::ptr>("Frame::ptr");
    CrashHandler crash_handler({SIGSEGV, SIGABRT});
    cerr << "Starting PlanetaryImager - version " << PLANETARY_IMAGER_VERSION << " (" << HOST_PROCESSOR << ")" << endl;
    QApplication app(argc, argv);
    LogHandler log_handler;
    app.setApplicationName("PlanetaryImager");
    app.setApplicationDisplayName("Planetary Imager");
    app.setApplicationVersion(PLANETARY_IMAGER_VERSION);
    auto dispatcher = make_shared<NetworkDispatcher>();
    auto networkClient = make_shared<NetworkClient>(dispatcher);
    auto remoteDriver = make_shared<RemoteDriver>(dispatcher);
    QMetaObject::invokeMethod(networkClient.get(), "connectToHost", Q_ARG(QString, "localhost"), Q_ARG(int, 9999));
    auto configuration = make_shared<Configuration>();
    QObject::connect(networkClient.get(), &NetworkClient::connected, networkClient.get(), [=, &configuration]{
      auto mainWindow = new PlanetaryImagerMainWindow{remoteDriver, make_shared<RemoteSaveImages>(dispatcher), configuration};
      mainWindow->show();
      if(auto running_camera = remoteDriver->existing_running_camera()) {
        mainWindow->connectCamera(running_camera);
      }
    });
    return app.exec();
}
