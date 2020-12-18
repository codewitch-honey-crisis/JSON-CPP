#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_JSONREADER_HPP
#define HTCW_JSONREADER_HPP
#include <string.h> 
// optimizations for Arduino
#include "ArduinoCommon.h"
// for managing the cursor
#include "LexContext.hpp"
#define JSON_ERROR_NO_ERROR 0
#define JSON_ERROR_UNTERMINATED_OBJECT 1
char JSON_ERROR_UNTERMINATED_OBJECT_MSG[] = PROGMEM "Unterminated object";
#define JSON_ERROR_UNTERMINATED_ARRAY 2
char JSON_ERROR_UNTERMINATED_ARRAY_MSG[] = PROGMEM "Unterminated array";
#define JSON_ERROR_UNTERMINATED_STRING 3
char JSON_ERROR_UNTERMINATED_STRING_MSG[] = PROGMEM "Unterminated string";
#define JSON_ERROR_UNTERMINATED_OBJECT_OR_ARRAY 4
char JSON_ERROR_UNTERMINATED_OBJECT_OR_ARRAY_MSG[] = PROGMEM "Unterminated object or array";
#define JSON_ERROR_FIELD_NO_VALUE 5
char JSON_ERROR_FIELD_NO_VALUE_MSG[] = PROGMEM "Field has no value";
#define JSON_ERROR_UNEXPECTED_VALUE 6
char JSON_ERROR_UNEXPECTED_VALUE_MSG[] = PROGMEM "Unexpected value";
#define JSON_ERROR_UNKNOWN_STATE 7
char JSON_ERROR_UNKNOWN_STATE_MSG[] = PROGMEM "Unknown state";
#define JSON_ERROR_OUT_OF_MEMORY 8
char JSON_ERROR_OUT_OF_MEMORY_MSG[] = PROGMEM "Out of memory";
#define JSON_ERROR_INVALID_ARGUMENT 9
char JSON_ERROR_INVALID_ARGUMENT_MSG[] = PROGMEM "Invalid argument";
#define JSON_ERROR_END_OF_DOCUMENT 10
char JSON_ERROR_END_OF_DOCUMENT_MSG[] = PROGMEM "End of document";
#define JSON_ERROR_NO_DATA 11
char JSON_ERROR_NO_DATA_MSG[] = PROGMEM "No data";
#define JSON_ERROR_FIELD_NOT_SUPPORTED 12
char JSON_ERROR_FIELD_NOT_SUPPORTED_MSG[] = PROGMEM "Individual fields cannot be returned from parseSubtree()";

using namespace lex;
namespace json
{
  #ifdef HTCW_JSONTREE
  // represents a filter for retrieving subsets of objects from the parser
  struct JsonParseFilter {
    // fields indicates fields which should be excluded:
    static const int8_t BlackList=0;
    // fields indicates which fields should be included:
    static const int8_t WhiteList=1;
    // indicates whether the parse filter is whitelist or blacklist based
    int8_t type;
    // the array of fields
    const char** pfields;
    // the count of fields
    size_t fieldCount;
    // the values for the whitelisted fields
    // if this is set to a pointer to an array 
    // with the same number of elements as fields
    // and the type is WhiteList this will be filled
    // with pointers to the JsonElements for each value
    // This is provided as an optimization opportunity
    // for certain common operations
    JsonElement** pvalues;
    // whitelists can be heirarchal. you can specify 
    // subobject filters for any whitelisted field
    JsonParseFilter** pfilters;

    JsonParseFilter() {
      type=BlackList;
      fieldCount = 0;
    }
    JsonParseFilter(const char* fields[],size_t fieldCount,int8_t type=BlackList) {
      this->pfields = fields;
      this->fieldCount = fieldCount;
      this->type=type;
      this->pvalues = nullptr;
      this->pfilters =nullptr;
    }
  };
  #endif
  // represents a pull parser for reading JSON documents
  class JsonReader {
    public:
      // as error node
      static const int8_t Error = -3;
      // an end document node
      static const int8_t EndDocument = -2;
      // an initial node
      static const int8_t Initial = -1;
      // an unknown node
      static const int8_t Unknown = -1;
      // a value node
      static const int8_t Value = 0;
      // a field node
      static const int8_t Field = 1;
      // an array node
      static const int8_t Array = 2;
      // an end array node
      static const int8_t EndArray = 3;
      // an object node
      static const int8_t Object = 4;
      // an end object node
      static const int8_t EndObject = 5;
      // a string type
      static const int8_t String = 6;
      // a number type
      static const int8_t Number = 7;
      // a boolean type
      static const int8_t Boolean = 8;
      // the null type
      static const int8_t Null = 9;
      // the siblings axis
      static const int8_t Siblings = 10;
      // the descendants axis
      static const int8_t Descendants = 11;
      // the special "all" axis
      static const int8_t All = 12;

