#ifndef HTCW_JSON_H
#define HTCW_JSON_H
#include "LexContext.h"
template<size_t S> class JsonReader {
  public:
    static const int8_t Error = -3;
    static const int8_t EndDocument = -2;
    static const int8_t Initial = -1;
    static const int8_t Value = 0;
    static const int8_t Key = 1;
    static const int8_t Array = 2;
    static const int8_t EndArray = 3;
    static const int8_t Object = 4;
    static const int8_t EndObject = 5;
    static const int8_t String = 6;
    static const int8_t Number = 7;
    static const int8_t Boolean = 8;
    static const int8_t Null = 9;
    
  private:
    LexContext<S> _lc;
    int8_t _state;
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
#ifdef JSON_CANONICAL_SKIP
    // optimization
    void skipObjectPart()
    {
      int depth = 1;
      while (Error!=_state && LexContext<S>::EndOfInput != _lc.current())
      {
        switch (_lc.current())
        {
          case '[':
            skipArrayPart();
            break;
            
          case '{':
            ++depth;
            _lc.advance();
            if(LexContext<S>::EndOfInput==_lc.current())
              _state = Error;
            break;
          case '\"':
            skipString();
            break;
          case '}':
            --depth;
            _lc.advance();
            if (depth == 0)
            {
              return;
            }
            if(LexContext<S>::EndOfInput==_lc.current())
              _state = Error;
            break;
          default:
            _lc.advance();
            break;
        }
      }
    }

    void skipArrayPart()
    {
      int depth = 1;
      while (Error!=_state && LexContext<S>::EndOfInput != _lc.current())
      {
        switch (_lc.current())
        {
          case '{':
            skipObjectPart();
            break;
          case '[':
            ++depth;
            if(LexContext<S>::EndOfInput==_lc.advance()) {
              _state = Error;
              return;
            }
            break;
          case '\"':
            skipString();
            break;
          case ']':
            --depth;
            _lc.advance();
            if (depth == 0)
            {
              return;
            }
            if(LexContext<S>::EndOfInput==_lc.current())
              _state = Error;
            break;
          default:
            _lc.advance();
            break;
        }
      }
    }
#else
    void skipPart()
    {
      int depth = 1;
      while (Error!=_state && LexContext<S>::EndOfInput != _lc.current())
      {
        switch (_lc.current())
        {
          case '[':
          case '{':
            ++depth;
            _lc.advance();
            if(LexContext<S>::EndOfInput==_lc.current())
              _state = Error;
            break;
          case '\"':
            skipString();
            break;
          case '}':
          case ']':
            --depth;
            _lc.advance();
            if (depth == 0)
            {
              return;
            }
            if(LexContext<S>::EndOfInput==_lc.current())
              _state = Error;
            break;
          default:
            _lc.advance();
            break;
        }
      }
    }
#endif
    void skipString()
    {
      if('\"'!=_lc.current()) {
         _state = Error;
         return;
      }
      _lc.advance();
      _lc.trySkipUntil('\"', false);
      if('\"'!=_lc.current()) {
         _state = Error;
         return;
      }
      _lc.advance();
    }

  public:

    JsonReader() {
    }
    bool begin(Stream &stream) {
      _state = Initial;
      _lc.begin(stream);
    }
    int8_t nodeType() {
      return _state;
    }
    int32_t line() {
      return _lc.line();
    }
    int32_t column() {
      return _lc.column();
    }
    int64_t position() {
      return _lc.position();
    }

    bool read() {
      int16_t qc;
      int16_t ch;
      switch (_state) {
        case JsonReader<S>::Error:
        case JsonReader<S>::EndDocument:
          return false;
        case JsonReader<S>::Initial:
          _lc.ensureStarted();
          _state = Value;
        // fall through
        case JsonReader<S>::Value:
value_case:
          _lc.clearCapture();
          switch (_lc.current()) {
            case LexContext<S>::EndOfInput:
              _state = EndDocument;
              return false;
            case ']':
              _lc.advance();
              _lc.trySkipWhiteSpace();
              _lc.clearCapture();
              _state = EndArray;
              return true;
            case '}':
              _lc.advance();
              _lc.trySkipWhiteSpace();
              _lc.clearCapture();
              _state = EndObject;
              return true;
            case ',':
              _lc.advance();
              _lc.trySkipWhiteSpace();
              if (!read()) { // read the next value
                _state = Error;
              }
              return true;
            case '[':
              _lc.advance();
              _lc.trySkipWhiteSpace();
              _state = Array;
              return true;
            case '{':
              _lc.advance();
              _lc.trySkipWhiteSpace();
              _state = Object;
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
              qc = _lc.current();
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              while (LexContext<S>::EndOfInput != _lc.advance() &&
                     ('E' == _lc.current() ||
                      'e' == _lc.current() ||
                      '+' == _lc.current() ||
                      '.' == _lc.current() ||
                      isdigit((char)_lc.current()))) {
                if (!_lc.capture()) {
                  _state = Error;
                  return true;
                }
              }
              _lc.trySkipWhiteSpace();
              return true;
            case '\"':
              _lc.capture();
              _lc.advance();
              _lc.tryReadUntil('\"', '\\', true);
              _lc.trySkipWhiteSpace();
              if (':' == _lc.current())
              {
                _lc.advance();
                _lc.trySkipWhiteSpace();
                if (LexContext<S>::EndOfInput == _lc.current()) {
                  _state = Error;
                  return true;
                }
                _state = Key;
              }
              return true;
            case 't':
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('r' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('u' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('e' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              _lc.advance();
              _lc.trySkipWhiteSpace();
              ch = _lc.current();
              if (',' != ch && ']' != ch && '}' != ch && -1 != ch) {
                _state = Error;
              }
              return true;
            case 'f':
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('a' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('l' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('s' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('e' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              _lc.advance();
              _lc.trySkipWhiteSpace();
              ch = _lc.current();
              if (',' != ch && ']' != ch && '}' != ch && -1 != ch) {
                _state = Error;
              }
              return true;
            case 'n':
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('u' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('l' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              if ('l' != _lc.advance()) {
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _state = Error;
                return true;
              }
              _lc.advance();
              _lc.trySkipWhiteSpace();
              ch = _lc.current();
              if (',' != ch && ']' != ch && '}' != ch && -1 != ch) {
                _state = Error;
              }
              return true;
            default:
              _state = Error;
              return true;

          }
        default:
          _state = Value;
          goto value_case;
      }
    }
    bool skipSubtree()
    {
      switch (_state)
      {
        case JsonReader<S>::Error:
          return false;
        case JsonReader<S>::EndDocument: // eos
          return false;
        case JsonReader<S>::Initial: // initial
          if (read())
            return skipSubtree();
          return false;
        case JsonReader<S>::Value: // value
          return true;
        case JsonReader<S>::Key: // key
          if (!read())
            return false;
          return skipSubtree();
        case JsonReader<S>::Array:// begin array
#ifdef JSON_CANONICAL_SKIP
          skipArrayPart();
#else
          skipPart();
#endif
          _lc.trySkipWhiteSpace();
          _state = EndArray; // end array
          return true;
        case JsonReader<S>::EndArray: // end array
          return true;
        case JsonReader<S>::Object:// begin object
#ifdef JSON_CANONICAL_SKIP
          skipObjectPart();
#else
          skipPart();
#endif
          _lc.trySkipWhiteSpace();
          _state = EndObject; // end object
          return true;
        case JsonReader<S>::EndObject: // end object
          return true;
        default:
          _state = Error;
          return true;
      }
    }
    bool skipToIndex(int index) {
      if (Initial==_state || Key == _state) // initial or key
        if (!read())
          return false;
      if (Array==_state) { // array start
        if (0 == index) {
          if (!read())
            return false;
        }
        else {
          for (int i = 0; i < index; ++i) {
            if (EndArray == _state) // end array
              return false;
            if (!read())
              return false;
            if (!skipSubtree())
              return false;
          }
          if ((EndObject==_state || EndArray==_state) && !read())
            return false;
        }
        return true;
      }
      return false;
    }
    bool skipToField(const char* key, bool searchDescendants = false) {
      if (searchDescendants) {
        while (read()) {
          if (Key == _state) { // key 
            undecorate();
            if (!strcmp(key , value()))
              return true;
          }
        }
        return false;
      }
      switch (_state)
      {
        case -1:
          if (read())
            return skipToField(key);
          return false;
        case 4:
          while (read() && Key == _state) { // first read will move to the child field of the root
            undecorate();
            if (strcmp(key,value()))
              skipSubtree(); // if this field isn't the target so just skip over the rest of it
            else
              break;
          }
          return Key == _state;
        case 1: // we're already on a field
          undecorate();
          if (!strcmp(key,value()))
            return true;
          else if (!skipSubtree())
            return false;

          while (read() && Key == _state) { // first read will move to the child field of the root
            undecorate();
            if (strcmp(key , value()))
              skipSubtree(); // if this field isn't the target just skip over the rest of it
            else
              break;
          }
          return Key == _state;
        default:
          return false;
      }
    }
    int8_t valueType() {
      char *sz = _lc.captureBuffer();
      char ch = *sz;
      if('\"'==ch)
        return String;
      if('t'==ch || 'f'==ch)
        return Boolean;
      if('n'==ch)
        return Null;
      return Number;
    }
    bool booleanValue() {
      return 't'==*_lc.captureBuffer();
    }
    double numericValue() {
      return strtod(_lc.captureBuffer(),NULL);
    }
    void undecorate() {
      char *src = _lc.captureBuffer();
      char *dst = src;
      char ch = *src;
      if ('\"' != ch)
        return;
      ++src;
      uint16_t uu;
      while ((ch = *src) && ch != '\"') {
        switch (ch) {
          case '\\':
            ch = *(++src);

            switch (ch) {
              case '\'':
              case '\"':
              case '\\':
              case '/':
                *(dst++) = ch;
                ++src;
                break;
              case 'r':
                *(dst++) = '\r';
                ++src;
                break;
              case 'n':
                *(dst++) = '\n';
                ++src;
                break;
              case 't':
                *(dst++) = '\t';
                ++src;
                break;
              case 'b':
                *(dst++) = '\b';
                ++src;
                break;
              case 'u':
                uu = 0;
                ch = *(++src);
                if (isHexChar(ch)) {
                  uu = fromHexChar(ch);
                  ch = *(++src);
                  uu *= 16;
                  if (isHexChar(ch)) {
                    uu |= fromHexChar(ch);
                    ch = *(++src);
                    uu *= 16;
                    if (isHexChar(ch)) {
                      uu |= fromHexChar(ch);
                      ch = *(++src);
                      uu *= 16;
                      if (isHexChar(ch)) {
                        uu |= fromHexChar(ch);
                        ch = *(++src);
                      }
                    }
                  }
                }
                if (0 < uu) {
                  // no unicode
                  if (256 > uu) {
                    *(dst++) = (char)uu;
                  } else
                    *(dst++) = '?';
                }
            }
            break;
          default:
            *dst = ch;
            ++dst;
            ++src;
        }

      }
      *dst = 0;
    }
    char* value() {
      switch (_state) {
        case JsonReader<S>::Key:
        case JsonReader<S>::Value:
          return _lc.captureBuffer();
      }
      return NULL;
    }
};
#endif
