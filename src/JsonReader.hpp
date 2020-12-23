#include <math.h>
#ifndef ARDUINO
#include <cinttypes>
#include <string.h>
#endif
#include "ArduinoCommon.h"
#include "LexContext.h"
#include "JsonTree.hpp"
namespace json {
#define JSON_ERROR_NO_ERROR 0
#define JSON_ERROR_UNTERMINATED_OBJECT 1
const char JSON_ERROR_UNTERMINATED_OBJECT_MSG[] PROGMEM =  "Unterminated object";
#define JSON_ERROR_UNTERMINATED_ARRAY 2
const char JSON_ERROR_UNTERMINATED_ARRAY_MSG[] PROGMEM =  "Unterminated array";
#define JSON_ERROR_UNTERMINATED_STRING 3
const char JSON_ERROR_UNTERMINATED_STRING_MSG[] PROGMEM =  "Unterminated string";
#define JSON_ERROR_UNTERMINATED_OBJECT_OR_ARRAY 4
const char JSON_ERROR_UNTERMINATED_OBJECT_OR_ARRAY_MSG[] PROGMEM =  "Unterminated object or array";
#define JSON_ERROR_FIELD_NO_VALUE 5
const char JSON_ERROR_FIELD_NO_VALUE_MSG[] PROGMEM =  "Field has no value";
#define JSON_ERROR_UNEXPECTED_VALUE 6
const char JSON_ERROR_UNEXPECTED_VALUE_MSG[] PROGMEM =  "Unexpected value";
#define JSON_ERROR_UNKNOWN_STATE 7
const char JSON_ERROR_UNKNOWN_STATE_MSG[] PROGMEM =  "Unknown state";
#define JSON_ERROR_OUT_OF_MEMORY 8
const char JSON_ERROR_OUT_OF_MEMORY_MSG[] PROGMEM =  "Out of memory";
#define JSON_ERROR_INVALID_ARGUMENT 9
const char JSON_ERROR_INVALID_ARGUMENT_MSG[] PROGMEM =  "Invalid argument";
#define JSON_ERROR_END_OF_DOCUMENT 10
const char JSON_ERROR_END_OF_DOCUMENT_MSG[] PROGMEM =  "End of document";
#define JSON_ERROR_NO_DATA 11
const char JSON_ERROR_NO_DATA_MSG[] PROGMEM =  "No data";
#define JSON_ERROR_FIELD_NOT_SUPPORTED 12
const char JSON_ERROR_FIELD_NOT_SUPPORTED_MSG[] PROGMEM =  "Individual fields cannot be returned from parseSubtree()";
#define JSON_ERROR_UNEXPECTED_FIELD 13
const char JSON_ERROR_UNEXPECTED_FIELD_MSG[] PROGMEM =  "Unexpected field";

#define JSON_ERROR(x) error(JSON_ERROR_ ## x,JSON_ERROR_ ## x ## _MSG)
    class JsonReader {
    public:
        // the initial node
        static const int8_t Initial = -1;
        // the end document/final node
        static const int8_t EndDocument = -2;
        // an error node
        static const int8_t Error = -3;
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

        // the type is undefined. this indicates an error in reading the value
        static const int8_t Undefined = -1;
        // this type is a null
        static const int8_t Null = 0;
        // this type is a string
        static const int8_t String = 1;
        // this type is a real number (floating point)
        static const int8_t Real = 2;
        // this type is an integer (not part of the JSON spec)
        static const int8_t Integer = 3; 
        // this type is a boolean
        static const int8_t Boolean = 4;

        // this axis is a fast forward search that ignores heirarchy
        static const int8_t Forward = 0;
        // this axis retrieves sibling fields
        static const int8_t Siblings = 1;
        // this axis retrieves descendant fields
        static const int8_t Descendants = 2;
        
