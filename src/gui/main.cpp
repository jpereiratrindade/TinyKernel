#include "gui_bridge.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

int main(int argc, char **argv) {
  QGuiApplication application(argc, argv);
  QCoreApplication::setApplicationName("tinykernel-gui");
  QCoreApplication::setApplicationVersion(TINYKERNEL_VERSION);
  QCommandLineParser parser;
  parser.addHelpOption(); parser.addVersionOption();
  const QCommandLineOption workspace({"w", "workspace"}, "TinyKernel workspace", "path", ".tinykernel");
  const QCommandLineOption smoke("smoke", "Exit after the first frame for startup verification");
  parser.addOption(workspace); parser.addOption(smoke); parser.process(application);

  GuiBridge bridge(parser.value(workspace));
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("bridge", &bridge);
  engine.loadFromModule("TinyKernel", "Main");
  if (engine.rootObjects().isEmpty()) return 1;
  if (parser.isSet(smoke)) QTimer::singleShot(250, &application, &QCoreApplication::quit);
  return application.exec();
}