    private:
      LexContext& m_lc;
      int8_t m_state;
      uint8_t m_lastError;
      unsigned long int m_objectDepth;
      size_t strcmps(const char** strs,const char* cmp,size_t size) {
        for(size_t i = 0;i<size;++i) {
          int result = strcmp(strs[i],cmp);
          if(!result)
            return i;
        }
        return -1;
      }
      uint8_t fromHexChar(char hex) {
        if (':' > hex && '/' < hex)
          return (uint8_t)(hex - '0');
        if ('G' > hex && '@' < hex)
          return (uint8_t)(hex - '7'); // 'A'-10
        return (uint8_t)(hex - 'W'); // 'a'-10
      }
      bool isHexChar(char hex) {
        return (':' > hex && '/' < hex) ||
              ('G' > hex && '@' < hex) ||
              ('g' > hex && '`' < hex);
      }
      char undecorateInline() {
        uint16_t uu;
        char ch=0;
        if (LexContext::EndOfInput!=m_lc.current() && (ch = m_lc.current()) != '\"') {
          switch (ch) {
            case '\\':
              if (LexContext::EndOfInput == m_lc.advance()) {
                m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
              }
              ch =(char)m_lc.current();
              switch (ch) {
                case '\'':
                case '\"':
                case '\\':
                case '/':
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }   
                  break;
                case 'r':
                  ch= '\r';
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }
                  break;
                case 'n':
                  ch = '\n';
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }
                  break;
                case 't':
                  ch = '\t';
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }
                  break;
                case 'b':
                  ch = '\b';
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }
                  break;
                case 'f':
                  ch = '\f';
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }
                  break;
                case 'u':
                  uu = 0;
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                  }
                  ch = (char)m_lc.current();
                  if (isHexChar(ch)) {
                    uu = fromHexChar(ch);
                    if (LexContext::EndOfInput == m_lc.advance()) {
                      m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                      STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                    }
                    ch = (char)m_lc.current();
                    uu *= 16;
                    if (isHexChar(ch)) {
                      uu |= fromHexChar(ch);
                      if (LexContext::EndOfInput == m_lc.advance()) {
                        m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                        STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                      }
                      ch = (char)m_lc.current();
                      uu *= 16;
                      if (isHexChar(ch)) {
                        uu |= fromHexChar(ch);
                        if (LexContext::EndOfInput == m_lc.advance()) {
                          m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                        }
                        ch = (char)m_lc.current();
                        uu *= 16;
                        if (isHexChar(ch)) {
                          uu |= fromHexChar(ch);
                          if (LexContext::EndOfInput == m_lc.advance()) {
                            m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                          }
                          ch = (char)m_lc.current();
                        }
                      }
                    }
                  }
                  if (0 < uu) {
                    // no unicode
                    if (256 > uu) {
                      ch = (char)uu;
                    } else
                      ch = '?';
                  }
              }
              break;
            default:
              if (LexContext::EndOfInput == m_lc.advance()) {
                m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
              }
              break;
          }
          return ch;
        }
        return 0;
      }
      char undecorateImpl(char*&szsrc) {
        uint16_t uu;
        char ch;
        if ((ch = *szsrc) && ch != '\"') {
          switch (ch) {
            case '\\':
              ch = *(++szsrc);

              switch (ch) {
                case '\'':
                case '\"':
                case '\\':
                case '/':
                  ++szsrc;
                  break;
                case 'r':
                  ch= '\r';
                  ++szsrc;
                  break;
                case 'n':
                  ch = '\n';
                  ++szsrc;
                  break;
                case 't':
                  ch = '\t';
                  ++szsrc;
                  break;
                case 'b':
                  ch = '\b';
                  ++szsrc;
                  break;
                case 'f':
                  ch = '\f';
                  ++szsrc;
                  break;
                case 'u':
                  uu = 0;
                  ch = *(++szsrc);
                  if (isHexChar(ch)) {
                    uu = fromHexChar(ch);
                    ch = *(++szsrc);
                    uu *= 16;
                    if (isHexChar(ch)) {
                      uu |= fromHexChar(ch);
                      ch = *(++szsrc);
                      uu *= 16;
                      if (isHexChar(ch)) {
                        uu |= fromHexChar(ch);
                        ch = *(++szsrc);
                        uu *= 16;
                        if (isHexChar(ch)) {
                          uu |= fromHexChar(ch);
                          ch = *(++szsrc);
                        }
                      }
                    }
                  }
                  if (0 < uu) {
                    // no unicode
                    if (256 > uu) {
                      ch = (char)uu;
                    } else
                      ch = '?';
                  }
              }
              break;
            default:
              ++szsrc;
              break;
          }
          return ch;
        }
        return 0;
      }
      void readLiteral(const char* value) {
        m_lastError = JSON_ERROR_NO_ERROR;
        const char*sz=value;
        if(nullptr==value) {
          m_lastError = JSON_ERROR_INVALID_ARGUMENT;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_INVALID_ARGUMENT_MSG, m_lc.captureCapacity());
          return;
        }
        char ch;
        while(ch=*sz) {
          if(ch!=m_lc.advance()) {
            m_lastError = JSON_ERROR_UNEXPECTED_VALUE;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
            break;
          }
          if(!m_lc.capture()) {
            m_lastError = JSON_ERROR_OUT_OF_MEMORY;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
            break;
          }
          ++sz;
        }
        if(m_lastError!=JSON_ERROR_NO_ERROR) {
          // try to recover the cursor
          while(true) {
            m_lc.advance();
            ch=(char)m_lc.current();
            if (',' == ch || ']' == ch || '}' == ch || LexContext::EndOfInput == ch) 
              return; 
          }
        } else {
          m_lc.advance();
          m_lc.trySkipWhiteSpace();
          ch = m_lc.current();
          if (',' != ch && ']' != ch && '}' != ch && LexContext::EndOfInput != ch) {
            m_lastError = JSON_ERROR_UNEXPECTED_VALUE;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
          }
        }
      }
      // optimization
      void skipObjectPart(int depth=1)
      {
        m_lastError = JSON_ERROR_NO_ERROR;
        while (Error != m_state && LexContext::EndOfInput != m_lc.current())
        {
          switch (m_lc.current())
          {
            case '[':
              if (LexContext::EndOfInput == m_lc.advance()) {
                m_lastError = JSON_ERROR_UNTERMINATED_ARRAY;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_ARRAY_MSG, m_lc.captureCapacity());
                return;
              }
              skipArrayPart();
              break;

            case '{':
              ++m_objectDepth;
              ++depth;
              m_lc.advance();
              if (LexContext::EndOfInput == m_lc.current()) {
                m_lastError = JSON_ERROR_UNTERMINATED_OBJECT;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
              }
              break;
            case '\"':
              skipString();
              break;
            case '}':
              m_lc.advance();
              --depth;
              --m_objectDepth;
              if (0>=depth)
              {
                m_lc.trySkipWhiteSpace();
                m_state=(LexContext::EndOfInput==m_lc.current())?EndDocument:EndObject;
                return;
              }
              if (LexContext::EndOfInput == m_lc.current()) {
                m_lastError = JSON_ERROR_UNTERMINATED_OBJECT;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
              }
              break;
            default:
              m_lc.advance();
              break;
          }
        }
      }

      void skipArrayPart(int depth=1)
      {
        m_lastError = JSON_ERROR_NO_ERROR;
        while (Error != m_state && LexContext::EndOfInput != m_lc.current())
        {
          switch (m_lc.current())
          {
            case '{':
              ++m_objectDepth;
              if (LexContext::EndOfInput == m_lc.advance()) {
                m_lastError = JSON_ERROR_UNTERMINATED_OBJECT;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
                m_state = Error;
              }
              skipObjectPart();
              break;
            case '[':
              ++depth;
              if (LexContext::EndOfInput == m_lc.advance()) {
                m_lastError = JSON_ERROR_UNTERMINATED_ARRAY;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_ARRAY_MSG, m_lc.captureCapacity());
                m_state = Error;
              }
              break;
            case '\"':
              skipString();
              if (m_lastError!=JSON_ERROR_NO_ERROR)
                return;
              break;
            case ']':
              --depth;
              m_lc.advance();
              if (0>=depth) {
                m_lc.trySkipWhiteSpace();
                m_state=(LexContext::EndOfInput==m_lc.current())?EndDocument:EndArray;
                return;
              }
              if (LexContext::EndOfInput == m_lc.current()) {
                m_lastError = JSON_ERROR_UNTERMINATED_ARRAY;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_ARRAY_MSG, m_lc.captureCapacity());
              }
              break;
            default:
              m_lc.advance();
              break;
          }
        }
      }

      bool skipString()
      {
        m_lastError = JSON_ERROR_NO_ERROR;
        if ('\"' != m_lc.current()) {
          m_lastError = JSON_ERROR_UNTERMINATED_STRING;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
          return false;
        }
        m_lc.advance();
        
        if (!m_lc.trySkipUntil('\"','\\', true)) {
          m_lastError = JSON_ERROR_UNTERMINATED_STRING;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
          return false;
        }
        m_lc.trySkipWhiteSpace();

        return true;
      }
      bool scanMatchField(const char* field) {
        bool match = false;
        const char* sz;
        char ch;
        if('\"'!=m_lc.current()) {
          m_lastError = JSON_ERROR_UNEXPECTED_VALUE;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
          return false;  
        }
        if (LexContext::EndOfInput == m_lc.advance()) {
          m_lastError = JSON_ERROR_UNTERMINATED_STRING;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
          return false;
        }
        match = true;
        sz=field;
        while(m_lc.current()!=LexContext::EndOfInput && m_lc.current()!='\"')
        {
          if(0==(*sz))
            break;
          ch=undecorateInline();
          if(0==ch) break;
          if(ch!=*sz) {
            match=false;
            break;
          }
          ++sz;
        }
        // read to the end of the field so the reader isn't broken
        if('\"'==m_lc.current()) {
          m_lc.advance();
          m_lc.trySkipWhiteSpace();
          if(m_lc.current()==':') { // it's a field. This is a match!
            if(LexContext::EndOfInput==m_lc.advance()) {
              m_lastError = JSON_ERROR_FIELD_NO_VALUE;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
              return false;
            }
            m_lc.trySkipWhiteSpace();
            m_state = Field;
            if(match && 0==(*sz))
              return JSON_ERROR_NO_ERROR==lastError();
          } 
        } else {
          // nothing to be done if it fails - i/o error or badly formed document
          // can't recover our cursor
          if(!m_lc.trySkipUntil('\"','\\',true)) {
            m_lastError = JSON_ERROR_UNTERMINATED_STRING;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
            return false;
          }
          m_lc.trySkipWhiteSpace();
          if(m_lc.current()==':') { // it's a field. This is a match!
            if(LexContext::EndOfInput==m_lc.advance()) {
              m_lastError = JSON_ERROR_FIELD_NO_VALUE;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
              return false;
            }
            m_lc.trySkipWhiteSpace();
            m_state = Field;
            return false;
          } else {
            m_lastError = JSON_ERROR_FIELD_NO_VALUE;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
            return false;
          }
        }
        return false;
      }
      bool scanMatchSiblings(const char* field) {
        while(true) {
          bool matched = scanMatchField(field);
          if(JSON_ERROR_NO_ERROR!=lastError()) return false;
          if(matched) {
            return true;
          } else {
            if(!skipFieldValuePart(true))
              return false;
          }
        }
        return false;
      }
      bool skipFieldValuePart(bool skipFinalRead = false) {
        if(m_state!=Field)
          return false;
        switch(m_lc.current()) {
          case LexContext::EndOfInput:
            m_lastError=JSON_ERROR_FIELD_NO_VALUE;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
            return false;
          
          case '[':
            if(LexContext::EndOfInput==m_lc.advance()) {
              m_lastError=JSON_ERROR_UNTERMINATED_ARRAY;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_ARRAY_MSG, m_lc.captureCapacity());
              return false;
            }
            m_lc.trySkipWhiteSpace();
            m_state=Array;
            skipArrayPart();
            if(m_state!=JsonReader::EndArray)
              return false;
            if(!skipFinalRead) {
              if(!read() || JSON_ERROR_NO_ERROR!=lastError())
                return false;
              return true;
            } else {
              if(LexContext::EndOfInput==m_lc.current()) {
                return true;
              }
              if(','==(char)m_lc.current()) {
                m_lc.advance();  
                m_lc.trySkipWhiteSpace();
                if(LexContext::EndOfInput==m_lc.current()) {
                  m_lastError=JSON_ERROR_UNTERMINATED_OBJECT;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
                  return false;
                }  
                if('\"'!=m_lc.current()) {
                  m_lastError=JSON_ERROR_UNEXPECTED_VALUE;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
                  return false;
                }
                m_state = Field;
                return true;
              }
              return false;
            }
              
            return true;
          case '{':
            if(LexContext::EndOfInput==m_lc.advance()) {
              m_lastError=JSON_ERROR_UNTERMINATED_OBJECT;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
              return false;
            }
            m_lc.trySkipWhiteSpace();
            m_state=Object;
            ++m_objectDepth;
            skipObjectPart();
          
            if(m_state!=JsonReader::EndObject)
              return false;
            if(!skipFinalRead) {
              if(!read() || JSON_ERROR_NO_ERROR!=lastError())
                return false;
              return true;
            } else {
              if(LexContext::EndOfInput==m_lc.current()) {
                return true;
              }
              if(','==(char)m_lc.current()) {
                m_lc.advance();  
                m_lc.trySkipWhiteSpace();
                if(LexContext::EndOfInput==m_lc.current()) {
                  m_lastError=JSON_ERROR_UNTERMINATED_OBJECT;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
                  return false;
                }  
                if('\"'!=m_lc.current()) {
                  m_lastError=JSON_ERROR_UNEXPECTED_VALUE;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
                  return false;
                }
                m_state = Field;
                return true;
              }
            }
            return false;         
          case '\"':
            
            if(LexContext::EndOfInput==m_lc.advance()) {
              m_lastError = JSON_ERROR_UNTERMINATED_STRING;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
              return false;
            }
            if(!m_lc.trySkipUntil('\"', '\\', true)) {
              m_lastError = JSON_ERROR_UNTERMINATED_STRING;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
              return false;
            }
            
            m_lc.trySkipWhiteSpace();
            if(m_lc.current()==',') {
              if(LexContext::EndOfInput==m_lc.advance()) {
                m_lastError=JSON_ERROR_UNTERMINATED_OBJECT;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
                return false;
              }
              m_lc.trySkipWhiteSpace();
              if(!skipFinalRead) {
                if(m_lc.current()!='\"') {
                  m_lastError=JSON_ERROR_UNEXPECTED_VALUE;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
                  return false;
                }
                m_state = Value;
                return read() && lastError()!=Error;
              } else return true;
            } else if(m_lc.current()!='}') {
              m_lastError=JSON_ERROR_UNEXPECTED_VALUE;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
              return false;
            } else {
              if(!skipFinalRead) {
                m_state = EndObject;
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
              }
              return true;
            }
            return false;
          default:
            // skip a scalar value
            while(LexContext::EndOfInput!=m_lc.advance() && m_lc.current()!=',' && m_lc.current()!='}');
            if(LexContext::EndOfInput==m_lc.current()) {
              m_lastError=JSON_ERROR_UNTERMINATED_OBJECT;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
              return false;
            }
            if('}'==m_lc.current()) {
              m_state = EndObject;
              --m_objectDepth;
              m_lc.advance();
              m_lc.trySkipWhiteSpace();
              return true;
            }
            // it was a ,
            if(LexContext::EndOfInput==m_lc.advance()) {
              m_lastError=JSON_ERROR_UNTERMINATED_OBJECT;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
              return false;
            }
            m_lc.trySkipWhiteSpace();
            if(!skipFinalRead) {
              if('\"'==m_lc.current()) {
                m_state = Value;
                if(read() && lastError()==JSON_ERROR_NO_ERROR && m_state==Field)
                    return true;
                
                // TODO: see if we need better error handling here
              }
            } else {
              if(m_lc.current()!='\"') {
                m_lastError=JSON_ERROR_UNEXPECTED_VALUE;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
                return false;
              }
              m_state=Field;
              return true;
            }
            return false;
        }   
      }