    private:
        int8_t m_state;
        int8_t m_valueType;
        uint8_t m_lastError;
        LexContext& m_lc;
        unsigned long int m_objectDepth;
        void error(uint8_t code,const char* msg) {
            m_lastError = code;
            size_t c = strlen(msg);
            if(c<m_lc.captureCapacity()) {
                STRNCPYP(m_lc.captureBuffer(),msg,c+1);
            } else {
                STRNCPYP(m_lc.captureBuffer(),msg,m_lc.captureCapacity());
            }
        }
        void clearError() {
            m_lastError = JSON_ERROR_NO_ERROR;
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
                        JSON_ERROR(UNTERMINATED_STRING);
                        m_state = Error; // unrecoverable
                        return 0;
                    }
                    ch =(char)m_lc.current();
                    switch (ch) {
                        case '\'':
                        case '\"':
                        case '\\':
                        case '/':
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING);
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        break;
                        case 'r':
                        ch= '\r';
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING); 
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        break;
                        case 'n':
                        ch = '\n';
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING);
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        break;
                        case 't':
                        ch = '\t';
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING);
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        break;
                        case 'b':
                        ch = '\b';
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING);
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        break;
                        case 'f':
                        ch = '\f';
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING);
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        break;
                        case 'u':
                        uu = 0;
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_STRING);
                            m_state = Error; // unrecoverable
                            return 0;
                        }
                        ch = (char)m_lc.current();
                        if (isHexChar(ch)) {
                            uu = fromHexChar(ch);
                            if (LexContext::EndOfInput == m_lc.advance()) {
                                JSON_ERROR(UNTERMINATED_STRING);
                                m_state = Error; // unrecoverable
                                return 0;
                            }
                            ch = (char)m_lc.current();
                            uu *= 16;
                            if (isHexChar(ch)) {
                                uu |= fromHexChar(ch);
                                if (LexContext::EndOfInput == m_lc.advance()) {
                                    JSON_ERROR(UNTERMINATED_STRING);
                                    m_state = Error; // unrecoverable
                                    return 0;
                                }
                                ch = (char)m_lc.current();
                                uu *= 16;
                                if (isHexChar(ch)) {
                                    uu |= fromHexChar(ch);
                                    if (LexContext::EndOfInput == m_lc.advance()) {
                                        JSON_ERROR(UNTERMINATED_STRING);
                                        m_state = Error; // unrecoverable
                                        return 0;
                                    }
                                    ch = (char)m_lc.current();
                                    uu *= 16;
                                    if (isHexChar(ch)) {
                                        uu |= fromHexChar(ch);
                                        if (LexContext::EndOfInput == m_lc.advance()) {
                                            JSON_ERROR(UNTERMINATED_STRING);
                                            m_state = Error; // unrecoverable
                                            return 0;
                                        }
                                        ch = (char)m_lc.current();
                                    }
                                }
                            }
                        }
                        if (0 < uu && uu<256) {
                            ch = uu;
                        } else
                            ch='?';
                        
                    }
                    break;
                    default:
                    if (LexContext::EndOfInput == m_lc.advance()) {
                        JSON_ERROR(UNTERMINATED_STRING);
                        m_state = Error; // unrecoverable
                        return 0;
                    }
                    break;
                }
                return ch;
            }
            return 0;
        }
        bool readLiteral(const char* value) {
            const char*sz=value;
            if(nullptr==value) {
                JSON_ERROR(INVALID_ARGUMENT);
                return false;
            }
            char ch;
            while((ch=*sz)) {
                if(ch!=m_lc.advance()) {
                    JSON_ERROR(UNEXPECTED_VALUE);
                    break;
                }
                if(!m_lc.capture()) {
                    JSON_ERROR(OUT_OF_MEMORY);
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
                        break;
                }
            } else {
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                ch = m_lc.current();
                if (',' != ch && ']' != ch && '}' != ch && LexContext::EndOfInput != ch) {
                    JSON_ERROR(UNEXPECTED_VALUE);
                    m_state = Error; // unrecoverable
                    return false;
                }
                return true;
            }
            return false;
        }
        void recoverValue() {
            // try to recover 
            char ch;
            while(true) {
                ch=(char)m_lc.advance();
                if (',' == ch || ']' == ch || '}' == ch || LexContext::EndOfInput == ch) 
                    break;
            }
            if(m_lc.current()==',') {
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                m_state = Value;
            }
        }
        bool readFieldOrEndObject() {
            switch(m_lc.current()) {
                case '}':
                    --m_objectDepth;
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    m_state = EndObject;
                    return true;
                case '\"':
                    if(!readString(true))
                        return false;
                    if(Value==m_state) {
                        JSON_ERROR(FIELD_NO_VALUE);
                        return false;
                    }
                    return true;
            }
            return false;
        }
        bool readValueOrEndArray() {
            switch(m_lc.current()) {
                case ']':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    m_state = EndArray;
                    return true;
                
                default:
                    if(!readAnyOpen(false))
                        return false;
                    return true;
            }
        }
        bool readEndObject() {
            if(m_lc.current()!='}')
                return false;
            m_lc.advance();
            m_lc.trySkipWhiteSpace();
            m_state = EndObject;
            return true;
        }
        bool readEndArray() {
            if(m_lc.current()!=']')
                return false;
            m_lc.advance();
            m_lc.trySkipWhiteSpace();
            m_state = EndArray;
            return true;
        }
        bool readString(bool allowFields) {
            if('\"'!=m_lc.current())
                return false;
            if(LexContext::EndOfInput==m_lc.advance()) {
                JSON_ERROR(UNTERMINATED_STRING);            
                m_state = Error; // unrecoverable
                return false;
            }
            m_lc.clearCapture();
            int16_t ch;
            while('\"'!=m_lc.current() && 0!=(ch=undecorateInline())) {
                if(!m_lc.capture(ch)) {
                    JSON_ERROR(OUT_OF_MEMORY);
                    m_lc.trySkipUntil('\"','\\',true);
                    m_state = Value;
                    return false;
                }
            }
            if(m_lc.current()!='\"')
                return false; // error should have already been set
            m_lc.advance();
            m_lc.trySkipWhiteSpace();
            m_state = Value;
            m_valueType = String;
            if(m_lc.current()==':') {
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                m_valueType = Undefined;
                if(LexContext::EndOfInput==m_lc.current()) {
                    JSON_ERROR(FIELD_NO_VALUE);
                    m_state = Error; // unrecoverable
                    return false;
                }
                if(']'==m_lc.current()||'}'==m_lc.current()||','==m_lc.current()) {
                    JSON_ERROR(FIELD_NO_VALUE);
                    m_state = Error; // unrecoverable
                    return false;
                }
                if(!allowFields) {
                    JSON_ERROR(UNEXPECTED_FIELD);
                    m_state = Field; // TODO: should be unrecoverable?
                    return false;
                } else {
                    m_state = Field;
                    return true;
                }
            }
            return true;
        }
        bool readAnyOpen(bool allowFields=false) {
            m_valueType = Undefined;
            switch(m_lc.current()) {
                case '[':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    if(LexContext::EndOfInput==m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_ARRAY);
                        m_state = Error; // unrecoverable            
                        return false;
                    }
                    m_lc.trySkipWhiteSpace();
                    m_state = Array;
                    return true;
                    
                case '{':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    if(LexContext::EndOfInput==m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_OBJECT); 
                        m_state = Error; // unrecoverable           
                        return false;
                    }
                    m_lc.trySkipWhiteSpace();
                    m_state = Object;
                    ++m_objectDepth;
                    return true;
                    
                case '\"':
                    // m_valueType is set in readString()
                    if(!readString(allowFields)) {
                        return false;
                    }
                    if(m_state==Field)
                        return true; // short circuit for field names
                    break;

                case 't':
                    m_lc.clearCapture();
                    if(!m_lc.capture()) {
                        JSON_ERROR(OUT_OF_MEMORY);
                        recoverValue();
                        return false;

                    }
                    if(!readLiteral("rue"))
                        return false;
                    m_valueType = Boolean;
                    break;

                case 'f':
                    m_lc.clearCapture();
                    if(!m_lc.capture()) {
                        JSON_ERROR(OUT_OF_MEMORY);
                        recoverValue();
                        return false;
                    }
                    if(!readLiteral("alse"))
                        return false;
                    m_valueType = Boolean;
                    break;
                    
                case 'n':
                    m_lc.clearCapture();
                    if(!m_lc.capture()) {
                        JSON_ERROR(OUT_OF_MEMORY);
                        recoverValue();
                        return false;
                    }
                    if(!readLiteral("ull"))
                        return false;
                    m_valueType = Null;
                    break;
                case '.':
                    m_valueType = Real;
                    break;
                case '-':
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
                    m_lc.clearCapture();
                    if(Real!=m_valueType)
                        m_valueType=Integer;
                    if (!m_lc.capture()) {
                        JSON_ERROR(OUT_OF_MEMORY);
                        recoverValue();
                        return false;
                    }
                    bool isReal=false;
                    while (LexContext::EndOfInput != m_lc.advance() &&
                        (isReal=('E' == m_lc.current() ||
                            'e' == m_lc.current() ||
                            '+' == m_lc.current() ||
                            '.' == m_lc.current()) ||
                            isdigit((char)m_lc.current()))) {
                        if (!m_lc.capture()) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            recoverValue();
                            if(isReal)
                                m_valueType=Real;
                            return false;
                        }
                    }
                    if(isReal)
                        m_valueType = Real;
                    break;
            }
            m_lc.trySkipWhiteSpace();
            m_state=Value;
            return true;
        }
        bool skipObjectPart(int depth=1)
        {
            clearError();
            while (Error != m_state && LexContext::EndOfInput != m_lc.current())
            {
                switch (m_lc.current())
                {
                    case '[':
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_ARRAY);
                            m_state=Error; // unrecoverable
                            return false;
                        }
                        if(!skipArrayPart())
                            return false;
                    break;

                    case '{':
                        ++m_objectDepth;
                        ++depth;
                        m_lc.advance();
                        if (LexContext::EndOfInput == m_lc.current()) {
                            JSON_ERROR(UNTERMINATED_OBJECT);
                            m_state=Error; // unrecoverable
                            return false;
                        }
                    break;

                    case '\"':
                        if(!skipString())
                            return false;
                        break;

                    case '}':
                        --depth;
                        --m_objectDepth;
                        if (0>=depth)
                        {
                            m_state=(LexContext::EndOfInput==m_lc.current())?EndDocument:EndObject;
                            m_lc.advance();
                            m_lc.trySkipWhiteSpace();
                            return true;
                        }
                        m_lc.advance();
                        m_lc.trySkipWhiteSpace();
                            
                        if (LexContext::EndOfInput == m_lc.current()) {
                            JSON_ERROR(UNTERMINATED_OBJECT);
                            m_state=Error; // unrecoverable
                            return false;
                        }
                        break;

                    default:
                        m_lc.advance();
                        break;
                }
            }
            return false;
        }

        bool skipArrayPart(int depth=1)
        {
            clearError();
            while (Error != m_state && LexContext::EndOfInput != m_lc.current())
            {
                switch (m_lc.current())
                {
                    case '{':
                        ++m_objectDepth;
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_OBJECT);
                            m_state = Error; // unrecoverable
                            return false;
                        }
                        if(!skipObjectPart())
                            return false;
                        break;

                    case '[':
                        ++depth;
                        if (LexContext::EndOfInput == m_lc.advance()) {
                            JSON_ERROR(UNTERMINATED_ARRAY);
                            m_state = Error; // unrecoverable
                            return false;
                        }
                        break;

                    case '\"':
                        if(!skipString())
                            return false;
                        break;

                    case ']':
                        --depth;
                        m_lc.advance();
                        if (0>=depth) {
                            m_lc.trySkipWhiteSpace();
                            m_state=(LexContext::EndOfInput==m_lc.current())?EndDocument:EndArray;
                            return true;
                        }
                        if (LexContext::EndOfInput == m_lc.current()) {
                            JSON_ERROR(UNTERMINATED_ARRAY);
                            m_state = Error; // unrecoverable
                            return false;
                        }
                    break;
                    
                    default:
                        m_lc.advance();
                        break;
                }
            }
            return false;
        }
        bool skipObjectOrArrayOrValuePart() {
            switch(m_lc.current()) {
                case '[':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    if(LexContext::EndOfInput==m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_ARRAY);
                        m_state = Error;
                        return false;
                    }
                    if(!skipArrayPart())
                        return false;        
                    break;
                case '{':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    if(LexContext::EndOfInput==m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_OBJECT);
                        m_state = Error;
                        return false;
                    }
                    if(!skipObjectPart())
                        return false;        
                    break;
                default:
                    if(!skipValuePart())
                        return false;
                    break;
            }
            return true;
        }
        bool skipString()
        {
            clearError();
            if ('\"' != m_lc.current()) {
                JSON_ERROR(UNTERMINATED_STRING);
                m_state=Error; // unrecoverable
                return false;
            }
            m_lc.advance();
            
            if (!m_lc.trySkipUntil('\"','\\', true)) {
                JSON_ERROR(UNTERMINATED_STRING);
                m_state=Error; // unrecoverable
                return false;
            }
            m_lc.trySkipWhiteSpace();
            m_state = Value;
            return true;
        }     
        // skips scalar values only
        bool skipValuePart() {
            clearError();
            switch(m_lc.current()) {
                case '\"':
                    if(!skipString())
                        return false;
                    return true;
                default:
                    while(LexContext::EndOfInput!=m_lc.advance() && ','!=m_lc.current() && ']'!=m_lc.current() && '}'!=m_lc.current());
                    m_state = Value;
                    return true;
            }
        }   
        bool readAny() {
            switch(m_lc.current()) {
                case ']':
                    return readEndArray();

                case '}':                            
                    return readEndObject();

            }
            skipIfComma();
            return readAnyOpen(true);
        }
        bool skipIfComma() {
            if(m_lc.current()==',') {
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                if(LexContext::EndOfInput==m_lc.current()) {
                    JSON_ERROR(UNTERMINATED_OBJECT_OR_ARRAY);
                    m_state = Error; // unrecoverable
                    return false;
                }
            }
            return true;
        }
        bool skipCommaOrEndObjectOrEndArray() {
            switch(m_lc.current()) {
                case ',':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    if(LexContext::EndOfInput==m_lc.current()) {
                        JSON_ERROR(UNTERMINATED_OBJECT_OR_ARRAY);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    break;
                case ']':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    m_state = EndArray;
                    break;
                case '}':
                    m_lc.advance();
                    m_lc.trySkipWhiteSpace();
                    --m_objectDepth;
                    m_state = EndObject;
                    break;
                default:
                    return false;
            }
            return true;
        }
        bool scanMatchField(const char* field) {
            bool match = false;
            const char* sz;
            int16_t ch;
            if('\"'!=m_lc.current()) {
                JSON_ERROR(UNEXPECTED_VALUE);
                return false;
            }
            if (LexContext::EndOfInput == m_lc.advance()) {
                JSON_ERROR(UNTERMINATED_STRING);
                m_state = Error; // unrecoverable
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
                        JSON_ERROR(FIELD_NO_VALUE);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    m_lc.trySkipWhiteSpace();
                    m_state = Field;
                    if(match && 0==(*sz))
                        return !hasError();
                } 
            } else {
                // nothing to be done if it fails - i/o error or badly formed document
                // can't recover our cursor
                if(!m_lc.trySkipUntil('\"','\\',true)) {
                    JSON_ERROR(UNTERMINATED_STRING);
                    m_state = Error;
                    return false;
                }
                m_lc.trySkipWhiteSpace();
                if(':'==m_lc.current()) { // it's a field. This is a match!
                    if(LexContext::EndOfInput==m_lc.advance()) {
                        JSON_ERROR(FIELD_NO_VALUE);
                        m_state = Error; // unrecoverable
                        return false;
                    }
                    m_lc.trySkipWhiteSpace();
                    m_state = Field;
                    return false;
                } else {
                    JSON_ERROR(FIELD_NO_VALUE);
                    return false;
                }
            }
            return false;
        }
        size_t scanMatchFields(const char** fields,size_t fieldCount) {
            size_t result = -1;
            const char* sz;
            char ch;
            if('\"'!=m_lc.current()) {
                JSON_ERROR(UNEXPECTED_VALUE);
                return -1;  
            }
            if (LexContext::EndOfInput == m_lc.advance()) {
                JSON_ERROR(UNTERMINATED_STRING);
                m_state = Error;
                return -1;
            }
            size_t fc = fieldCount;
            while(0<fc && LexContext::EndOfInput!=m_lc.current() && '\"'!=m_lc.current())
            {
                ch=undecorateInline();
                if(0==ch) 
                    break; // something went wrong  
                for(size_t i = 0;i<fieldCount;++i) {
                    sz = fields[i];  
                    if(sz) {
                        if(ch!=*sz) {
                            // disqualify this entry
                            fields[i]=nullptr;
                            --fc;
                            continue;
                        }
                        ++fields[i];
                    }
                }
            }
            if('\"'==m_lc.current()) {
                for(size_t i = 0;i<fieldCount;++i) {
                    if(fields[i] && 0==(*fields[i])) {
                        result = i;
                        break;
                    }
                }
                // if result != -1 there is potentially a match
                m_lc.advance();
                m_lc.trySkipWhiteSpace();
                if(':'==m_lc.current()) { // it's a field. This is a match!
                    if(LexContext::EndOfInput==m_lc.advance()) {
                        JSON_ERROR(FIELD_NO_VALUE);
                        m_state = Error; // unrecoverable
                        return -1;
                    }
                    m_lc.trySkipWhiteSpace();
                    m_state = Field;
                    // if we didn't match, skip past this field's value.
                    if(result==-1) {
                        if(!skipObjectOrArrayOrValuePart())
                            return -1;
                        if(!skipCommaOrEndObjectOrEndArray())
                            return -1;
                        if(EndObject!=m_state)
                            m_state = Field;
                        return -1;
                    }
                    return result;
                } else {
                    JSON_ERROR(FIELD_NO_VALUE);
                    return -1;
                }
            } else {
                // read to the end of the field so the reader isn't broken
                // nothing to be done if it fails - i/o error or badly formed document
                // can't recover our cursor
                if(!m_lc.trySkipUntil('\"','\\',true)) {
                    JSON_ERROR(UNTERMINATED_STRING);
                    return -1;
                }
                m_lc.trySkipWhiteSpace();
                if(m_lc.current()==':') { 
                    if(LexContext::EndOfInput==m_lc.advance()) {
                        JSON_ERROR(FIELD_NO_VALUE);
                        return -1;
                    }
                    m_lc.trySkipWhiteSpace();
                    // skip past the field's value
                    if(!skipObjectOrArrayOrValuePart())
                        return -1;
                    if(!skipCommaOrEndObjectOrEndArray())
                        return -1;
                    if(EndObject!=m_state)    
                        m_state = Field;
                    return -1;
                }
            }
            return -1;
        }
        bool scanMatchSiblings(const char* field) {
            while(true) {
                bool matched = scanMatchField(field);
                if(hasError()) 
                    return false;
                if(matched) {
                    return true;
                } else {
                    if(!skipObjectOrArrayOrValuePart())
                        return false;
                    if(!skipIfComma())
                        return false;
                }
            }
            return false;
        }
