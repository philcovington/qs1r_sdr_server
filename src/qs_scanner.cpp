#include "../include/qs_scanner.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_globals.hpp"

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

    m_frequencies = {39640000, 39800000, 39880000};
    m_is_init = true;
}

void QsScanner::start() {
    if (!m_is_running && !m_thread_go) {
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

    while (m_thread_go) {
        if (!QsGlobal::g_server->isDspProcessorRunning()) {
            sleep.msleep(500);
        } else {
			int currentFreq = m_frequencies[freqIndex];
			QsGlobal::g_server->setRxFrequency(currentFreq);
			if (QsGlobal::g_memory->getSquelchOpened()) {
				// Squelch is open, so hold on the current frequency
				_debug() << "Current frequency: " << currentFreq;
				while (QsGlobal::g_memory->getSquelchOpened() && m_thread_go) {
					sleep.msleep(100); // Check every 100 ms while squelch is open
				}
				// After squelch closes, hold on the frequency for a specified delay
				sleep.msleep(m_holdTime);
				_debug() << "resuming...";
			} else {
				// If squelch is not open, move to the next frequency
				freqIndex = (freqIndex + 1) % m_frequencies.size();
			}
			sleep.msleep(m_settleTime);
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
