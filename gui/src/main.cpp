#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QMetaType>
#include <QTimer>

#include <algorithm>

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
  QCommandLineOption saveOpt("save-settings", "Write current UI settings to ~/.config/super-ircam/settings.json.");
  QCommandLineOption recOpt("record-seconds", "Record AVI for N seconds after warmup, then stop.", "seconds");
  parser.addOption(shotOpt);
  parser.addOption(quitOpt);
  parser.addOption(cmapOpt);
  parser.addOption(layoutOpt);
  parser.addOption(saveOpt);
  parser.addOption(recOpt);
  parser.process(app);

  MainWindow window;
  if (parser.isSet(cmapOpt)) {
    const QString want = parser.value(cmapOpt);
    const QStringList names = cmapNames();
    int idx = names.indexOf(want);
    if (idx < 0) {
      for (int i = 0; i < names.size(); ++i) {
        if (names[i].compare(want, Qt::CaseInsensitive) == 0) {
          idx = i;
          break;
        }
      }
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
  if (parser.isSet(saveOpt)) {
    window.saveSettings();
    if (!parser.isSet(shotOpt) && !parser.isSet(recOpt)) {
      return 0;
    }
  }

  if (parser.isSet(recOpt)) {
    const int seconds = std::max(1, parser.value(recOpt).toInt());
    QObject::connect(
        &window, &MainWindow::frameReceived, &app,
        [&window, seconds]() {
          static int frames = 0;
          static bool started = false;
          static bool stopped = false;
          ++frames;
          if (!started && frames >= 40) {
            started = true;
            window.onRecordToggle();
          }
          if (started && !stopped && frames >= 40 + seconds * 25) {
            stopped = true;
            window.onRecordToggle();
            QApplication::quit();
          }
        },
        Qt::QueuedConnection);
  }

  if (parser.isSet(shotOpt)) {
    const QString path = parser.value(shotOpt);
    const bool quitAfter = parser.isSet(quitOpt);
    auto *shotSaved = new bool(false);
    QObject::connect(
        &window, &MainWindow::frameReceived, &app,
        [&window, path, quitAfter, shotSaved]() {
          static int frames = 0;
          ++frames;
          if (*shotSaved || frames < 40) {
            return;
          }
          *shotSaved = true;
          window.saveWindowShot(path);
          if (quitAfter) {
            QApplication::quit();
          }
        },
        Qt::QueuedConnection);
    if (quitAfter) {
      QTimer::singleShot(4000, &app, [&window, path, shotSaved]() {
        if (*shotSaved) {
          return;
        }
        *shotSaved = true;
        window.saveWindowShot(path);
        QApplication::quit();
      });
    }
  }

  return app.exec();
}
