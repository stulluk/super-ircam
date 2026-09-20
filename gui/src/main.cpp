#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMetaType>

int main(int argc, char *argv[]) {
  qRegisterMetaType<QVector<float>>("QVector<float>");
  qRegisterMetaType<QImage>("QImage");
  qRegisterMetaType<ThermalStats>("ThermalStats");

  QApplication app(argc, argv);
  app.setApplicationName("super-ircam");
  app.setApplicationVersion("0.2.0");
  app.setDesktopFileName("super-ircam");

  QCommandLineParser parser;
  parser.setApplicationDescription("Qt viewer for Qianli Super IRCam / InfiRay USB cameras");
  parser.addHelpOption();
  QCommandLineOption shotOpt("screenshot", "Write a window PNG after the first frames.", "path");
  QCommandLineOption quitOpt("quit-after-shot", "Quit after --screenshot is written.");
  parser.addOption(shotOpt);
  parser.addOption(quitOpt);
  parser.process(app);

  MainWindow window;
  window.show();

  if (parser.isSet(shotOpt)) {
    const QString path = parser.value(shotOpt);
    const bool quitAfter = parser.isSet(quitOpt);
    QObject::connect(
        &window, &MainWindow::frameReceived, &app,
        [&window, path, quitAfter]() {
          static int frames = 0;
          static bool saved = false;
          ++frames;
          if (saved || frames < 8) {
            return;
          }
          saved = true;
          window.saveWindowShot(path);
          if (quitAfter) {
            QApplication::quit();
          }
        },
        Qt::QueuedConnection);
  }

  return app.exec();
}
