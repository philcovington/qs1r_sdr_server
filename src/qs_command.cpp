#include "../include/qs_command.hpp"
#include "../include/config.h"
#include "../include/qs_squelch.hpp"
#include "../include/qs_state.hpp"
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
                 double frequency = std::stod(param);
                 m_p_server->setRxFrequency(frequency);
                 std::cout << "Frequency set to " << static_cast<int>(frequency) << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid frequency parameter" << std::endl;
             }
         }},
        // Alias for set.freq
        {"set.f", [this, &commands](const std::string &param) { commands["set.freq"](param); }},
        {"get.freq",
         [this](const std::string &) {
             double frequency = QsGlobal::g_memory->getRxLOFrequency();
             std::cout << "Frequency set to " << static_cast<int>(frequency) << std::endl;
         }},
        // Alias for get.freq
        {"get.f", [this, &commands](const std::string &param) { commands["get.freq"](param); }},
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
        {"set.agct",
         [this](const std::string &param) {
             try {
                 int intVal = std::stoi(param);
                 QsGlobal::g_memory->setAgcThreshold(intVal);
                 std::cout << "set AGC threshold to " << intVal << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid AGC threshold parameter" << std::endl;
             }
         }},
        {"get.agct",
         [this](const std::string &) {
             int agcThresh = static_cast<int>(QsGlobal::g_memory->getAgcThreshold());
             std::cout << "AGC threshold " << agcThresh << std::endl;
         }},
        {"set.agcs",
         [this](const std::string &param) {
             try {
                 int intVal = std::stoi(param);
                 QsGlobal::g_memory->setAgcDecaySpeed(intVal);
                 std::cout << "set AGC decay speed to " << intVal << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid AGC decay speed parameter" << std::endl;
             }
         }},
        {"get.agcs",
         [this](const std::string &) {
             int agcThresh = static_cast<int>(QsGlobal::g_memory->getAgcDecaySpeed());
             std::cout << "AGC decay speed is " << agcThresh << std::endl;
         }},
        {"get.agcc",
         [this](const std::string &) {
             int agcGain = static_cast<int>(QsGlobal::g_memory->getAgcCurrentGain());
             std::cout << "AGC current gain is " << agcGain << std::endl;
         }},
        {"set.scan",
         [this](const std::string &param) {
             try {
                 int intVal = std::stoi(param);
                 if (intVal != 0 && intVal != 1) {
                     throw std::invalid_argument("Only 0 or 1 is allowed.");
                 }
                 bool on = static_cast<bool>(intVal);
                 if (on) {
                     std::cout << "Starting scanner..." << std::endl;
                     if (QsGlobal::g_scanner != nullptr) {
                         if (!QsGlobal::g_scanner->isRunning()) {
                             QsGlobal::g_scanner->start();
                         }
                     }
                 } else {
                     std::cout << "Stopping scanner..." << std::endl;
                     if (QsGlobal::g_scanner != nullptr) {
                         QsGlobal::g_scanner->stop();
                     }
                 }
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid parameter. Use 0 or 1." << std::endl;
             }
         }},
        {"scan",
         [this, &commands](const std::string &) {
             // Call "set.scan" with "1" as the parameter to start the scanner
             commands["set.scan"]("1");
         }},
        {"hold",
         [this, &commands](const std::string &) {
             // Call "set.scan" with "0" as the parameter to start the scanner
             commands["set.scan"]("0");
         }},
        {"set.speed",
         [this](const std::string &param) {
             try {
                 if (QsGlobal::g_scanner) {
                     int intVal = std::stoi(param);
                     QsGlobal::g_scanner->setScanSpeed(intVal);
                     std::cout << "Scan speed set to " << intVal << std::endl;
                 } else {
                     std::cout << "Scan speed is N/A" << std::endl;
                 }
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid scan speed parameter" << std::endl;
             }
         }},
        {"get.speed",
         [this](const std::string &) {
             if (QsGlobal::g_scanner) {
                 int speed = static_cast<int>(QsGlobal::g_scanner->getScanSpeed());
                 std::cout << "Scan speed is " << speed << std::endl;
             } else {
                 std::cout << "Scan speed is N/A" << std::endl;
             }
         }},
        {"set.demph",
         [this](const std::string &param) {
             try {
                 int intVal = std::stoi(param);
                 if (intVal != 0 && intVal != 1) {
                     throw std::invalid_argument("Only 0 or 1 is allowed.");
                 }
                 bool on = static_cast<bool>(intVal);
                 QsGlobal::g_memory->setDeEmphasisOn(on);
                 std::cout << "De-emphasis set to " << (on ? "on" : "off") << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid De-emphasis parameter. Use 0 or 1." << std::endl;
             }
         }},
        {"get.demph",
         [this](const std::string &param) {
             int on = QsGlobal::g_memory->getDeEmphasisOn();
             std::cout << "De-emphasis is " << (on ? "on" : "off") << std::endl;
         }},
        {"set.ampref",
         [this](const std::string &param) {
             try {
                 float alpha = std::stof(param);
                 QsGlobal::g_memory->setAMPostFilterAlpha(alpha);
                 std::cout << "AM post filter alpha set to " << alpha << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid AM post filter alpha parameter" << std::endl;
             }
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
        {"set.sq", [this, &commands](const std::string &param) { commands["set.squelch"](param); }},
        {"get.squelch",
         [this](const std::string &) {
             bool on = QsGlobal::g_memory->getSquelchOn();
             std::cout << "Squelch is " << (on ? "on" : "off") << std::endl;
         }},
        {"get.sq", [this, &commands](const std::string &param) { commands["get.squelch"](param); }},
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
        {"set.sqt", [this, &commands](const std::string &param) { commands["set.squelchthr"](param); }},
        {"get.squelchthr",
         [this](const std::string &) {
             double thresh = QsGlobal::g_memory->getSquelchThreshold();
             std::cout << "Squelch threshold is " << thresh << std::endl;
         }},
        {"get.sqt", [this, &commands](const std::string &param) { commands["get.squelchthr"](param); }},
        {"set.ctcsst",
         [this](const std::string &param) {
             try {
                 double threshold = std::stod(param);
                 QsGlobal::g_memory->setCTCSSThreshold(threshold);
                 std::cout << "CTCSS threshold set to " << threshold << std::endl;
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid CTCSS threshold parameter" << std::endl;
             }
         }},
        {"get.ctcsst",
         [this](const std::string &) {
             double thresh = QsGlobal::g_memory->getCTCSSThreshold();
             std::cout << "CTCSS threshold is " << thresh << std::endl;
         }},
        {"get.ctcssm",
         [this](const std::string &) {
             if (QsGlobal::g_dsp_proc != nullptr) {
                 double magnitude = QsGlobal::g_dsp_proc->p_sq->getCTCSSMagnitude();
                 std::cout << "CTCSS magnitude is " << magnitude << std::endl;
             } else {
                 std::cout << "CTCSS magnitude is N/A!" << std::endl;
             }
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
        {"set.v", [this, &commands](const std::string &param) { commands["set.volume"](param); }},
        {"get.volume",
         [this](const std::string &) {
             double volume = QsGlobal::g_memory->getVolume();
             std::cout << "Volume is " << volume << std::endl;
         }},
        {"get.v", [this, &commands](const std::string &param) { commands["get.volume"](param); }},
        {"set.ctcsstone",
         [this](const std::string &param) {
             try {
                 if (QsGlobal::g_dsp_proc != nullptr) {
                     double ctcss_tone = std::stod(param);
                     QsGlobal::g_dsp_proc->p_sq->setToneFrequency(ctcss_tone);
                     std::cout << "CTCSS tone set to " << ctcss_tone << std::endl;
                 } else {
                     std::cout << "DSP process not available!" << std::endl;
                 }
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid CTCSS tone parameter" << std::endl;
             }
         }},
        {"get.ctcsstone",
         [this](const std::string &) {
             if (QsGlobal::g_dsp_proc != nullptr) {
                 double ctcss_tone = QsGlobal::g_dsp_proc->p_sq->getToneFrequency();
                 std::cout << "CTCSS tone is " << ctcss_tone << std::endl;
             } else {
                 std::cout << "CTCSS tone is N/A!" << std::endl;
             }
         }},
        {"get.smeter",
         [this](const std::string &) {
             double smeter = QsGlobal::g_memory->getSMeterCurrentValue();
             std::cout << "Signal level is " << smeter << std::endl;
         }},
        {"get.status",
         [this](const std::string &) {
             // Retrieve values
             int frequency = static_cast<int>(QsGlobal::g_server->getRxFrequency());
             int smeter = static_cast<int>(QsGlobal::g_memory->getSMeterCurrentValue());
             int volume = static_cast<int>(QsGlobal::g_memory->getVolume());
             std::string mode = m_p_server->getRxMode().toStdString();
             int filterHi = static_cast<int>(QsGlobal::g_memory->getFilterHi());
             int filterLo = static_cast<int>(QsGlobal::g_memory->getFilterLo());
             bool squelchOn = static_cast<bool>(QsGlobal::g_memory->getSquelchOn());
             int sqThresh = static_cast<int>(QsGlobal::g_memory->getSquelchThreshold());
             int agcThresh = static_cast<int>(QsGlobal::g_memory->getAgcThreshold());
             int agcSpeed = static_cast<int>(QsGlobal::g_memory->getAgcDecaySpeed());

             // Format output
             std::ostringstream output;
             output << "F:[" << frequency << "] SM:[" << smeter << "] V:[" << volume << "] M:[" << mode << "] FHL:["
                    << filterHi << ", " << filterLo << "] SQ:[" << squelchOn << "] SQT:[" << sqThresh << "] AGCT:["
                    << agcThresh << "] AGCS:[" << agcSpeed << "]";

             // Print to console
             std::cout << output.str() << std::endl;
         }},
        {"get.stat", [this, &commands](const std::string &param) { commands["get.status"](param); }},
        {"?", [this, &commands](const std::string &param) { commands["get.status"](param); }},
        {"get.outdevices",
         [this](const std::string &) {
             freopen("/dev/null", "w", stderr);
             StringList device_list = QsGlobal::g_audio->getOutputDevices();
             freopen("/dev/tty", "w", stderr);
             for (int i = 0; i < device_list.size(); i++) {
                 std::cout << "Device " << i << ": " << device_list[i] << std::endl;
             }
         }},
        {"set.outdevice",
         [this](const std::string &param) {
             try {
                 int device_id = std::stod(param);

                 if (QsGlobal::g_audio->isStreamRunning()) {
                     QsGlobal::g_audio->stopStream();
                 }

                 int frames = QsGlobal::g_memory->getRtAudioFrames();
                 int rate = QsGlobal::g_memory->getRtAudioRate();
                 int out_dev_id = device_id;
                 int in_dev_id = -1;

                 bool ok = false;
                 freopen("/dev/null", "w", stderr);
                 QsGlobal::g_audio->initAudio(frames, rate, in_dev_id, out_dev_id, ok);
                 if (!QsGlobal::g_audio->isStreamRunning()) {
                     QsGlobal::g_audio->startStream();
                 }
                 std::cout << "Out device set to " << device_id << std::endl;
                 freopen("/dev/tty", "w", stderr);
             } catch (const std::invalid_argument &) {
                 std::cerr << "Invalid out device parameter" << std::endl;
                 freopen("/dev/tty", "w", stderr);
             }
         }},
        {"get.outdevice",
         [this](const std::string &) {
             int dev_id = QsGlobal::g_server->p_qsState->rtAudioOutDevId();
             std::cout << "Device ID is " << dev_id << std::endl;
         }},
        {"get.version", [this](const std::string &) { std::cout << "Version " << VERSION << std::endl; }},
        {"get.help",
         [&commands](const std::string &) {
             std::cout << "Available commands:\n";
             for (const auto &command : commands) {
                 std::cout << " - " << command.first << '\n';
             }
         }},
    };

    std::string line;
    while (true) {
        char *input = readline("?: "); // Use readline for input
        if (!input) {
            break; // Handle EOF or error
        }

        if (*input) {
            add_history(input); // Add to history if the input is not empty
        }

        line = input; // Assign the input to the line variable
        free(input);  // Free the allocated memory

        std::istringstream iss(line);
        std::string cmd, param;
        iss >> cmd;
        std::getline(iss, param);                     // Capture the rest of the line as the parameter
        param.erase(0, param.find_first_not_of(" ")); // Trim leading whitespace

        // exit (quit) program
        if (cmd == "exit" || cmd == "q")
            break;

        // Translation of "g." to "get." and "s." to "set."
        if (cmd.rfind("g.", 0) == 0) {
            cmd.replace(0, 2, "get.");
        } else if (cmd.rfind("s.", 0) == 0) {
            cmd.replace(0, 2, "set.");
        }

        auto it = commands.find(cmd);
        if (it != commands.end()) {
            try {
                it->second(param); // Pass the parameter to the command function
            } catch (const std::exception &e) {
                std::cerr << "Error executing command: " << e.what() << '\n';
            }
        } else {
            // By default just return the status
            int frequency = static_cast<int>(QsGlobal::g_server->getRxFrequency());
            int smeter = static_cast<int>(QsGlobal::g_memory->getSMeterCurrentValue());
            int volume = static_cast<int>(QsGlobal::g_memory->getVolume());
            std::string mode = m_p_server->getRxMode().toStdString();
            int filterHi = static_cast<int>(QsGlobal::g_memory->getFilterHi());
            int filterLo = static_cast<int>(QsGlobal::g_memory->getFilterLo());
            bool squelchOn = static_cast<bool>(QsGlobal::g_memory->getSquelchOn());
            int sqThresh = static_cast<int>(QsGlobal::g_memory->getSquelchThreshold());
            int agcThresh = static_cast<int>(QsGlobal::g_memory->getAgcThreshold());
            int agcSpeed = static_cast<int>(QsGlobal::g_memory->getAgcDecaySpeed());

            // Format output
            std::ostringstream output;
            output << "F:[" << frequency << "] SM:[" << smeter << "] V:[" << volume << "] M:[" << mode << "] FHL:["
                   << filterHi << ", " << filterLo << "] SQ:[" << squelchOn << "] SQT:[" << sqThresh << "] AGCT:["
                   << agcThresh << "] AGCS:[" << agcSpeed << "]";

            // Print to console
            std::cout << "Unknown command!" << std::endl;
            std::cout << output.str() << std::endl;
        }
    }
}