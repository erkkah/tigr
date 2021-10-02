#ifndef TIGR_OBJC_H
#define TIGR_OBJC_H

#if defined(__IOS__) || defined (__MACOS__)

#if defined(__OBJC__) && __has_feature(objc_arc)
#error "Can't compile as objective-c code!"
#endif

// ABI is a bit different between platforms
#ifdef __arm64__
#define abi_objc_msgSend_stret objc_msgSend
#else
#define abi_objc_msgSend_stret objc_msgSend_stret
#endif
#ifdef __i386__
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#else
#define abi_objc_msgSend_fpret objc_msgSend
#endif

#define objc_msgSendSuper_t(RET, ...) ((RET(*)(struct objc_super*, SEL, ##__VA_ARGS__))objc_msgSendSuper)
#define objc_msgSend_t(RET, ...) ((RET(*)(id, SEL, ##__VA_ARGS__))objc_msgSend)
#define objc_msgSend_stret_t(RET, ...) ((RET(*)(id, SEL, ##__VA_ARGS__))abi_objc_msgSend_stret)
#define objc_msgSend_id objc_msgSend_t(id)
#define objc_msgSend_void objc_msgSend_t(void)
#define objc_msgSend_void_id objc_msgSend_t(void, id)
#define objc_msgSend_void_bool objc_msgSend_t(void, bool)

#define sel(NAME) sel_registerName(NAME)
#define class(NAME) ((id)objc_getClass(NAME))
#define makeClass(NAME, SUPER) \
    objc_allocateClassPair((Class)objc_getClass(SUPER), NAME, 0)

// Check here to get the signature right: https://nshipster.com/type-encodings/
#define addMethod(CLASS, NAME, IMPL, SIGNATURE) \
    if (!class_addMethod(CLASS, sel(NAME), (IMP) (IMPL), (SIGNATURE))) assert(false)

#define addIvar(CLASS, NAME, SIZE, SIGNATURE) \
    if (!class_addIvar(CLASS, NAME, SIZE, rint(log2(SIZE)), SIGNATURE)) assert(false)

#define objc_alloc(CLASS) objc_msgSend_id(class(CLASS), sel("alloc"))

#if __LP64__ || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

#endif // defined(__IOS__) || defined (__MACOS__)
#endif // TIGR_OBJC_H
