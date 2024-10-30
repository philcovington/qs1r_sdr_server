#include "../include/qs_command.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

CommandProcessor::CommandProcessor() {}

CommandProcessor::~CommandProcessor() {}

void CommandProcessor::init(QS1RServer *server) {
    if (server != nullptr) {
        m_p_server = server;
        m_is_init = true;
    } else {
        m_is_init = false;
        throw std::runtime_error("In CommandProcessor::init qs1rServer is nullptr!");
    }
};

void CommandProcessor::process() {
    if (!m_is_init) {
        throw std::runtime_error("In CommandProcessor::process, init() was not called!");
    }

    std::unordered_map<std::string, std::function<void(const std::string &)>> commands = {
        {"start", [this](const std::string &) { m_p_server->startIo(); }},
        {"stop", [this](const std::string &) { m_p_server->stopIo(); }},
        {"set.freq",
         [this](const std::string &param) {
             try {
                 int frequency = std::stoi(param);
                 m_p_server->setRxFrequency(frequency);
                 std::cout << "Frequency set to " << frequency << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid frequency parameter" << std::endl;
             }
         }},
        {"get.freq",
         [this](const std::string &) {
             double frequency = QsGlobal::g_memory->getRxLOFrequency();
             std::cout << "Frequency set to " << frequency << std::endl;
         }},
        {"set.mode",
         [this](const std::string &mode) {
             if (mode.empty()) {
                 std::cerr << "Mode parameter is missing" << std::endl;
                 return;
             }
             m_p_server->setRxMode(mode);
             std::cout << "Mode set to " << mode << std::endl;
         }},
        {"get.mode",
         [this](const std::string &) {
             std::string mode = m_p_server->getRxMode().toStdString();
             std::cout << "Mode set to " << mode << std::endl;
         }},
        {"set.squelch",
         [this](const std::string &param) {
             try {
                 int intVal = std::stoi(param);
                 if (intVal != 0 && intVal != 1) {
                     throw std::invalid_argument("Only 0 or 1 is allowed.");
                 }
                 bool on = static_cast<bool>(intVal);
                 m_p_server->setSquelchOn(on);
                 std::cout << "Squelch set " << (on ? "on" : "off") << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid squelch parameter. Use 0 or 1." << std::endl;
             }
         }},
        {"get.squelch",
         [this](const std::string &) {
             bool on = QsGlobal::g_memory->getSquelchOn();
             std::cout << "Squelch is " << (on ? "on" : "off") << std::endl;
         }},
        {"set.squelchthr",
         [this](const std::string &param) {
             try {
                 double threshold = std::stod(param);
                 m_p_server->setSquelchThreshold(threshold);
                 std::cout << "Squelch threshold set to " << threshold << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid squelch threshold parameter" << std::endl;
             }
         }},
        {"get.squelchthr",
         [this](const std::string &) {
             double thresh = QsGlobal::g_memory->getSquelchThreshold();
             std::cout << "Squelch threshold is " << thresh << std::endl;
         }},
        {"set.filter",
         [this](const std::string &param) {
             try {
                 double value = std::stod(param);
                 m_p_server->setFilter(value);
                 std::cout << "Filter set to " << value << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid filter parameter." << std::endl;
             }
         }},
        {"get.filter",
         [this](const std::string &) {
             double valuehi = QsGlobal::g_memory->getFilterHi();
             double valuelo = QsGlobal::g_memory->getFilterLo();
             std::cout << "Filter Hi: " << valuehi << ", Filter Lo: " << valuelo << std::endl;
         }},
        {"set.volume",
         [this](const std::string &param) {
             try {
                 double volume = std::stod(param);
                 m_p_server->setVolume(volume);
                 std::cout << "Volume set to " << volume << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid volume parameter" << std::endl;
             }
         }},
        {"get.volume",
         [this](const std::string &) {
             double volume = QsGlobal::g_memory->getVolume();
             std::cout << "Volume is " << volume << std::endl;
         }},
		{"get.smeter",
         [this](const std::string &) {
             double smeter = QsGlobal::g_memory->getSMeterCurrentValue();
             std::cout << "Signal level is " << smeter << std::endl;
         }},
    };

    std::string line;
    while (true) {
        std::cout << "?: ";
        std::getline(std::cin, line);

        std::istringstream iss(line);
        std::string cmd, param;
        iss >> cmd;
        std::getline(iss, param);                     // Capture the rest of the line as the parameter
        param.erase(0, param.find_first_not_of(" ")); // Trim leading whitespace

        if (cmd == "exit")
            break;

        auto it = commands.find(cmd);
        if (it != commands.end()) {
            try {
                it->second(param); // Pass the parameter to the command function
            } catch (const std::exception &e) {
                std::cerr << "Error executing command: " << e.what() << '\n';
            }
        } else {
            std::cout << "Unknown command: " << cmd << '\n';
        }
    }
}