#ifdef HTCW_JSONTREE
    char* poolStringPointer(MemoryPool* stringPool,const char* sz) {
      if(nullptr==stringPool || nullptr==sz || nullptr==stringPool->base()) return nullptr;
      uint8_t* cur = (uint8_t*)stringPool->base();
      size_t len = strlen(sz)+1;
      // look for a matching string
      while(cur<stringPool->next()) {
        size_t c = *((size_t*)cur);
        cur+=sizeof(size_t);
        // if the lengths don't match, move to the next string
        if(c!=len) {
          cur+=c;
          continue;
        }
        const char* szcur = sz;
        char* szcmp= (char*)cur;
        char* szstart = szcmp;
        while(0!=*szcmp && 0!=*szcur && szcur<(char*)stringPool->next() && *(szcur)==*szcmp) {
          ++szcmp;
          ++szcur;
        }
        if(0!=(*szcmp) || 0!=(*szcur)) {
          // skip to the end of the string
          cur+=(c-(szcmp-szstart));
        } else {
          // match
          return szstart;
        }
      }
      // if we got here we have to allocate
      cur = (uint8_t*)stringPool->alloc(len+sizeof(size_t));
      if(nullptr==cur)
        return nullptr;
      *((size_t*)cur)=len;
      cur+=sizeof(size_t);
      strcpy((char*)cur,sz);
      return (char*)cur;
    }
