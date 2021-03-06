/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Igor Zinken - http://www.igorski.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <jni/javabridge.h>
#include <string.h>

/**
 * called when the Java VM has finished loading the native library
 * NOTE: this must remain within the global namespace
 */
jint JNI_OnLoad( JavaVM* vm, void* reserved )
{
    JNIEnv* env;

    if ( vm->GetEnv(( void** ) &env, JNI_VERSION_1_6 ) != JNI_OK )
        return -1;

    MWEngine::JavaBridge::registerVM( vm );

    return JNI_VERSION_1_6;
}

namespace MWEngine {
namespace JavaBridge {

static JavaVM*  _vm    = nullptr;
static jclass   _class = nullptr; // cached reference to Java mediator class

/**
 * registers the reference to the JAVA_CLASS (and its host environment)
 * where all subsequent calls will be executed upon, the Java class
 * will only expose static methods for its interface with the native code
 */
void registerInterface( JNIEnv* env, jobject jobj )
{
    JNIEnv* environment = getEnvironment(); // always use stored environment reference!
    jclass localRefCls  = environment->FindClass( MWENGINE_JAVA_CLASS );

    if ( localRefCls == NULL )
        return; /* exception thrown */

    /* Create a global reference */
    _class = ( jclass ) environment->NewGlobalRef( localRefCls );

    /* The local reference is no longer useful */
    environment->DeleteLocalRef( localRefCls );

    /* Is the global reference created successfully? */
    if ( _class == NULL) {
        return; /* out of memory exception thrown */
    }

    jmethodID native_method_id = getJavaMethod( JavaAPIs::REGISTRATION_SUCCESS );

    if ( native_method_id != 0 )
        environment->CallStaticVoidMethod( getJavaInterface(), native_method_id, 1 );
}

/**
 * the reference to the JavaVM should be registered
 * when the JNI environment has loaded, the JVM is
 * queried for all subsequent JNI communications with the
 * mediator Java class
 */
void registerVM( JavaVM* aVM )
{
    _vm = aVM;
}

JavaVM* getVM()
{
    return _vm;
}

/**
 * retrieve a Java method ID from the registered Java interface class
 * note: all these methods are expected to be static Java methods
 *
 * @param aAPImethod {javaAPI}
 */
jmethodID getJavaMethod( javaAPI aAPImethod )
{
    jmethodID native_method_id = 0;
    jclass    javaClass        = getJavaInterface();
    JNIEnv*   environment      = getEnvironment();

    if ( javaClass != 0 && environment != 0 )
        native_method_id = environment->GetStaticMethodID( javaClass, aAPImethod.method, aAPImethod.signature );

    return native_method_id;
}

jclass getJavaInterface()
{
    JNIEnv *environment = getEnvironment();

    // might as well return as subsequent usage of
    // the interface requires a valid environment for it's invocation!
    if ( environment == nullptr )
        return nullptr;

    return _class; // we have a cached reference!
}

JNIEnv* getEnvironment()
{
    if ( _vm == nullptr )
        return nullptr;

    JNIEnv *env;
    jint rs = _vm->AttachCurrentThread( &env, NULL );

    // no need to detach after this call as this will be a repeated
    // invocation for communication from C(++) to Java
    // trying to attach a thread that is already attached is a no-op.

    if ( rs == JNI_OK )
        return env;
    else
        return nullptr;
}

std::string getString( jstring aString )
{
    const char* s = getEnvironment()->GetStringUTFChars( aString, NULL );
    std::string theString = s;
    getEnvironment()->ReleaseStringUTFChars( aString, s );

    return theString;
}

}
} // E.O namespace MWEngine
