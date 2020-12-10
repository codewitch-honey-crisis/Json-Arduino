#ifndef HTCW_LEXCONTEXT_H
#define HTCW_LEXCONTEXT_H
#include <stdlib.h>

template<const size_t S> class LexContext {
    Stream *_pstream;
    char _capture[S];
    uint32_t _captureCount;
    int16_t _current;
    uint32_t _line;
    uint32_t _column;
    uint64_t _position;

  public:
    // Represents the tab width of an input device
    static const uint8_t TabWidth = 4;
    // Represents the end of input symbol
    static const int8_t EndOfInput = -1;
    // Represents the before input symbol
    static const int8_t BeforeInput = -2;
    // Represents a symbol for the disposed state
    static const int8_t Closed = -3;

    int16_t current() const {
      return _current;
    }
    uint32_t line() const {
      return _line;
    }
    uint32_t column() const {
      return _column;
    }
    uint64_t position() const {
      return _position;
    }

    LexContext() {
      _pstream = NULL;
    }

    bool begin(Stream& stream) {
      memset(_capture, 0, S);
      _captureCount = 0;
      _current = BeforeInput;
      _pstream = &stream;
      setLocation(1, 0, 0);
      return true;
    }
    void setLocation(uint32_t line, uint32_t column, uint64_t position) {
      _line = line;
      _column = column;
      _position = position;
    }


    bool ensureStarted() {
      if (!_pstream) return false;
      if (BeforeInput == _current)
        advance();
    }

    int16_t advance() {
      if (!_pstream) return Closed;
      if (EndOfInput == _current)
        return EndOfInput;
      int d = _pstream->read();
      if (-1 == d)
        _current = EndOfInput;
      else
        _current = (int16_t)d;

      switch (_current) {
        case '\n':
          ++_line;
          _column = 0;
          break;
        case '\r':
          ++_column = 0;
          break;
        case '\t':
          _column += TabWidth;
          break;
        default:
          ++_column;
          break;
      }

      ++_position;
      return _current;
    }

    size_t captureCount() const {
      return _captureCount;
    }
    void setCaptureCount(size_t size) {
			if(size>S-1) return false;
			_capture[size]=0;
			_captureCount = size;
    }
    size_t captureMax() const {
      return S;
    }
    char* captureBuffer() {
      return _capture;
    }
    bool capture() {
      if (_pstream && EndOfInput != _current && BeforeInput != _current && (S - 1) > _captureCount)
      {
        _capture[_captureCount++] = (uint8_t)_current;
        _capture[_captureCount] = 0;
        return true;
      }
      return false;
    }

    void clearCapture() {
      _captureCount = 0;
    }
    void zeroCapture() {
      memset(_capture, 0, _captureCount);
    }

    //
    // Base Extensions
    //
    bool tryReadWhiteSpace()
    {
      ensureStarted();
      if (EndOfInput == _current || !isspace((char)_current))
        return false;
      if (!capture())
        return false;
      while (EndOfInput != advance() && isspace((char)_current))
        if (!capture()) return false;
      return true;
    }

    bool trySkipWhiteSpace()
    {
      ensureStarted();
      if (EndOfInput == _current || !isspace((char)_current))
        return false;
      while (EndOfInput != advance() && isspace((char)_current));
      return true;
    }
    bool tryReadUntil(int16_t character, bool readCharacter = true)
    {
      ensureStarted();
      if (0 > character) character = EndOfInput;
      if (!capture()) return false;
      if (_current == character) {
        return true;
      }

      while (-1 != advance() && _current != character) {
        if (!capture()) return false;
      }
      //
      if (_current == character) {
        if (readCharacter) {
          if (!capture())
            return false;
          advance();
        }
        return true;
      }
      return false;
    }
    bool trySkipUntil(int16_t character, bool skipCharacter = true)
    {
      ensureStarted();
      if (0 > character) character = -1;
      if (_current == character)
        return true;
      while (EndOfInput != advance() && _current != character) ;
      if (_current == character)
      {
        if (skipCharacter)
          advance();
        return true;
      }
      return false;
    }
    bool tryReadUntil(int16_t character, int16_t escapeChar, bool readCharacter = true)
    {
      ensureStarted();
      if (0 > character) character = EndOfInput;
      if (EndOfInput == _current) return false;
      if (_current == character)
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
        if (escapeChar == _current)
        {
          if (!capture())
            return false;
          if (-1 == advance())
            return false;
          if (!capture())
            return false;
        }
        else
        {
          if (character == _current)
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
    bool trySkipUntil(int16_t character, int16_t escapeChar, bool skipCharacter = true)
    {
      ensureStarted();
      if (0 > character) character = EndOfInput;
      if (_current == character)
        return true;
      while (EndOfInput != advance() && _current != character)
      {
        if (character == escapeChar)
          if (EndOfInput == advance())
            break;
      }
      if (_current == character)
      {
        if (skipCharacter)
          advance();
        return true;
      }
      return false;
    }
};
#endif HTCW_LEXCONTEXT_H