#endif
    public:

      JsonReader(LexContext &context) : m_lc(context) {
        m_state = Initial;
        m_objectDepth=0;
      }
      LexContext& context() {
        return m_lc;
      }
      int8_t nodeType() {
        return m_lastError != JSON_ERROR_NO_ERROR?Error:m_state;
      }
      uint8_t lastError() {
        return m_lastError;
      }
      
      bool read() {
        if(JsonReader::Error!=m_state)
          m_lastError = JSON_ERROR_NO_ERROR;
        int16_t qc;
        int16_t ch;
        switch (m_state) {
          case JsonReader::Error:
          case JsonReader::EndDocument:
            return false;
          case JsonReader::Initial:
            m_lc.ensureStarted();
            m_state = Value;
          // fall through
          case JsonReader::Value:
  value_case:
            m_lc.clearCapture();
            switch (m_lc.current()) {
              case LexContext::EndOfInput:
                m_state = EndDocument;
                return false;
              case ']':
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                m_lc.clearCapture();
                m_state = EndArray;
                return true;
              case '}':
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                m_lc.clearCapture();
                m_state = EndObject;
                --m_objectDepth;
                return true;
              case ',':
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                if (!read()) { // read the next value
                  m_lastError = JSON_ERROR_UNTERMINATED_ARRAY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_ARRAY_MSG, m_lc.captureCapacity());
                  return true;  
                }
                return true;
              case '[':
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                m_state = Array;
                return true;
              case '{':
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                m_state = Object;
                ++m_objectDepth;
                return true;
              case '-':
              case '.':
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                qc = m_lc.current();
                if (!m_lc.capture()) {
                  m_lastError = JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return true;
                }
                while (LexContext::EndOfInput != m_lc.advance() &&
                      ('E' == m_lc.current() ||
                        'e' == m_lc.current() ||
                        '+' == m_lc.current() ||
                        '.' == m_lc.current() ||
                        isdigit((char)m_lc.current()))) {
                  if (!m_lc.capture()) {
                    m_lastError = JSON_ERROR_OUT_OF_MEMORY;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                    return true;
                  }
                }
                m_lc.trySkipWhiteSpace();
                return true;
              case '\"':
                m_lc.capture();
                m_lc.advance();
                if (!m_lc.tryReadUntil('\"', '\\', true)) {
                  if (LexContext::EndOfInput == m_lc.current()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                    return true;
                  } else {
                    m_lastError = JSON_ERROR_OUT_OF_MEMORY;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                    // try to recover the cursor
                    if(!m_lc.trySkipUntil('\"', '\\', true)) {
                      m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                      STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                      return true;
                    }
                    m_lc.trySkipWhiteSpace();
                    if (':' == m_lc.current())
                    {
                      m_lc.advance();
                      m_lc.trySkipWhiteSpace();
                      if (LexContext::EndOfInput == m_lc.current()) {
                        m_lastError = JSON_ERROR_FIELD_NO_VALUE;
                        STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
                        return true;
                      }
                      m_state = Field;
                    } else
                      m_state = Value;
                    return JSON_ERROR_NO_ERROR==m_lastError;
                  }
                }
                m_lc.trySkipWhiteSpace();
                if (':' == m_lc.current())
                {
                  m_lc.advance();
                  m_lc.trySkipWhiteSpace();
                  if (LexContext::EndOfInput == m_lc.current()) {
                    m_lastError = JSON_ERROR_FIELD_NO_VALUE;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
                    return true;
                  }
                  m_state = Field;
                }
                return true;
              case 't':
                if (!m_lc.capture()) {
                  m_lastError = JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return true;
                }
                readLiteral("rue");
                return true;
              case 'f':
                if (!m_lc.capture()) {
                  m_lastError = JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return true;
                }
                readLiteral("alse");
                return true;
              case 'n':
                if (!m_lc.capture()) {
                  m_lastError = JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return true;
                }
                readLiteral("ull");
                return true;
              default:
                // printf("Line %d, Column %d, Position %d\r\n",_lc.line(),_lc.column(),(int32_t)_lc.position());
                m_lastError = JSON_ERROR_UNEXPECTED_VALUE;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNEXPECTED_VALUE_MSG, m_lc.captureCapacity());
                return true;

            }
          default:
            m_state = Value;
            goto value_case;
        }
      }
