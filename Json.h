#ifndef HTCW_JSON_H
#define HTCW_JSON_H
#include "LexContext.h"
#define JSON_ERROR_NO_ERROR 0
#define JSON_ERROR_UNTERMINATED_OBJECT 1
char JSON_ERROR_UNTERMINATED_OBJECT_MSG[] = PROGMEM "Unterminated object";
#define JSON_ERROR_UNTERMINATED_ARRAY 2
char JSON_ERROR_UNTERMINATED_ARRAY_MSG[] = PROGMEM "Unterminated array";
#define JSON_ERROR_UNTERMINATED_STRING 3
char JSON_ERROR_UNTERMINATED_STRING_MSG[] = PROGMEM "Unterminated string";
#define JSON_ERROR_FIELD_NO_VALUE 4
char JSON_ERROR_FIELD_NO_VALUE_MSG[] = PROGMEM "Field has no value";
#define JSON_ERROR_UNEXPECTED_VALUE 5
char JSON_ERROR_UNEXPECTED_VALUE_MSG[] = PROGMEM "Unexpected value";
#define JSON_ERROR_UNKNOWN_STATE 6
char JSON_ERROR_UNKNOWN_STATE_MSG[] = PROGMEM "Unknown state";
#define JSON_ERROR_OUT_OF_MEMORY 7
char JSON_ERROR_OUT_OF_MEMORY_MSG[] = PROGMEM "Out of memory";

