#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_JSONTREE_HPP
#define HTCW_JSONTREE_HPP
#define HTCW_JSONTREE
#include <cinttypes>
#include <cstddef>
#include <string.h>
#include <stdio.h>
// optimizations for Arduino
#include "ArduinoCommon.h"
// for memory management
#include "MemoryPool.hpp"
using namespace mem;
char JSON_LITERAL_NULL[] = PROGMEM "null";
#define JSON_LITERAL_NULL_LEN 4
char JSON_LITERAL_TRUE[] = PROGMEM "true";
#define JSON_LITERAL_TRUE_LEN 4
char JSON_LITERAL_FALSE[] = PROGMEM "false";
#define JSON_LITERAL_FALSE_LEN 5
char JSON_LITERAL_OBJECT[] = PROGMEM "{";
#define JSON_LITERAL_OBJECT_LEN 1
char JSON_LITERAL_ENDOBJECT[] = PROGMEM "}";
#define JSON_LITERAL_ENDOBJECT_LEN 1
char JSON_LITERAL_ARRAY[] = PROGMEM "[";
#define JSON_LITERAL_ARRAY_LEN 1
char JSON_LITERAL_ENDARRAY[] = PROGMEM "]";
#define JSON_LITERAL_ENDARRAY_LEN 1
char JSON_LITERAL_COMMA[] = PROGMEM ",";
#define JSON_LITERAL_COMMA_LEN 1
char JSON_LITERAL_COLON[] = PROGMEM ":";
#define JSON_LITERAL_COLON_LEN 1
char JSON_LITERAL_QUOTE[] = PROGMEM "\"";
#define JSON_LITERAL_QUOTE_LEN 1
char JSON_LITERAL_ESCAPE_CARRIAGERETURN[] = PROGMEM "\\r";
#define JSON_LITERAL_ESCAPE_CARRIAGERETURN_LEN 2
char JSON_LITERAL_ESCAPE_NEWLINE[] = PROGMEM "\\n";
#define JSON_LITERAL_ESCAPE_NEWLINE_LEN 2
char JSON_LITERAL_ESCAPE_FORMFEED[] = PROGMEM "\\f";
#define JSON_LITERAL_ESCAPE_FORMFEED_LEN 2
char JSON_LITERAL_ESCAPE_TAB[] = PROGMEM "\\t";
#define JSON_LITERAL_ESCAPE_TAB_LEN 2
char JSON_LITERAL_ESCAPE_BACKSPACE[] = PROGMEM "\\b";
#define JSON_LITERAL_ESCAPE_BACKSPACE_LEN 2
char JSON_LITERAL_ESCAPE_QUOTE[] = PROGMEM "\\\"";
#define JSON_LITERAL_ESCAPE_QUOTE_LEN 2
char JSON_LITERAL_ESCAPE_APOS[] = PROGMEM "\\\'";
#define JSON_LITERAL_ESCAPE_APOS_LEN 2
//char JSON_LITERAL_ESCAPE_SLASH[] = PROGMEM "\\/";
//#define JSON_LITERAL_ESCAPE_SLASH_LEN 2
char JSON_LITERAL_ESCAPE_BACKSLASH[] = PROGMEM "\\\\";
#define JSON_LITERAL_ESCAPE_BACKSLASH_LEN 4
char JSON_LITERAL_ESCAPE_UNICODE[] = PROGMEM "\\u";
#define JSON_LITERAL_ESCAPE_UNICODE_LEN 2

namespace json {
    class JsonElement;
    
