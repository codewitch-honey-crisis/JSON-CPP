#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_FILELEXCONTEXT_HPP
#define HTCW_FILELEXCONTEXT_HPP
#ifndef ARDUINO
#include <cstdint>
#endif
#include <stdio.h>
#include "LexContext.hpp"
namespace lex {
    // represents a LexContext over a file
    class FileLexContext : virtual public LexContext {
        FILE *m_pfile;
    protected:
        // reads from the underlying device
        int16_t read() override {
            if(nullptr==m_pfile)
                return LexContext::Closed;
            return fgetc(m_pfile);
        }
    public:
        FileLexContext() {

        }
        // attaches the lexcontext to an existing file pointer
        // file pointer must be opened for reading
        bool attach(FILE* pfile) {
            if(nullptr==pfile)
                return false;
            if(m_pfile)
                close();
            m_pfile=pfile;
            initCapture();
            reset();
            return true;
        }
        // opens a file for reading
        bool open(const char* pfn) {
            FILE* pfile=fopen(pfn,"r");
            if(nullptr==pfile)
                return false;
            if(m_pfile)
                close();
            m_pfile=pfile;
            initCapture();
            reset();
            return true;
        }
        // detaches from a file without closing it
        void detach() {
            m_pfile=NULL;
        }
        // closes the attached file
        void close() {
            if(m_pfile) {
                fclose(m_pfile);
                m_pfile=NULL;
            }
        }
    };
    template<const size_t TCapacity> class StaticFileLexContext :public FileLexContext, public StaticLexContext<TCapacity> {
        public:
    };
}
#endif
