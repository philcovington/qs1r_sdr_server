#include "../include/qs_volume.hpp"

QsVolume ::QsVolume() : m_volume_db(0), m_volume_val(0) {}

void QsVolume ::process(qs_vect_cpx &src_dst) {
    m_volume_db = QsGlobal::g_memory->getVolume();
    m_volume_val = pow(10.0, m_volume_db / 20.0);
    for (cpx_itr = src_dst.begin(); cpx_itr != src_dst.end(); cpx_itr++) {
        (*cpx_itr) *= m_volume_val;
    }
}

void QsVolume ::process(qs_vect_f &src_dst) {
    m_volume_db = QsGlobal::g_memory->getVolume();
    m_volume_val = pow(10.0, m_volume_db / 20.0);
    for (f_itr = src_dst.begin(); f_itr != src_dst.end(); f_itr++) {
        (*f_itr) *= m_volume_val;
    }
}
