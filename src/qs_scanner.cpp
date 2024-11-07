#include "../include/qs_scanner.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_globals.hpp"
#include "../include/json.hpp"
#include <fstream>
#include <iostream>

using nlohmann::json;

// Constructor and destructor remain the same
QsScanner::QsScanner() : m_thread_go(false), m_is_running(false) {}

QsScanner::~QsScanner() {
    stop(); // Ensure the thread is stopped before destruction
    if (m_thread.joinable()) {
        m_thread.join(); // Wait for the thread to finish
    }
}

void QsScanner::reinit() { init(); }

void QsScanner::init() {
    QsGlobal::g_memory->setSquelchOn(true);
    QsGlobal::g_memory->setSquelchThreshold(-70.0);
    QsGlobal::g_memory->setDemodMode(QSDEMODMODE::dmFMN);
    QsGlobal::g_memory->setDeEmphasisOn(true);
    QsGlobal::g_server->setFilter(10000);

	std::ifstream file("qs1r_scanlist.json"); // Open the JSON file
    if (!file.is_open()) {
        _debug() << "Could not open qs1r_scanlist.json! Scanning disabled.";
		m_is_init = false;
		return;
    }

    json jsonData;
    file >> jsonData; // Parse JSON data
    file.close();

    // Clear m_frequencies and populate it from JSON data
    m_frequencies.clear();
    for (auto& [freqStr, channelName] : jsonData.items()) {
        int frequency = std::stoi(freqStr); // Convert key to int
        m_frequencies[frequency] = channelName;
    }
    m_is_init = true;
}

void QsScanner::start() {
    if (!m_is_running && !m_thread_go && m_is_init) {
        m_thread_go = true;
        m_thread = std::thread(&QsScanner::run, this); // Launch the run() method in a new thread
    }
}

void QsScanner::run() {
    if (!m_is_init) {
        throw std::runtime_error("QsScanner::run must call init() first!");
    }
    m_is_running = true;

    size_t freqIndex = 0;

    _debug() << "Scanning...";

    auto freqIt = m_frequencies.begin(); // Iterator to current frequency
    while (m_thread_go) {
        if (!QsGlobal::g_server->isDspProcessorRunning()) {
            sleep.msleep(500);
        } else {
            // Set current frequency and get channel name
            int currentFreq = freqIt->first;
            std::string channelName = freqIt->second;

            QsGlobal::g_server->setRxFrequency(currentFreq);
            // std::cout << "Scanning: " << currentFreq << std::endl;
			sleep.msleep(m_settleTime);
            if (QsGlobal::g_memory->getSquelchOpened()) {
                // Squelch is open, so hold on the current frequency
                _debug() << "Current frequency: " << currentFreq << " (" << channelName << ") [" << QsGlobal::g_memory->getSMeterCurrentValue() << "]";
                while (QsGlobal::g_memory->getSquelchOpened() && m_thread_go) {
                    sleep.msleep(200); // Check every 100 ms while squelch is open
                }
                // After squelch closes, hold on the frequency for a specified delay
                sleep.msleep(m_holdTime);
                _debug() << "Resuming scan...";
            } else {
                // If squelch is not open, move to the next frequency
                ++freqIt;
                if (freqIt == m_frequencies.end()) {
                    freqIt = m_frequencies.begin(); // Wrap around to the start
                }
            }            
        }
    }

    m_is_running = false;
    _debug() << "QsScanner thread stopped.";
}

void QsScanner::stop() {
    m_thread_go = false; // Signal the thread to stop
    if (m_thread.joinable()) {
        m_thread.join(); // Wait for the thread to finish
    }
}

bool QsScanner::isRunning() { return m_thread_go; }
