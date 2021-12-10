/****************************************************************************
**
** Copyright (C) 2017-2020 Elros https://github.com/elros34
**               2020 Rinigus https://github.com/rinigus
**               2012 Digia Plc and/or its subsidiary(-ies).
**
** This file is part of Flatpak Runner.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of the copyright holder nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**
****************************************************************************/

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QStandardPaths>
#include <QDir>
#include <sailfishapp.h>

#include <iostream>

#include "qmlcompositor.h"
#include "runner.h"

QString getFreeWaylandSocket()
{
  QDir runtime(QStandardPaths::standardLocations(QStandardPaths::RuntimeLocation).first());
  QString relative_base = "../../display";
  // limit search for the limited number of displays
  for (int i=0; i < 1000; ++i)
    {
      QString wd = QString("%1/wayland-%2").arg(relative_base).arg(i);
      if (!runtime.exists(wd))
        return wd;
    }
  return QString();
}

int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    app->setApplicationName("waydroid-runner");
    app->setOrganizationName("waydroid-runner");
    app->setApplicationVersion(APP_VERSION);

    qmlRegisterUncreatableType<Runner>("org.sailfishosopen", 1, 0, "Runner", "Register for enum types");

    QQuickView *view = SailfishApp::createView();

    // compositor
    QString socket = getFreeWaylandSocket();
    if (socket.isEmpty())
      {
        std::cerr << "Cannot find empty socket\n";
        return -1;
      }
    else
      std::cerr << "Wayland socket: " << socket.toStdString() << "\n";
    QmlCompositor *compositor = new QmlCompositor(view, socket.toStdString().c_str());
    QObject::connect(view, SIGNAL(afterRendering()), compositor, SLOT(sendCallbacks()));
    view->rootContext()->setContextProperty("compositor", compositor);

    // runner
    Runner *runner = new Runner(socket, app);
    view->rootContext()->setContextProperty("runner", runner);

    // whether settings are allowed and misc variables
    view->rootContext()->setContextProperty("programVersion", APP_VERSION);

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->create();
    QObject *firstPage = view->rootObject()->findChild<QObject*>("mainPage");
    QObject::connect(compositor, SIGNAL(windowAdded(QVariant)), firstPage, SLOT(windowAdded(QVariant)));
    QObject::connect(compositor, SIGNAL(windowResized(QVariant)), firstPage, SLOT(windowResized(QVariant)));

    view->show();
    app->processEvents();

    int rcode = app->exec();

    // cleanup
    delete runner;
    delete compositor;

    return rcode;
}

