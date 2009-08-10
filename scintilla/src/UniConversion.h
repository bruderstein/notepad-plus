// Scintilla source code edit control
/** @file UniConversion.h
 ** Functions to handle UFT-8 and UCS-2 strings.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// NPPSTART Joce 08/09/09 LintCleanup
#ifndef UNICONVERSION_H
#define UNICONVERSION_H
// NPPEND

unsigned int UTF8Length(const wchar_t *uptr, unsigned int tlen);
void UTF8FromUTF16(const wchar_t *uptr, unsigned int tlen, char *putf, unsigned int len);
unsigned int UTF16Length(const char *s, unsigned int len);
unsigned int UTF16FromUTF8(const char *s, unsigned int len, wchar_t *tbuf, unsigned int tlen);

// NPPSTART Joce 08/09/09 LintCleanup
#endif // UNICONVERSION_H
// NPPEND