#ifdef HTCW_JSONTREE
    JsonElement* parseSubtree(mem::MemoryPool& pool,JsonParseFilter* pfilter = nullptr,mem::MemoryPool* pstringPool=nullptr,bool poolValues=false)
		{
      size_t c;
      char* fn;
      JsonElement e;
      JsonElement* pse;
      size_t fi;
      JsonParseFilter *pnextFilter=nullptr;
      switch (m_state)
			{
				case EndDocument:
          m_lastError = JSON_ERROR_END_OF_DOCUMENT;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_END_OF_DOCUMENT_MSG, m_lc.captureCapacity());
					return nullptr;
				case Initial:
					if (!read()) {
            m_lastError = JSON_ERROR_NO_DATA;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_NO_DATA_MSG, m_lc.captureCapacity());
            return nullptr;
          }
					return parseSubtree(pool,pfilter,pstringPool,poolValues);
				case Value:
          switch(valueType()) {
            case Null:
              e.null(nullptr);
              break;
            case Number:
              // we handle integers specially in order to avoid
              // losing fidelity as much as possible
              if(isIntegerValue()) {
                e.integer(integerValue());
              } else {
                e.real(numericValue());
              }
              break;
            case Boolean:
              e.boolean(booleanValue());
              break;
            case String:
              undecorate();
              
              if(poolValues && nullptr!=pstringPool) {
                char *psz = poolStringPointer(pstringPool,value());
                if(nullptr==psz) {
                  m_lastError=JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return nullptr;
                }
                e.string(psz);
                break;
              }
              if(!e.allocString(pool,value())) {
                m_lastError=JSON_ERROR_OUT_OF_MEMORY;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                return nullptr;
              }
            
              break;
          }
          read();
					break;
				case Field: 
          // we have no structure with which to return a field
          m_lastError = JSON_ERROR_FIELD_NOT_SUPPORTED;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NOT_SUPPORTED_MSG, m_lc.captureCapacity());
          return nullptr;
				case Array:
          e.parray(nullptr);
          if(!read())
            return nullptr;
					while (EndArray != m_state) {
					  JsonElement *pitem =parseSubtree(pool,pfilter,pstringPool,poolValues);
            if(nullptr==pitem)
              return nullptr;
            if(!e.addItem(pool,pitem)) {
              m_lastError=JSON_ERROR_OUT_OF_MEMORY;
              STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_NO_DATA_MSG, m_lc.captureCapacity());
              return nullptr;
            }
          }
          if(EndArray==m_state && !read()) 
            return nullptr;
					break;
				case EndArray: 
        case EndObject:
					// we have no data to return
          m_lastError = JSON_ERROR_NO_DATA;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_NO_DATA_MSG, m_lc.captureCapacity());
          return nullptr;
				case Object:// begin object
					e.pobject(nullptr);
          if(!read()) {
            return nullptr;
          }
          if(pfilter && pfilter->pvalues) {
            memset(pfilter->pvalues,0,sizeof(pfilter->fieldCount*sizeof(JsonElement*)));
          }
          fi=0;
          while (JSON_ERROR_NO_ERROR==lastError() && EndObject != m_state)
					{
            undecorate();
            bool skip = false;
            if(pfilter && value()) {
              switch (pfilter->type)
              {
              case JsonParseFilter::BlackList:
                pnextFilter=nullptr;
                if(pfilter->fieldCount && -1!=strcmps(pfilter->pfields,value(),pfilter->fieldCount)) {
                  // skip this field, it's in the blacklist
                  skip=true;
                }
                break;
              case JsonParseFilter::WhiteList:
                skip=true;
                if(pfilter->fieldCount) {
                  fi=strcmps(pfilter->pfields,value(),pfilter->fieldCount);
                  if(-1!=fi) {
                    // skip this field, it's in the blacklist
                    skip=false;
                    // set the next filter if we have one
                    if(nullptr!=pfilter->pfilters) {
                      pnextFilter=pfilter->pfilters[fi];;
                    } 
                  }
                }
                break;
              }
            }
     
            if(!skip) {
              if(nullptr==pstringPool) {
                c=strlen(value())+1;
                fn=(char*)pool.alloc(c);
                if(nullptr==fn) {
                  m_lastError=JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return nullptr;
                }
                strcpy(fn,value());
              } else {
                fn=poolStringPointer(pstringPool,value());
                if(nullptr==fn) {
                  m_lastError=JSON_ERROR_OUT_OF_MEMORY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                  return nullptr;
                }
              }
              if(!read()) {
                m_lastError=JSON_ERROR_FIELD_NO_VALUE;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
                return nullptr;
              }
              
              if(nullptr==fn || nullptr==(pse=parseSubtree(pool,pnextFilter,pstringPool,poolValues)) || !e.addFieldPooled(pool,fn,pse))
              {
                m_lastError=JSON_ERROR_OUT_OF_MEMORY;
                STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
                return nullptr;
              }    
              pnextFilter=nullptr;
              if(pfilter && pfilter->type==JsonParseFilter::WhiteList && pfilter->pvalues) {
                   pfilter->pvalues[fi]=pse;
              }

            } else {
              if(!skipSubtree())
                return nullptr;
            }
					}
          if(EndObject==m_state && !read()) 
            if( EndDocument!=m_state)
              return nullptr;
					break;
				default:
          m_lastError = JSON_ERROR_UNKNOWN_STATE;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNKNOWN_STATE_MSG, m_lc.captureCapacity());
          return nullptr;
			}
      // sanity check
			if(!e.undefined()) {
        JsonElement *presult= (JsonElement*)pool.alloc(sizeof(JsonElement));
        if(presult==nullptr) {
          m_lastError=JSON_ERROR_OUT_OF_MEMORY;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_OUT_OF_MEMORY_MSG, m_lc.captureCapacity());
          return nullptr;
        }
        *presult=e;
        return presult;
      }
      // shouldn't get here
      return nullptr;
		}