#ifdef HTCW_JSONTREE
	bool tryParseSubtreeImpl(MemoryPool& pool,JsonElement* pelem,bool skipFinalRead=false) {
            size_t c;
            JsonElement e;
            JsonElement* pse;
            switch (m_state)
            {
                case EndDocument:
                    JSON_ERROR(END_OF_DOCUMENT);
                    return false;
                case Initial:
                    if (!read()) {
                        JSON_ERROR(NO_DATA);
                        return false;
                    }
                    return tryParseSubtreeImpl(pool,pelem,skipFinalRead);
                case Value:
                    switch(valueType()) {
                        case Null:
                            e.null(nullptr);
                            break;

                        case Real:
                            e.real(realValue());
                            break;

                        case Integer:
                            e.integer(integerValue());
                            break;

                        break;
                        case Boolean:
                            e.boolean(booleanValue());
                            break;

                        case String:
                            if(!e.allocString(pool,value())) {
                                JSON_ERROR(OUT_OF_MEMORY);
                                return false;
                            }
                            break;
                    }
                    if(!skipFinalRead)
                        read();
                    else {
                        m_lc.trySkipWhiteSpace();
                        if(!skipIfComma())
                            return false;
                        m_state = Field;
                    }
                    break;
                case Field: 
                    // we have no structure with which to return a field
                    JSON_ERROR(FIELD_NOT_SUPPORTED);
                    return false;

                case Array:
                    e.parray(nullptr);
                    if(!read())
                        return false;
                    while (EndArray != m_state) {
                        JsonElement je;
                        JsonElement* pje = (JsonElement*)pool.alloc(sizeof(JsonElement));
                        *pje=je;
                        //printf("Before pool used: %d\r\n",(int)pool.used());
                        if(false==(tryParseSubtreeImpl(pool,pje)) || !e.addItem(pool,pje)) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }
                        //printf("After pool used: %d\r\n",(int)pool.used());
                    }
                    if(!skipFinalRead) {
                        if(EndArray==m_state && !read()) 
                            return false;
                    } else {
                        if(!skipIfComma())
                            return false;
                    }
                    break;
                case EndArray: 
                case EndObject:
                    // we have no data to return
                    JSON_ERROR(NO_DATA);
                    return false;
                case Object:// begin object
                    e.pobject(nullptr);
                    if(!read()) {
                        return false;
                    }
                    //printf("Before pool used: %d\r\n",(int)pool.used());
                    while (!hasError() && EndObject != m_state) {
                        c=strlen(value())+1;
                        char *fn=(char*)pool.alloc(c);
                        if(nullptr==fn) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }
                        strcpy(fn,value());
                        if(!read()) {
                            JSON_ERROR(FIELD_NO_VALUE);
                            return false;
                        }
                        JsonElement je;
                        JsonElement* pje = (JsonElement*)pool.alloc(sizeof(JsonElement));
                        *pje=je;
                        if(nullptr==fn || false==(tryParseSubtreeImpl(pool,pje)) || !e.addFieldPooled(pool,fn,pje)) {
                            JSON_ERROR(OUT_OF_MEMORY);
                            return false;
                        }	
                    }
                    //printf("Before after used: %d\r\n",(int)pool.used());
                    if(!skipFinalRead) {
                        if(EndObject==m_state && !read()) 
                            if(EndDocument!=m_state)
                                return false;
                    } else {
                        if(!skipIfComma())
                            return false;
                    }
                    break;
                default:
                    JSON_ERROR(UNKNOWN_STATE);
                    return false;
            }
            if(!e.undefined()) {
                *pelem= e;
                return true;
            }
            return false;
        }
        bool extractImpl(MemoryPool& pool,JsonExtractor& extraction,bool subcall) {
            if(0==extraction.count) {
                if(m_state==Object || m_state==Value || m_state==Array || m_state==Initial) {
                    if(!tryParseSubtreeImpl(pool,extraction.presult,subcall))
                        return false;
                    return true;
                }
                return false;
            }
            size_t matched;
            size_t c;
            size_t idx;
            unsigned long int depth;
            switch(m_state) {
                case JsonReader::Initial:
                    if(!read())
                        return false;
                    return extractImpl(pool,extraction,false);
                case JsonReader::Array:
                    if(extraction.pindices==nullptr) {
                        // we're not on an array extraction.
                        return false;
                    }
                    c= extraction.count;
                    idx = 0;
                    depth = m_objectDepth;
                    while(!hasError() && m_state!=EndArray) {
                        matched = -1;
                        for(size_t i = 0;i<extraction.count;++i) {
                            if(extraction.pindices[i]==idx) {
                                matched = i;
                                break;
                            }
                        }
                        if(-1!=matched) {
                            if(nullptr!=extraction.pchildren) {
                                if(!read())
                                    return false;
                                if(EndArray==m_state)
                                    break;
                                // extract the array value
                                if(!extractImpl(pool,extraction.pchildren[matched],true))
                                    return false;
                            } else {
                                JSON_ERROR(INVALID_ARGUMENT);
                                if(!skipToEndArray())
                                    m_state = Error; // can't recover unless we have a known good location
                                return false;
                            }
                            --c;
                            if(0==c) {
                                if(!skipToEndArray()) {
                                    return false;
                                }
                                break;  
                            }    
                        } else {
                            if(!skipObjectOrArrayOrValuePart())
                                return false;
                            if(!skipIfComma())
                                return false;
                            ++idx;    
                        }
                        
                    }
                    if(m_state!=EndArray)
                        return false;
                    skipIfComma();
                    return !hasError();
                case JsonReader::Object:
                    if(extraction.pfields==nullptr) {
                        // we're not on an object extraction.
                        return false;
                    }
                    matched=-1;
                    c= extraction.count;
                    // prepare our allocated space by copying our field pointers
                    depth = m_objectDepth;
                    while(!hasError() && (m_state!=EndObject || (m_objectDepth>depth-1))) {
                        // we need space for extra string pointers for scanMatchFields()
                        // so if we haven't already, allocate it
                        if(nullptr==extraction.palloced) {
                            extraction.palloced = (const void**)pool.alloc(extraction.count*sizeof(char*));
                            if(nullptr==extraction.palloced) {
                                JSON_ERROR(OUT_OF_MEMORY);
                                return false;    
                            }
                        }
                        memcpy(extraction.palloced,extraction.pfields,extraction.count*sizeof(char*));
                        matched = scanMatchFields((const char**)extraction.palloced,extraction.count);
                        pool.unalloc(extraction.count*sizeof(char*));
                        extraction.palloced = nullptr;
                        if(-1!=matched) {
                            if(nullptr!=extraction.pchildren) {
                                // read to the field value
                                if(!read()) {
                                    return false;
                                }
                                if(!this->extractImpl(pool,extraction.pchildren[matched],true))
                                        return false;
                                if(EndObject==m_state)
                                    {
                                        skipIfComma();
                                        m_state=Value;
                                    }
                            }
                            --c;
                            if(0==c) {
                                if(!this->skipToEndObject()) {
                                    return false;
                                }
                                break;  
                            }    
                        } 
                    }
                    if(m_state!=EndObject)
                        return false;
                    skipIfComma();
                    return !hasError();
            }
            return false;
        }

