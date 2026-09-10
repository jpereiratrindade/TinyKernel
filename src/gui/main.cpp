#include "gui_bridge.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

int main(int argc, char **argv) {
  QGuiApplication application(argc, argv);
  QCoreApplication::setApplicationName("tinykernel-gui");
  QCoreApplication::setApplicationVersion(TINYKERNEL_VERSION);
  QCommandLineParser parser;
  parser.addHelpOption(); parser.addVersionOption();
  const QCommandLineOption workspace({"w", "workspace"}, "TinyKernel workspace", "path", ".tinykernel");
  const QCommandLineOption smoke("smoke", "Exit after the first frame for startup verification");
  const QCommandLineOption screenshot("screenshot", "Save a rendered window and exit", "path");
  const QCommandLineOption exercise("exercise", "Execute and export TK-0001 before opening");
  parser.addOption(workspace); parser.addOption(smoke); parser.addOption(screenshot); parser.addOption(exercise);
  parser.process(application);

  GuiBridge bridge(parser.value(workspace));
  if (parser.isSet(exercise)) {
    bridge.runInvestigation();
    bridge.exportInvestigation();
  }
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("bridge", &bridge);
  engine.loadFromModule("TinyKernel", "Main");
  if (engine.rootObjects().isEmpty()) return 1;
  if (parser.isSet(screenshot)) {
    const auto destination = parser.value(screenshot);
    QTimer::singleShot(700, &application, [&application, &engine, destination] {
      auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().front());
      application.exit(window && window->grabWindow().save(destination) ? 0 : 2);
    });
  } else if (parser.isSet(smoke)) {
    QTimer::singleShot(250, &application, &QCoreApplication::quit);
  }
  return application.exec();
}