#endif
      bool skipSubtree()
      {
        m_lastError=JSON_ERROR_NO_ERROR;
        switch (m_state)
        {
          case JsonReader::Error:
          case JsonReader::EndDocument: // eos
            return false;
          case JsonReader::Initial: // initial
            if (read() && Error!=nodeType())
              return skipSubtree();
            return false;
          case JsonReader::Value: // value
            if(read() && Error!=nodeType())
              return true;
            return false;
          case JsonReader::Field: // field
            // we're doing this here to avoid loading values
            // if we just call read() to advance, the field's 
            // value will be loaded. This is undesirable if
            // we don't care about the field because it uses
            // lexcontext space
            // instead we're advancing manually. We start right 
            // after the field's ':' delimiter
            if(!skipFieldValuePart())
              return false;
            return true;
          case JsonReader::Array:// begin array
            skipArrayPart();
            if(m_state!=JsonReader::EndArray || !read() || Error==nodeType())
              return false;
            //_lc.trySkipWhiteSpace();
            return true;
          case JsonReader::Object:// begin object
            skipObjectPart();
            if(m_state!=JsonReader::EndObject || !read() || Error==nodeType())
              return false;
            //_lc.trySkipWhiteSpace();
            return true;
          case JsonReader::EndArray: // end array
          case JsonReader::EndObject: // end object
            if(Error==nodeType())
              return false;
            return true;
          default:
            m_lastError = JSON_ERROR_UNKNOWN_STATE;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNKNOWN_STATE_MSG, m_lc.captureCapacity());
            return false;
        }
      }
      bool skipToIndex(int index) {
        m_lastError=JSON_ERROR_NO_ERROR;
        if (Initial == m_state || Field == m_state) // initial or field
          if (!read() || Error==nodeType())
            return false;
        if (Array == m_state) { // array start
          if (0 == index) {
            if (!read() || Error==nodeType())
              return false;
          }
          else {
            for (int i = 0; i < index; ++i) {
              if (EndArray == m_state) // end array
                return false;
              if (!read() || Error==nodeType())
                return false;
              if (!skipSubtree())
                return false;
            }
            if ((EndObject == m_state || EndArray == m_state) && !read() || Error==nodeType())
              return false;
          }
          return true;
        }
        return false;
      }
      
      bool skipToField(const char* field, int8_t searchAxis,unsigned long int *pdepth=nullptr) {

        m_lastError=JSON_ERROR_NO_ERROR;
        if(!field) {
          m_lastError = JSON_ERROR_INVALID_ARGUMENT;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_INVALID_ARGUMENT_MSG, m_lc.captureCapacity());
          return false;
        }
        bool match;
        char ch;
        const char*sz;
        switch(searchAxis) {
        case Siblings:
          switch(m_state) {
          case Initial:
            if(!read()||Object!=m_state)
              return false;
            // fall through
          case Object:
            if(scanMatchSiblings(field)) {
              return true;
            }
            if(JSON_ERROR_NO_ERROR!=lastError())
              return false;
          case Field:
            if(!skipFieldValuePart(true))
              return false;
            if(scanMatchSiblings(field)) {
              return true;
            }
            if(JSON_ERROR_NO_ERROR!=lastError())
              return false;
          } 
          break;
        case Descendants:
          if(nullptr==pdepth) {
            m_lastError = JSON_ERROR_INVALID_ARGUMENT;
            STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_INVALID_ARGUMENT_MSG, m_lc.captureCapacity());
            return false;
          }
          if(m_state==Initial) {
            if(!read())
              return false;
          }
          
          if(0==(*pdepth))
            *pdepth=1;
          // fall through
        case All:
          while (LexContext::EndOfInput != m_lc.current()) {
              switch (m_lc.current()) {
                case '\"':
                  if (LexContext::EndOfInput == m_lc.advance()) {
                    m_lastError = JSON_ERROR_UNTERMINATED_STRING;
                    STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_STRING_MSG, m_lc.captureCapacity());
                    return false;
                  }
                  match = true;
                  sz=field;
                  while(m_lc.current()!=LexContext::EndOfInput && m_lc.current()!='\"')
                  {
                    if(0==(*sz))
                      break;
                    ch=undecorateInline();
                    if(0==ch) break;
                    if(ch!=*sz) {
                      match=false;
                      break;
                    }
                    ++sz;
                  }
                  // read to the end of the field so the reader isn't broken
                  if('\"'==m_lc.current()) {
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    if(m_lc.current()==':') { // it's a field. This is a match!
                      if(LexContext::EndOfInput==m_lc.advance()) {
                        m_lastError = JSON_ERROR_FIELD_NO_VALUE;
                        STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_FIELD_NO_VALUE_MSG, m_lc.captureCapacity());
                        return false;
                      }
                      m_lc.trySkipWhiteSpace();
                      m_state = Field;
                      if(match && !(*sz))
                        return JSON_ERROR_NO_ERROR==lastError();
                    } 
                  } else if(!m_lc.trySkipUntil('\"','\\',true)) {
                    return false;
                  }
                  break;
              case '{':
                if(LexContext::EndOfInput==m_lc.advance()) {
                  m_lastError = JSON_ERROR_UNTERMINATED_OBJECT;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_OBJECT_MSG, m_lc.captureCapacity());
                  return false;
                }
                if(pdepth)
                  ++(*pdepth);
                ++m_objectDepth;
         
                break;
              case '[':
                if(LexContext::EndOfInput==m_lc.advance()) {
                  m_lastError = JSON_ERROR_UNTERMINATED_ARRAY;
                  STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_UNTERMINATED_ARRAY_MSG, m_lc.captureCapacity());
                  return false;
                }
                if(pdepth)
                  ++(*pdepth);
                
                break;
              case '}':
                --m_objectDepth;
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                if(pdepth) {
                  --*pdepth;
                  if(!*pdepth) {
                    m_state = EndObject;
                    return false;
                  }
                }
                break;
              case ']':
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                if(pdepth) {
                  --*pdepth;
                  if(!*pdepth) {
                    m_state = EndArray;
                    return false;
                  }
                }
                break;
              default:
                m_lc.advance();
                break;
              }
            }
            return false;
        default:
          m_lastError = JSON_ERROR_INVALID_ARGUMENT;
          STRNCPYP(m_lc.captureBuffer(), JSON_ERROR_INVALID_ARGUMENT_MSG, m_lc.captureCapacity());
          return false;
          

        }
        return false;
      }
      
      bool skipToEndObject() {
        if(m_state==Error) return false;
        skipObjectPart(0);
        return (m_state != EndDocument);
      }
      bool skipToEndArray() {
        if(m_state==Error) return false;
        skipArrayPart(0);
        return (m_state != EndDocument);
      }
      unsigned long int objectDepth() { return m_objectDepth;}
      int8_t valueType() {
        if(m_state!=Error && m_state!=Value && m_state!=Field)
          return Unknown;
        if(m_state==Error)
          return String;
        char *sz = m_lc.captureBuffer();
        char ch = *sz;
        if ('\"' == ch)
          return String;
        if ('t' == ch || 'f' == ch)
          return Boolean;
        if ('n' == ch)
          return Null;
        return Number;
      }
      bool isIntegerValue() {
        if(valueType()!=Number)
          return false;
        char*sz = value();
        if('-'==*sz)
          ++sz;
        if(!(*sz))
          return false;
        while(*sz && isdigit(*sz)) ++sz;
        return !(*sz);
      }
      long long integerValue() {
        if(isIntegerValue())
          return atoll(value());
        return 0;
      }
      bool booleanValue() {
        return 't' == *m_lc.captureBuffer();
      }
      double numericValue() {
        return strtod(m_lc.captureBuffer(), NULL);
      }
      
      void undecorate() {
        char *src = m_lc.captureBuffer();
        char *dst = src;
        char ch = *src;
        if ('\"' != ch)
          return;
        ++src;
        while(ch=undecorateImpl(src))
          *(dst++)=ch;
        *dst=0;
      }
      char* value() {
        switch (m_state) {
          case JsonReader::Field:
          case JsonReader::Value:
          case JsonReader::Error:
            return m_lc.captureBuffer();
        }
        return NULL;
      }
  };
}
#endif