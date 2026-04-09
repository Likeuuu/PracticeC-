#include "mnf/app/cli.h"

int main(int argc, char** argv) {
  mnf::CliApp app;
  return app.Run(argc, argv);
}
