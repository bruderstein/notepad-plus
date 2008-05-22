//This code was retrieved from
//http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/
//(Visual C++ exception handling)
//By Bennett
//Formatting Slightly modified for N++

#include "windows.h"
#include <exception>

typedef const void* ExceptionAddress; // OK on Win32 platform

class Win32Exception : public std::exception
{
public:
    static void 		installHandler();
    virtual const char* what()  const { return _event;    };
    ExceptionAddress 	where() const { return _location; };
    unsigned 			code()  const { return _code;     };

protected:
    Win32Exception(const EXCEPTION_RECORD * info);	//Constructor only accessible by exception handler
    static void 		translate(unsigned code, EXCEPTION_POINTERS * info);

private:
    const char * _event;
    ExceptionAddress _location;
    unsigned int _code;
};

class Win32AccessViolation: public Win32Exception
{
public:
    bool 				isWrite()    const { return _isWrite;    };
    ExceptionAddress	badAddress() const { return _badAddress; };
private:
    Win32AccessViolation(const EXCEPTION_RECORD * info);

    bool _isWrite;
    ExceptionAddress _badAddress;

    friend void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};
