#ifndef DELPHI_BASE64_HPP
#define DELPHI_BASE64_HPP

CString base64_encode(const CString& S);
CString base64_encode(const unsigned char *bytes_to_encode, unsigned int len);

CString base64_decode(const CString& S);
CString base64_decode(const unsigned char *bytes_to_decode, unsigned int len);

#endif
