%module cmemlink
%{
#include "../../common.h"
#include "memlink_client.h"
#include <stddef.h>
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
%apply(char BYTE[256]){(char attr[256])};

#elif defined(SWIGPYTHON)

%typemap(out) char [256]{
    if (offsetof(MemLinkItem, value) == ((unsigned long)$1 - (unsigned long)arg1)) {
        resultobj = SWIG_FromCharPtrAndSize($1,arg1->valuesize);
    }else{
        resultobj = SWIG_FromCharPtrAndSize($1,arg1->attrsize);
    }
}

#elif defined(SWIGPHP)
#else
    #warning no "in" typemap defined
#endif

%include "../../common.h"
%include "memlink_client.h"