    struct JsonFieldEntry {
        char *name;
        JsonElement* pvalue;
        JsonFieldEntry* pnext;
    };
    struct JsonArrayEntry {
        JsonElement* pvalue;
        JsonArrayEntry* pnext;
    };
    class JsonElement  {
    public:
        static const int8_t Undefined = -1;
        static const int8_t Null = 0;
        static const int8_t String = 1;
        static const int8_t Real = 2;
        static const int8_t Integer = 3;
        static const int8_t Boolean = 4;
        static const int8_t Field = 5;
        static const int8_t Array = 6;
        static const int8_t Object = 7;
    private:
        int8_t m_type;
        union {
            char* m_string;
            double m_real;
            long long m_integer;
            bool m_boolean;
            JsonArrayEntry* m_parray;
            JsonFieldEntry* m_pobject;
        };
        bool writeToString(MemoryPool& pool) const {
            size_t c;
            char ch;
            char *sz;
            char *sznew;
            char szn[32];
            switch(m_type) {
                case Null:
                    sznew=(char*)pool.alloc(JSON_LITERAL_NULL_LEN);
                    STRNCPYP(sznew,JSON_LITERAL_NULL,JSON_LITERAL_NULL_LEN);
                    return true;
                case Real:
#ifdef ARDUINO
                    dtostrf(real(), 31,6,szn);
#else
                    sprintf(szn,"%.6lf",real());
#endif
                    c=strlen(szn);
                    sznew=(char*)pool.alloc(c);
                    if(nullptr==sznew)
                        return false;
                    strncpy(sznew,szn,c);
                    return true;
                case Integer:
                    sprintf(szn,"%lld",integer());
                    c=strlen(szn);
                    sznew=(char*)pool.alloc(c);
                    if(nullptr==sznew)
                        return false;
                    strncpy(sznew,szn,c);
                    return true;
                case Boolean:
                    if(boolean()) {
                        sznew=(char*)pool.alloc(JSON_LITERAL_TRUE_LEN);
                        if(nullptr==sznew)
                            return false;
                        STRNCPYP(sznew,JSON_LITERAL_TRUE,JSON_LITERAL_TRUE_LEN);
                    } else {
                        sznew=(char*)pool.alloc(JSON_LITERAL_FALSE_LEN);
                        if(nullptr==sznew)
                            return false;
                        STRNCPYP(sznew,JSON_LITERAL_FALSE,JSON_LITERAL_FALSE_LEN);
                    }
                    return true;
                case String:
                    c=strlen(m_string);
                    sz = m_string;
                    sznew=(char*)pool.alloc(1);
                    if(nullptr==sznew)
                        return false;
                    *(sznew++)='\"';
                    while(ch=*sz) {
                        switch(ch) {
                            case '\r':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_CARRIAGERETURN_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_CARRIAGERETURN,JSON_LITERAL_ESCAPE_CARRIAGERETURN_LEN);
                                break;
                            case '\n':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_NEWLINE_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_NEWLINE,JSON_LITERAL_ESCAPE_NEWLINE_LEN);
                                break;
                            case '\t':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_TAB_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_TAB,JSON_LITERAL_ESCAPE_TAB_LEN);
                                break;
                            case '\\':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_BACKSLASH_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_BACKSLASH,JSON_LITERAL_ESCAPE_BACKSLASH_LEN);
                                break;
                            /*case '//':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_SLASH_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_BACKSLASH,JSON_LITERAL_ESCAPE_SLASH_LEN);
                                break;
                            */
                           case '\"':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_QUOTE_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_QUOTE,JSON_LITERAL_ESCAPE_QUOTE_LEN);
                                break;
                           case '\'':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_APOS_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_QUOTE,JSON_LITERAL_ESCAPE_APOS_LEN);
                                break;
                           case '\f':
                                sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_FORMFEED_LEN);
                                if(nullptr==sznew)
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_ESCAPE_QUOTE,JSON_LITERAL_ESCAPE_FORMFEED_LEN);
                                break;
                            default:
                                if(ch<32||ch>127) {
                                    sznew=(char*)pool.alloc(JSON_LITERAL_ESCAPE_UNICODE_LEN+4);
                                    if(nullptr==sznew)
                                        return false;
                                    STRNCPYP(sznew,JSON_LITERAL_ESCAPE_UNICODE,JSON_LITERAL_ESCAPE_UNICODE_LEN);
                                    sprintf(szn,"%04x",(unsigned int)ch);
                                    strncpy(sznew+JSON_LITERAL_ESCAPE_UNICODE_LEN,szn,4);
                                    break;
                                } else {
                                    sznew=(char*)pool.alloc(1);
                                    if(nullptr==sznew)
                                        return false;
                                    *sznew=ch;
                                }

                        }
                        ++sz;
                    }
                    sznew=(char*)pool.alloc(1);
                    if(nullptr==sznew)
                        return false;
                    *sznew='\"';
                    return true;
                case Array:
                    sznew=(char*)pool.alloc(JSON_LITERAL_ARRAY_LEN);
                    if(nullptr==sznew)
                        return false;
                    STRNCPYP(sznew,JSON_LITERAL_ARRAY,JSON_LITERAL_ARRAY_LEN);
                    if(m_parray) {
                        JsonArrayEntry* pcurrent = m_parray;
                        bool first = true;
                        while(pcurrent) {
                            if(first)
                                first = false;
                            else {
                                sznew=(char*)pool.alloc(JSON_LITERAL_COMMA_LEN);
                                if(nullptr==sznew) 
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_COMMA,JSON_LITERAL_COMMA_LEN);
                            }
                            if(!pcurrent->pvalue->writeToString(pool))
                                return false;
                            pcurrent=pcurrent->pnext;
                        }
                    }
                    sznew=(char*)pool.alloc(JSON_LITERAL_ENDARRAY_LEN);
                    if(nullptr==sznew)
                        return false;
                    STRNCPYP(sznew,JSON_LITERAL_ENDARRAY,JSON_LITERAL_ENDARRAY_LEN);
                    return true;
                case Object:
                    sznew=(char*)pool.alloc(JSON_LITERAL_OBJECT_LEN);
                    if(nullptr==sznew)
                        return false;
                    STRNCPYP(sznew,JSON_LITERAL_OBJECT,JSON_LITERAL_OBJECT_LEN);
                    if(m_pobject) {
                        JsonFieldEntry* pcurrent = m_pobject;
                        bool first = true;
                        while(pcurrent) {
                            if(first)
                                first = false;
                            else {
                                sznew=(char*)pool.alloc(JSON_LITERAL_COMMA_LEN);
                                if(nullptr==sznew) 
                                    return false;
                                STRNCPYP(sznew,JSON_LITERAL_COMMA,JSON_LITERAL_COMMA_LEN);
                            }
                            JsonElement t;
                            t.string(pcurrent->name);
                            if(!t.writeToString(pool)) 
                                return false;
                            sznew=(char*)pool.alloc(JSON_LITERAL_COLON_LEN);
                            if(nullptr==sznew) 
                                return false;
                            STRNCPYP(sznew,JSON_LITERAL_COLON,JSON_LITERAL_COLON_LEN);
                            if(!pcurrent->pvalue->writeToString(pool))
                                return false;
                            pcurrent=pcurrent->pnext;
                        }
                    }
                    sznew=(char*)pool.alloc(JSON_LITERAL_ENDOBJECT_LEN);
                    if(nullptr==sznew)
                        return false;
                    STRNCPYP(sznew,JSON_LITERAL_ENDOBJECT,JSON_LITERAL_ENDOBJECT_LEN);
                    return true;
            }
            return false;
        }
    public:

        JsonElement() : m_type(Undefined) {
        }
        JsonElement(nullptr_t value) {
            null(nullptr);
        }
        JsonElement(char* value) {
            string(value);
        }
        JsonElement(double value) {
            real(value);
        }
        JsonElement(long long value) {
            integer(value);
        }
        JsonElement(bool value) {
            boolean(value);
        }
        JsonElement(const JsonElement& rhs) = default;
        JsonElement& operator=(JsonElement&& rhs) = default;
        JsonElement& operator=(const JsonElement& rhs) = default;
        ~JsonElement()=default;
        const JsonElement operator[](const int index) {
            // only works for arrays
            if(0>index || m_type!=Array || !m_parray)
                return JsonElement();
            int i =  index;
            JsonArrayEntry* pcurrent=m_parray;
            while(i && pcurrent) {
                pcurrent=pcurrent->pnext;
                --i;
            }
            if(!pcurrent) return JsonElement();// index out of range
            return *pcurrent->pvalue;
        }
        const JsonElement operator[](const char* name) {
            // only works for objects
            if(!name || m_type!=Object || !m_pobject)
                return JsonElement();
            JsonFieldEntry* pcurrent=m_pobject;
            while(pcurrent) {
                if(!strcmp(pcurrent->name,name))
                    return *pcurrent->pvalue;
                pcurrent=pcurrent->pnext;
            }
            return JsonElement();// not found
        }
        int8_t type() const { return m_type; }
        nullptr_t null() const { return nullptr; } 
        void null(nullptr_t value) { m_type = Null;}
        double real() const { return m_real;}
        void real(double value) { m_type=Real; m_real = value; }
        long long integer() const {return m_integer;}
        void integer(long long value) { m_type=Integer; m_integer = value; }
        bool boolean() const {return m_boolean;}
        void boolean(bool value) { m_type=Boolean; m_boolean = value; }
        char* string() const {return m_string; }
        char* string(char* value) { m_type = String; m_string=value;}
        JsonFieldEntry* pobject() { return m_pobject;}
        void pobject(nullptr_t dummy) {
            m_type=Object;
            m_pobject = nullptr;
        }
        JsonArrayEntry* parray() { return m_parray;}
        void parray(nullptr_t dummy) {
            m_type=Array;
            m_parray = nullptr;
        }
        bool undefined() const {return m_type==Undefined;}
        char* toString(MemoryPool &pool) const {
            char* result = (char*)pool.next();
            if(result==nullptr) return nullptr;
            if(writeToString(pool)) {
                // terminate the string
                char* pz = (char*)pool.alloc(1);
                if(pz==nullptr)
                    return nullptr;
                *pz=0;
                return result;
            }
            return nullptr;

        }
        bool allocString(MemoryPool &pool, const char* sz) {
            if(!sz)
                return false;
            size_t c = strlen(sz)+1;
            char *sznew = (char*)pool.alloc(c);
            if(nullptr==sznew)
                return false;
            m_type = String;
            strcpy(sznew,sz);
            m_string=sznew;
            return true;
        }
        bool addFieldPooled(MemoryPool& pool, char* name,JsonElement* pvalue) {
            if(m_type!=Object)
                return false;
            JsonFieldEntry *pfe = m_pobject;
            if(pfe) {
                while(pfe->pnext) 
                    pfe=pfe->pnext;
                pfe->pnext = (JsonFieldEntry*)pool.alloc(sizeof(JsonFieldEntry));
                if(!pfe->pnext)
                    return false;
                pfe=pfe->pnext;
            } else {
                pfe=(JsonFieldEntry*)pool.alloc(sizeof(JsonFieldEntry));
                if(!pfe)
                    return false;
                m_pobject=pfe;
            }
            pfe->name=name;
            pfe->pvalue = pvalue;
            pfe->pnext = nullptr;
            return true;
        }
        bool addField(MemoryPool& pool,const char* name,JsonElement* pvalue) {
            // only works for objects
            // call setObject() first
            if(m_type!=Object)
                return false;
            JsonFieldEntry *pfe = m_pobject;
            if(pfe) {
                while(pfe->pnext) 
                    pfe=pfe->pnext;
                pfe->pnext = (JsonFieldEntry*)pool.alloc(sizeof(JsonFieldEntry));
                if(!pfe->pnext)
                    return false;
                pfe=pfe->pnext;
            } else {
                pfe=(JsonFieldEntry*)pool.alloc(sizeof(JsonFieldEntry));
                if(!pfe)
                    return false;
                m_pobject=pfe;
            }
            size_t c = strlen(name)+1;
            char* pname = (char*)pool.alloc(c);
            if(!pname)
                return false;
            strcpy(pname,name);
            pfe->name=pname;
            pfe->pvalue = pvalue;
            pfe->pnext = nullptr;
            return true;
        }
        bool addField(MemoryPool& pool, const char* name,const JsonElement& value) {
            // only works for objects
            // call setObject() first
            if(m_type!=Object)
                return false;
            JsonElement *pe = (JsonElement*)pool.alloc(sizeof(JsonElement));
            if(!pe)
                return false;
            *pe=value;
            return addField(pool,name,pe);
        }
        bool addField(MemoryPool& pool,const char *name, const nullptr_t value) {
            // only works for objects
            // call setObject() first
            JsonElement e;
            e.null(nullptr);
            return addField(pool,name,e);
        }
        bool addField(MemoryPool& pool,const char*name, const char* value) {
            // only works for objects
            // call setObject() first
            JsonElement val;
            if(!val.allocString(pool,value))
                return false;
            return addField(pool,name,val);
        }
        bool addField(MemoryPool& pool,const char *name, const double value) {
            // only works for objects
            // call setObject() first
            JsonElement e;
            e.real(value);
;            return addField(pool,name,e);
        }
        bool addField(MemoryPool& pool,const char *name, const long long value) {
            // only works for objects
            // call setObject() first
            JsonElement e;
            e.integer(value);
            return addField(pool,name,e);
        }
        bool addField(MemoryPool& pool,const char*name, const bool value) {
            // only works for objects
            // call setObject() first
            JsonElement e;
            e.boolean(value);
            return addField(pool,name,e);
        }
        bool addItem(MemoryPool& pool,JsonElement* pvalue) {
            // only works for arrays
            // call setArray() first
            if(m_type!=Array)
                return false;
            JsonArrayEntry *pae = m_parray;
            if(pae) {
                while(pae->pnext) 
                    pae=pae->pnext;
                pae->pnext = (JsonArrayEntry*)pool.alloc(sizeof(JsonArrayEntry));
                if(!pae->pnext)
                    return false;
                pae=pae->pnext;
            } else {
                pae=(JsonArrayEntry*)pool.alloc(sizeof(JsonArrayEntry));
                if(!pae)
                    return false;
                m_parray=pae;
            }
            pae->pvalue = pvalue;
            pae->pnext = nullptr;
            return true;
        }
        bool addItem(MemoryPool& pool, const JsonElement& value) {
            // only works for arrays
            // call setArray() first
            if(m_type!=Array)
                return false;
            JsonElement *pe = (JsonElement*)pool.alloc(sizeof(JsonElement));
            if(!pe)
                return false;
            *pe=value;
            return addItem(pool,pe);
        }
        bool addItem(MemoryPool& pool, const nullptr_t value) {
            // only works for arrays
            // call setArray() first
            JsonElement e;
            e.null(nullptr);
            return addItem(pool,e);
        }
        bool addItem(MemoryPool& pool, const char* value) {
            // only works for arrays
            // call setArray() first
            JsonElement e;
            if(!e.allocString(pool,value))
                return false;
            return addItem(pool,e);
        }
        bool addItemPooled(MemoryPool& pool, char* value) {
            // only works for arrays
            // call setArray() first
            JsonElement e;
            e.string(value);
            return addItem(pool,e);
        }
        
        bool addItem(MemoryPool& pool, const double value) {
            // only works for arrays
            // call setArray() first
            JsonElement e;
            e.real(value);
            return addItem(pool,e);
        }
        bool addItem(MemoryPool& pool, const long long value) {
            // only works for arrays
            // call setArray() first
            JsonElement e;
            e.integer(value);
            return addItem(pool,e);
        }
        bool addItem(MemoryPool& pool, const bool value) {
            // only works for arrays
            // call setArray() first
            JsonElement e;
            e.boolean(value);
            return addItem(pool,e);
        }
        
    };
    
}
#endif