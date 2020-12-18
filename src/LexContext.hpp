#ifdef _MSC_VER
#pragma once
#endif
#ifndef HTCW_LEXCONTEXT_HPP
#define HTCW_LEXCONTEXT_HPP
#include <cstdint>
#include <ctype.h>
#include <stdlib.h>
namespace lex {
  // represents a cursor and capture buffer over an input source
  class LexContext {
      int16_t m_current;
      unsigned long int m_line;
      unsigned long int m_column;
      unsigned long long m_position;
    protected:
      // reads a character from the underlying device
      // read() should return EndOfInput if no more data is available,
      // and Closed if the underlying source has been closed or otherwise
      // invalidated, where applicable
      virtual int16_t read()=0;
      // called to initialize the capture buffer
      virtual void initCapture()=0;
    public:
      // Represents the tab width of an input device
      static const uint8_t TabWidth = 4;
      // Represents the end of input symbol
      static const int8_t EndOfInput = -1;
      // Represents the before input symbol
      static const int8_t BeforeInput = -2;
      // Represents the closed symbol
      static const int8_t Closed = -3;

      // indicates the capacity of the capture buffer
      virtual size_t captureCapacity()=0;
      
      // gets the current character under the cursor
      int16_t current() const {
        return m_current;
      }
      // indicates the one based line the cursor is on
      unsigned long int line() const {
        return m_line;
      }
      // indicates the one based column the cursor is on
      unsigned long int column() const {
        return m_column;
      }
      // indicates the zero based position of the cursor
      unsigned long long position() const {
        return m_position;
      }
      void reset() {
        m_current = BeforeInput;
        setLocation(1, 0, 0);
        clearCapture();
      }
      // constructs a new instance
      LexContext() {
        
        m_current = BeforeInput;
        setLocation(1, 0, 0);
        
      }
      // sets the new location info for the cursor
      void setLocation(uint32_t line, uint32_t column, uint64_t position) {
        m_line = line;
        m_column = column;
        m_position = position;
      }
      // ensures that advance() has been called at least once
      bool ensureStarted() {
        if(Closed==m_current)
          return false;
        if (BeforeInput == m_current)
          advance();
        return true;
      }
      // advances the cursor by one and returns the next value
      int16_t advance() {
        if (EndOfInput == m_current)
          return EndOfInput;
        int16_t d=read();
        if (-1 == d)
          m_current = EndOfInput;
        else
          m_current = (int16_t)d;

        switch (m_current) {
          case '\n':
            ++m_line;
            m_column = 0;
            break;
          case '\r':
            ++m_column = 0;
            break;
          case '\t':
            m_column += TabWidth;
            break;
          default:
            ++m_column;
            break;
        }

        ++m_position;
        return m_current;
      }
      // gets the count of characters in the capture buffer, not counting the trailing NULL
      virtual size_t captureCount() const=0; 
      // retrieves the capture buffer pointer
      virtual char* captureBuffer() =0;
      // captures the character under the cursor if any. If false is returned, there was no more room
      virtual bool capture()=0;
      // clears the capture buffer
      virtual void clearCapture()=0;
      //
      // Base Extensions
      //
      // reads whitespace into the capture buffer
      // returns true if whitespace was read, false
      // if not or if the capture ran out of room
      bool tryReadWhiteSpace()
      {
        if(!ensureStarted())
          return false;
        if (EndOfInput == m_current || !isspace((char)m_current))
          return false;
        if (!capture())
          return false;
        while (EndOfInput != advance() && isspace((char)m_current))
          if (!capture()) return false;
        return true;
      }
      // skips over whitespace
      // returns true if whitespace was skipped
      bool trySkipWhiteSpace()
      {
        if(!ensureStarted())
          return false;
        if (EndOfInput == m_current || !isspace((char)m_current))
          return false;
        while (EndOfInput != advance() && isspace((char)m_current));
        return true;
      }
      // reads into the capture buffer until the specified character was found
      // optionally capturing it. Returns true if the character was found, false
      // if not or if the capture ran out of room.
      bool tryReadUntil(int16_t character, bool readCharacter = true)
      {
        if(!ensureStarted())
          return false;
        if (0 > character) character = EndOfInput;
        if (!capture()) return false;
        if (m_current == character) {
          return true;
        }

        while (-1 != advance() && m_current != character) {
          if (!capture()) return false;
        }
        //
        if (m_current == character) {
          if (readCharacter) {
            if (!capture())
              return false;
            advance();
          }
          return true;
        }
        return false;
      }
      // skips until the specified character was found
      // optionally skipping it. Returns true if the character was found, false
      // if not
      bool trySkipUntil(int16_t character, bool skipCharacter = true)
      {
        if(!ensureStarted())
          return false;
        if (0 > character) character = -1;
        if (m_current == character)
          return true;
        while (EndOfInput != advance() && m_current != character) ;
        if (m_current == character)
        {
          if (skipCharacter)
            advance();
          return true;
        }
        return false;
      }
      // reads into the capture buffer until the specified character was found
      // optionally capturing it and skipping escapes. Returns true if the 
      // character was found, false if not or if the capture ran out of room.
      bool tryReadUntil(int16_t character, int16_t escapeChar, bool readCharacter = true)
      {
        if(!ensureStarted())
          return false;
        if (0 > character) character = EndOfInput;
        if (EndOfInput == m_current) {
          return false;
        }
        if (m_current == character)
        {
          if (readCharacter)
          {
            if (!capture())
              return false;
            advance();
          }
          return true;
        }

        do
        {
          if (escapeChar == m_current)
          {
            if (!capture())
              return false;
            if (EndOfInput == advance())
              return false;
            if (!capture())
              return false;
          }
          else
          {
            if (character == m_current)
            {
              if (readCharacter)
              {
                if (!capture())
                  return false;
                advance();
              }
              return true;
            }
            else if (!capture())
              return false;
          }
        }
        while (EndOfInput != advance());

        return false;
      }
      // skips until the specified character was found
      // optionally skipping it and skipping escapes. Returns true if the 
      // character was found, false if not
      bool trySkipUntil(int16_t character, int16_t escapeChar, bool skipCharacter = true)
      {
        if(!ensureStarted())
          return false;
        if (0 > character) character = EndOfInput;
        if (m_current == character) {
          if(skipCharacter)
            advance();
          return true;
        }
        while (EndOfInput != advance() && m_current != character)
        {
          if (m_current == escapeChar)
            if (EndOfInput == advance())
              break;
        }
        if (m_current == character)
        {
          if (skipCharacter)
            advance();
          return true;
        }
        return false;
      }
  };
  // represents a LexContext with a fixed size buffer
  template<const size_t TCapacity> class StaticLexContext : virtual public LexContext {
    static_assert(0<TCapacity,
                  "StaticLexContext requires a postive value for TCapacity");
    char m_capture[TCapacity];
    size_t m_captureCount;
    protected:
      // initializes the capture buffer
      void initCapture() override {
        *m_capture=0;
        m_captureCount=0;

      }
    public:
      StaticLexContext() {
            
      }
      // the capacity of the capture buffer (not including the trailing NULL)
      size_t captureCapacity() override { return TCapacity-1; }
      // the count of characters in the capture buffer
      size_t captureCount() const override {return m_captureCount;}
      // returns a pointer to the capture buffer
      char* captureBuffer() override {return m_capture;}
      // clears the capture buffer
      void clearCapture() override {
        m_captureCount = 0;
        *m_capture=0;
      }
      // captures the character under the cursor if any
      // returns true if a character was captured.
      bool capture() override {
        if (Closed!=current() && EndOfInput != current() && BeforeInput != current() && (TCapacity - 1) > m_captureCount)
        {
          m_capture[m_captureCount++] = (uint8_t)current();
          m_capture[m_captureCount] = 0;
          return true;
        }
        return false;
      }

  };
  
  class SZLexContext : virtual public LexContext {
    char* m_sz;
    protected:
      int16_t read() override {
          if(!m_sz)
            return LexContext::Closed;
          char ch = *m_sz;
          if(!ch)
            return LexContext::EndOfInput;
          ++m_sz;
          return ch;
      }
    public:
      bool attach(char* sz) {
        if(!sz)
          return false;
        this->initCapture();
        m_sz=sz;
        return true;
      }
  };
  template<const size_t TCapacity> class StaticSZLexContext : public StaticLexContext<TCapacity>, public SZLexContext {

  };
  
#ifdef ARDUINO
  // represents a fixed length LexContext for the Arduino built on the Arduino SDK's Stream class
  template<const size_t TCapacity> class ArduinoLexContext : public StaticLexContext<TCapacity> { 
        Stream* m_pstream;
    protected:
        // reads a character from the stream
        int16_t read() override {
            if(!m_pstream)
              return LexContext::Closed;
            return m_pstream->read();
        }
    public:
      ArduinoLexContext() {
            
      }
        // initializes the lexcontext with a stream
        bool begin(Stream& stream) {
            m_pstream = &stream;
            // this-> is required to help the compiler find the method
            this->initCapture();
            return true;
        }
    };
#endif
}
#endif