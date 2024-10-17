#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

using namespace std;

class QsNoiseReductionFilter {
  private:
    bool m_nr_switch;
    int m_nr_lms_sz;
    int m_nr_delay;
    int m_nr_dl_indx;
    int m_nr_mask;
    double m_nr_adapt_rate;
    double m_nr_leakage;
    double m_nr_adapt_size;

    qs_vect_f m_nr_delay_line;
    qs_vect_f m_nr_coeff;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsNoiseReductionFilter();

    void init(unsigned int size);
    void process(qs_vect_cpx &src_dst);
};