template<size_t S> class JsonReader {
  public:
    static const int8_t Error = -3;
    static const int8_t EndDocument = -2;
    static const int8_t Initial = -1;
    static const int8_t Value = 0;
    static const int8_t Field = 1;
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
    uint8_t _lastError;

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
    // optimization
    void skipObjectPart()
    {
      int depth = 1;
      while (Error!=_state && LexContext<S>::EndOfInput != _lc.current())
      {
        switch (_lc.current())
        {
          case '[':
            if(LexContext<S>::EndOfInput==_lc.advance()) {
              _lastError = JSON_ERROR_UNTERMINATED_ARRAY;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_ARRAY_MSG,S-1);              
              _state = Error;
              return;
            }
            skipArrayPart();
            break;
            
          case '{':
            ++depth;
            _lc.advance();
            if(LexContext<S>::EndOfInput==_lc.current())
              _lastError = JSON_ERROR_UNTERMINATED_OBJECT;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_OBJECT_MSG,S-1);
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
              _lc.trySkipWhiteSpace();
              return;
            }
            if(LexContext<S>::EndOfInput==_lc.current()) {
              _lastError = JSON_ERROR_UNTERMINATED_OBJECT;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_OBJECT_MSG,S-1);              
              _state = Error;
            }
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
            if(LexContext<S>::EndOfInput==_lc.advance()) {
              _lastError = JSON_ERROR_UNTERMINATED_OBJECT;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_OBJECT_MSG,S-1);              
              _state = Error;
              return;
            }
            skipObjectPart();
            break;
          case '[':
            ++depth;
            if(LexContext<S>::EndOfInput==_lc.advance()) {
              _lastError = JSON_ERROR_UNTERMINATED_ARRAY;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_ARRAY_MSG,S-1);              
              _state = Error;
              return;
            }
            break;
          case '\"':
            skipString();
            if(Error==_state)
              return;
            break;
          case ']':
            --depth;
            _lc.advance();
            if (depth == 0)
            {
              _lc.trySkipWhiteSpace();
              return;
            }
            if(LexContext<S>::EndOfInput==_lc.current()) {
              _lastError = JSON_ERROR_UNTERMINATED_ARRAY;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_ARRAY_MSG,S-1);
              _state = Error;
            }
            break;
          default:
            _lc.advance();
            break;
        }
      }
    }

    void skipString()
    {
      if('\"'!=_lc.current()) {
          _lastError = JSON_ERROR_UNTERMINATED_STRING;
          strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_STRING_MSG,S-1);
         _state = Error;
         return;
      }
      _lc.advance();
      _lc.trySkipUntil('\"', false);
      if('\"'!=_lc.current()) {
          _lastError = JSON_ERROR_UNTERMINATED_STRING;
          strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_STRING_MSG,S-1);
         _state = Error;
         return;
      }
      _lc.advance();
      _lc.trySkipWhiteSpace();
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
    uint8_t lastError() {
      return _lastError;    
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
                _lastError = JSON_ERROR_UNTERMINATED_ARRAY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_ARRAY_MSG,S-1);
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
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
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
                  _lastError = JSON_ERROR_OUT_OF_MEMORY;
                  strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                  _state = Error;
                  return true;
                }
              }
              _lc.trySkipWhiteSpace();
              return true;
            case '\"':
              _lc.capture();
              _lc.advance();
              if(!_lc.tryReadUntil('\"', '\\', true)) {
                if(LexContext<S>::EndOfInput==_lc.current()) {
                  _lastError = JSON_ERROR_UNTERMINATED_STRING;
                  strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNTERMINATED_STRING_MSG,S-1);
                  _state = Error;
                  return true;

                } else {
                  _lastError = JSON_ERROR_OUT_OF_MEMORY;
                  strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                  _state = Error;
                  return true;
                }
              }
              _lc.trySkipWhiteSpace();
              if (':' == _lc.current())
              {
                _lc.advance();
                _lc.trySkipWhiteSpace();
                if (LexContext<S>::EndOfInput == _lc.current()) {
                  _lastError = JSON_ERROR_FIELD_NO_VALUE;
                  strncpy_P(_lc.captureBuffer(),JSON_ERROR_FIELD_NO_VALUE_MSG,S-1);
                  _state = Error;
                  return true;
                }
                _state = Field;
              }
              return true;
            case 't':
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('r' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('u' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('e' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              _lc.advance();
              _lc.trySkipWhiteSpace();
              ch = _lc.current();
              if (',' != ch && ']' != ch && '}' != ch && LexContext<S>::EndOfInput != ch) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
              }
              return true;
            case 'f':
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('a' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('l' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('s' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('e' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              _lc.advance();
              _lc.trySkipWhiteSpace();
              ch = _lc.current();
              if (',' != ch && ']' != ch && '}' != ch && LexContext<S>::EndOfInput != ch) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
              }
              return true;
            case 'n':
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('u' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('l' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              if ('l' != _lc.advance()) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
                return true;
              }
              if (!_lc.capture()) {
                _lastError = JSON_ERROR_OUT_OF_MEMORY;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_OUT_OF_MEMORY_MSG,S-1);
                _state = Error;
                return true;
              }
              _lc.advance();
              _lc.trySkipWhiteSpace();
              ch = _lc.current();
              if (',' != ch && ']' != ch && '}' != ch && LexContext<S>::EndOfInput != ch) {
                _lastError = JSON_ERROR_UNEXPECTED_VALUE;
                strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
                _state = Error;
              }
              return true;
            default:
              // Serial.printf("Line %d, Column %d, Position %d\r\n",_lc.line(),_lc.column(),(int32_t)_lc.position());
              _lastError = JSON_ERROR_UNEXPECTED_VALUE;
              strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNEXPECTED_VALUE_MSG,S-1);
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
        case JsonReader<S>::Field: // field
          if (!read())
            return false;
          return skipSubtree();
        case JsonReader<S>::Array:// begin array
          skipArrayPart();
          _lc.trySkipWhiteSpace();
          _state = EndArray; // end array
          return true;
        case JsonReader<S>::EndArray: // end array
          return true;
        case JsonReader<S>::Object:// begin object
          skipObjectPart();
          _lc.trySkipWhiteSpace();
          _state = EndObject; // end object
          return true;
        case JsonReader<S>::EndObject: // end object
          return true;
        default:
          _lastError = JSON_ERROR_UNKNOWN_STATE;
          strncpy_P(_lc.captureBuffer(),JSON_ERROR_UNKNOWN_STATE_MSG,S-1);
          _state = Error;
          return true;
      }
    }
    bool skipToIndex(int index) {
      if (Initial==_state || Field == _state) // initial or field
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
    bool skipToField(const char* field, bool searchDescendants = false) {
      if (searchDescendants) {
        while (read()) {
          if (Field == _state) { // field 
            undecorate();
            if (!strcmp(field , value()))
              return true;
          }
        }
        return false;
      }
      switch (_state)
      {
        case JsonReader<S>::Initial:
          if (read())
            return skipToField(field);
          return false;
        case JsonReader<S>::Object:
          while (read() && Field == _state) { // first read will move to the child field of the root
            undecorate();
            if (strcmp(field,value()))
              skipSubtree(); // if this field isn't the target so just skip over the rest of it
            else
              break;
          }
          return Field == _state;
        case JsonReader<S>::Field: // we're already on a field
          undecorate();
          if (!strcmp(field,value()))
            return true;
          else if (!skipSubtree())
            return false;

          while (read() && Field == _state) { // first read will move to the child field of the root
            undecorate();
            if (strcmp(field , value()))
              skipSubtree(); // if this field isn't the target just skip over the rest of it
            else
              break;
          }
          return Field == _state;
        default:
          return false;
      }
    }
    void skipToEndObject() {
      skipObjectPart();
      _state=EndObject;
    }
    void skipToEndArray() {
      skipArrayPart();
      _state=EndArray;
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
        case JsonReader<S>::Field:
        case JsonReader<S>::Value:
        case JsonReader<S>::Error:
          return _lc.captureBuffer();
      }
      return NULL;
    }
};
#endif
