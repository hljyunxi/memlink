%module cmemlink
%{
#include "../../common.h"
#include "memlink_client.h"
%}

#if defined(SWIGJAVA)
%typemap(in) (char *BYTE,int LENGTH){
    $1 = (char *)JCALL2(GetByteArrayElements,jenv,$input,0);
    $2 = (int)JCALL1(GetArrayLength,jenv,$input);
}
%typemap(jni) (char *BYTE,int LENGTH) "jbyteArray"
%typemap(jtype) (char *BYTE,int LENGTH) "byte[]"
%typemap(jstype) (char *BYTE,int LENGTH) "byte[]"
%typemap(javain) (char *BYTE,int LENGTH) "$javainput"

%apply(char *BYTE,int LENGTH){(char *value,int valuelen)};
%apply(char *BYTE,int LENGTH){(char *valmin,int vminlen)};
%apply(char *BYTE,int LENGTH){(char *valmax,int vmaxlen)};

%typemap(out) char BYTE[256]{
   jresult = JCALL1(NewByteArray,jenv,256);
   JCALL4(SetByteArrayRegion,jenv,jresult,0,256,$1);
}

%typemap(jni) (char BYTE[256]) "jbyteArray"
%typemap(jtype) (char BYTE[256]) "byte[]"
%typemap(jstype) (char BYTE[256]) "byte[]"

%apply(char BYTE[256]){(char value[256])};
%apply(char BYTE[256]){(char mask[256])};

#elif defined(SWIGPYTHON)

%typemap(out) char BYTE[256]{
    resultobj = SWIG_FromCharPtrAndSize($1,256);
}

#elif defined(SWIGPHP)
#else
    #warning no "in" typemap defined
#endif

%include "../../common.h"
%include "memlink_client.h"