#endif
        JsonReader(const LexContext& rhs) = delete;
        JsonReader(LexContext&& rhs) = delete;
        JsonReader& operator=(const LexContext& rhs) = delete;
        JsonReader& operator=(LexContext&& rhs) = delete;
    public:
        JsonReader(LexContext& lexContext) : m_state(Initial),m_lc(lexContext),m_objectDepth(0) {
        
        }
        ~JsonReader() {

        }

        void reset(LexContext& lexContext) {
            m_state = Initial;
            m_lc = lexContext;
            m_objectDepth = 0;
        }
        bool read() {
            if(Error==m_state)
                return false;
            if(LexContext::EndOfInput==m_lc.current()) {
                m_state = EndDocument;
                return false;
            }
            clearError();
            m_valueType = Undefined;
            switch(m_state) {
                case Initial:
                    m_objectDepth=0;
                    m_lc.ensureStarted();
                    m_lc.trySkipWhiteSpace();
                    if(!readAnyOpen(false))
                        return false;
                    break;

                case Value:
                    if(!readAny())
                        return false;
                    break;

                case Object:
                    if(!readFieldOrEndObject())
                        return false;
                    break;

                case Field:
                    // handle field special by not breaking (doesn't change anything in current code)
                    return readAnyOpen(false);
                
                case EndObject:
                    if(!readAny())
                        return false;
                    break;

                case Array:
                    if(!readValueOrEndArray())
                        return false;
                    break;

                case EndArray:
                    if(!readAny())
                        return false;
                    break;

                case EndDocument:
                case Error:
                    return false;
            }
            return true;
        }
