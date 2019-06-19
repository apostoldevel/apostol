#ifndef BASE64_H
#define BASE64_H

CString base64_encode(const unsigned char *, unsigned int len);
CString base64_decode(const CString& s);

#endif
