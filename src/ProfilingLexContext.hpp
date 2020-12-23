#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_PROFILINGLEXCONTEXT_HPP
#define HTCW_PROFILINGLEXCONTEXT_HPP
#include "FileLexContext.hpp"
namespace lex {
#ifdef ARDUINO
    template<size_t TCapacity> class ProfilingArduinoLexContext : public ArduinoLexContext<TCapacity> {
        size_t m_max;
    public:
        ProfilingArduinoLexContext() {
            m_max = 0;
        }
        size_t used() {
            // count the trailing zero
            return m_max+1;
        }
        void resetUsed() {
            m_max = 0;
        }
        bool capture() override {
            if(ArduinoLexContext<TCapacity>::capture()) {
                if(this->captureCount()>m_max)
                    m_max = this->captureCount();
                return true;
            }
            return false;
        }
        bool capture(char ch) override {
            if(ArduinoLexContext<TCapacity>::capture(ch)) {
                if(this->captureCount()>m_max)
                    m_max = this->captureCount();
                return true;
            }
            return false;
        }
    };
#endif
    template<size_t TCapacity> class ProfilingStaticFileLexContext : public StaticFileLexContext<TCapacity> {
        size_t m_max;
    public:
        ProfilingStaticFileLexContext() {
            m_max = 0;
        }
        size_t used() {
            // count the trailing zero
            return m_max+1;
        }
        void resetUsed() {
            m_max = 0;
        }
        bool capture() override {
            if(StaticFileLexContext<TCapacity>::capture()) {
                if(this->captureCount()>m_max)
                    m_max = this->captureCount();
                return true;
            }
            return false;
        }
        bool capture(char ch) override {
            if(StaticFileLexContext<TCapacity>::capture(ch)) {
                if(this->captureCount()>m_max)
                    m_max = this->captureCount();
                return true;
            }
            return false;
        }
    };
}
#endif