#ifdef HTCW_JSONTREE
        bool parseSubtree(MemoryPool& pool,JsonElement* pelem) {
            if(nullptr==pelem) {
                JSON_ERROR(INVALID_ARGUMENT);
                return false;
            }
            if(tryParseSubtreeImpl(pool,pelem))
                return true;
            return false;
        }
        bool extract(MemoryPool& pool,JsonExtractor& extraction) {
            return extractImpl(pool,extraction,false);
        }
#endif
        // skips the substree under the cursor
        bool skipSubtree()
        {
            clearError();
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
                if(!skipObjectOrArrayOrValuePart())
                    return false;
                if(!read() || Error==m_state)
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
            }
            return false;
        }
        bool skipToFieldValue(const char* field, int8_t axis, unsigned long int *pdepth = nullptr) {
            return skipToField(field,axis,pdepth) && read();
        }
        bool skipToField(const char* field, int8_t axis, unsigned long int *pdepth = nullptr) {
            clearError();
            if(!field) {
                JSON_ERROR(INVALID_ARGUMENT);
                return false;
            }
            bool match;
            char ch;
            const char*sz;
            switch(axis) {
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
                        if(hasError())
                            return false;
                        break;

                    case Field:
                        if(!skipObjectOrArrayOrValuePart())
                            return false;
                        if(!skipIfComma())
                            return false;

                        if(scanMatchSiblings(field)) {
                            return true;
                        }
                        if(hasError())
                            return false;
                        break;
                } 
                return false;
            break;
            case Descendants:
                if(nullptr==pdepth) {
                    JSON_ERROR(INVALID_ARGUMENT);
                    return false;
                }
                if(Initial==m_state) {
                    if(!read())
                        return false;
                }
            
                if(0==(*pdepth))
                    *pdepth=1;
                // fall through
            case Forward:
                while (LexContext::EndOfInput != m_lc.current()) {
                    switch (m_lc.current()) {
                        case '\"':
                            if (LexContext::EndOfInput == m_lc.advance()) {
                                JSON_ERROR(UNTERMINATED_STRING);
                                m_state = Error; // unrecoverable
                                return false;
                            }
                            match = true;
                            sz=field;
                            while(m_lc.current()!=LexContext::EndOfInput && m_lc.current()!='\"')
                            {
                                if(0==(*sz))
                                    break;
                                ch=undecorateInline();
                                if(0==ch) 
                                    break;
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
                                if(':'==m_lc.current()) { // it's a field. This is a match!
                                    if(LexContext::EndOfInput==m_lc.advance()) {
                                        JSON_ERROR(FIELD_NO_VALUE);
                                        m_state = Error; // unrecoverable
                                        return false;
                                    }
                                    m_lc.trySkipWhiteSpace();
                                    m_state = Field;
                                    if(match && !(*sz))
                                        return !hasError();
                                } 
                            } else if(!m_lc.trySkipUntil('\"','\\',true)) {
                                JSON_ERROR(UNTERMINATED_STRING);
                                m_state = Error; // unrecoverable
                                return false;
                            }
                            break;

                        case '{':
                            if(LexContext::EndOfInput==m_lc.advance()) {
                                JSON_ERROR(UNTERMINATED_OBJECT);
                                m_state = Error; // unrecoverable
                                return false;
                            }
                            if(pdepth)
                                ++(*pdepth);
                            ++m_objectDepth;
                            break;

                        case '[':
                            if(LexContext::EndOfInput==m_lc.advance()) {
                                JSON_ERROR(UNTERMINATED_ARRAY);
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
                    JSON_ERROR(INVALID_ARGUMENT);
                    return false;
                
            }
            return false;
        }
        bool skipToIndex(size_t index) {
            clearError();
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
                        if (!skipObjectOrArrayOrValuePart())
                            return false;
                        if(!skipIfComma())
                            return false;
                    }
                    if(!read())
                        return false;
                }
                return true;
            }
            return false;
        }
        bool skipToEndObject() {
            clearError();
            if(!skipObjectPart(0))
                return false;
            return true;
        }
        bool skipToEndArray() {
            clearError();
            if(m_state==Error) return false;
            if(!skipArrayPart(0))
                return false;
            return true;
        }
        bool hasError() {
            return Error==m_state || m_lastError!=JSON_ERROR_NO_ERROR;
        }       
        uint8_t lastError() {
            return m_lastError;
        }
        LexContext& context() { 
            return m_lc; 
        }
         
        unsigned long int objectDepth() { return m_objectDepth;}
        int8_t nodeType() {
            if(hasError()) return Error;
            return m_state;
        }
        int8_t valueType() {
            switch(m_state) {
            case Field:
            case Value:
            case Error:
                return m_valueType;
            }
            return JsonReader::Undefined;
        }
        char* value() {
            switch(nodeType()) {
                case Error:
                case Value:
                case Field:
                    return m_lc.captureBuffer();
            }
            return nullptr;
        }
        double realValue() {
            if(Value!=m_state || (Real!=m_valueType && Integer!=m_valueType))
                return NAN;
            return strtod(m_lc.captureBuffer(),nullptr);
        }
        long long int integerValue() {
            if(Value!=m_state)
                return 0;
            switch(m_valueType) {
                case Integer:
#ifndef ARDUINO
                    return atoll(m_lc.captureBuffer());
#else
                    return atol(m_lc.captureBuffer());
#endif

                case Real:
                    return (long long int)(strtod(m_lc.captureBuffer(),nullptr)+.5);
            }
            return 0;
        }
        bool booleanValue() {
            return Value==m_state && 
                m_valueType==Boolean && 
                nullptr!=m_lc.captureBuffer() && 
                't'==*m_lc.captureBuffer();
        }
    };
}
