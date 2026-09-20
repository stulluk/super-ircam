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
  QCommandLineOption cmapOpt("cmap", "Colormap name (Jet, Inferno, Ironbow, ...).", "name");
  QCommandLineOption layoutOpt("layout", "Layout: thermal, image, wide, or high.", "name");
  parser.addOption(shotOpt);
  parser.addOption(quitOpt);
  parser.addOption(cmapOpt);
  parser.addOption(layoutOpt);
  parser.process(app);

  MainWindow window;
  if (parser.isSet(cmapOpt)) {
    const QString want = parser.value(cmapOpt);
    const QStringList names = cmapNames();
    int idx = names.indexOf(want);
    if (idx < 0) {
      idx = names.indexOf(want, 0, Qt::CaseInsensitive);
    }
    if (idx >= 0) {
      window.setCmapIndex(idx);
    }
  }
  if (parser.isSet(layoutOpt)) {
    const QString lay = parser.value(layoutOpt).toLower();
    int idx = 0;
    if (lay == "image") {
      idx = 1;
    } else if (lay == "wide") {
      idx = 2;
    } else if (lay == "high") {
      idx = 3;
    }
    window.setLayoutIndex(idx);
  }
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
          if (saved || frames < 40) {
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
