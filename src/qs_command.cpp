#include "../include/qs_command.hpp"
#include <stdexcept>

CommandProcessor::CommandProcessor() {}

CommandProcessor::~CommandProcessor() {}

void CommandProcessor::init(QS1RServer* server) {
    if (server == nullptr) {
        m_p_server = server;
        m_is_init = true;
    } else {
        m_is_init = false;
        throw std::runtime_error("In CommandProcessor::init qs1rServer is nullptr!");
    }
};

void CommandProcessor::process() {
    std::string cmd;
	if (!m_is_init) {
        throw std::runtime_error("In CommandProcessor::process init() was not called!");
    }
	while (cmd != "exit") {
        std::cout << "?:";
        std::cin >> cmd;
        std::cout << cmd << std::endl;
        if (cmd == "start") {
            m_p_server->startIo();
        } else if (cmd == "stop") {
            m_p_server->stopIo();
        }  
    }  
};