#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

class QsSquelch {
  private:
    // SQUELCH
    bool m_sq_switch;
    double m_sq_thresh;
    double m_sq_hist;

  public:
    QsSquelch();

    void init();
    void process(qs_vect_cpx &src_dst);
};