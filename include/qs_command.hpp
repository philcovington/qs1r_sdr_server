#pragma once

#include "../include/qs_defaults.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_globals.hpp"
#include <string>

class QS1RServer;

class CommandProcessor {
  public:
    CommandProcessor();
    ~CommandProcessor();

    void init(QS1RServer* server);
    void process();

  private:
    QS1RServer *m_p_server;
    bool m_is_init = false;
};