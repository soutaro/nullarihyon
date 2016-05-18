# nullabilint

This is experimental tool to check nullability constraints in Objective-C implementation.

# Setup

## Prepare LLVM & Clang

You need a LLVM & Clang. Build them in some directory you like.

The following command line will build in this repo's clone.

```
$ git clone https://github.com/soutaro/nulalbilint.git
$ cd nullabilint
$ svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
$ cd llvm/tools
$ svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
$ cd ../../
$ mkdir build
$ cd build
$ cmake -G "Unix Makefiles" ../llvm
$ make
```

## Build nullabilint

```
$ make tool
$ make tool LLVMCONFIG=path/to/llvm-config  # If you have LLVM different directory
```

## Run the tool

```
$ ./tool objc/SMHello.m -- -I`pwd`/build/lib/clang/3.9.0/include -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks -I build/include -I llvm/include -I build/tools/clang/include -I llvm/tools/clang/include -I /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../include/c++/v1 -I/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/7.3.0/include -I /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include
```

The `-I` and `-F` paths are for my case. You can find your paths with `clang` with `-v`.

```
$ clang -v -fsyntax-only objc/SMHello.m
......
#include "..." search starts here:
#include <...> search starts here:
 /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/7.3.0/include
 /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include
 /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/usr/include
 /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks (framework directory)
End of search list.
```

# Nullability Check

This tool checks nullability on:

* Local variable assignment with initial value
* Local variable assignment
* Objective-C method call
* Returning value

If *expected* type has `_Nonnull` attribute, and *actual* type does not have, the tool reports an error like:

```
AttributedType 0x7fbf941a9490 'NSNumber * _Nonnull' sugar
|-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
| `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
|   `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
`-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
  `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
    `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
AttributedType 0x7fbf93905f30 'NSNumber * _Nullable' sugar
|-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
| `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
|   `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
`-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
  `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
    `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
Nullability mismatch!! (var decl:number2)
/Users/soutaro/src/nullabilint/objc/SMHello.m:7:23
AttributedType 0x7fbf941a9490 'NSNumber * _Nonnull' sugar
|-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
| `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
|   `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
`-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
  `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
    `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
AttributedType 0x7fbf961dca40 'NSNumber * _Null_unspecified' sugar
|-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
| `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
|   `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
`-ObjCObjectPointerType 0x7fbf941a9460 'NSNumber *'
  `-ObjCInterfaceType 0x7fbf938388d0 'NSNumber'
    `-ObjCInterface 0x7fbf941a91e8 'NSNumber'
Nullability mismatch!! (assignment)
/Users/soutaro/src/nullabilint/objc/SMHello.m:11:13
```

It also prints a lot of warnings and errors like:

```
...
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks/ApplicationServices.framework/Frameworks/QD.framework/Headers/ColorSyncDeprecated.h:2034:3: warning: 'CMDeviceProfileInfo'
      is deprecated: first deprecated in OS X 10.6 [-Wdeprecated-declarations]
  CMDeviceProfileInfo  profiles[1];           /* The profile info records */
  ^
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks/ApplicationServices.framework/Frameworks/QD.framework/Headers/ColorSyncDeprecated.h:2015:3: note: 'CMDeviceProfileInfo' has
      been explicitly marked deprecated here
} CMDeviceProfileInfo DEPRECATED_IN_MAC_OS_X_VERSION_10_6_AND_LATER;
...
```

I don't know what is happening...

# Failures

I have tried the tool with some non-trivial sources I'm working on, and found some difficulties.

1. `[NSDictionary valueForKey:]` returns `nullable`
2. `for` loop variables should be `_Nonnull`
3. Some `_Nonnull` attribute on global variables looks ignored

1 and 2 seem essential. 3 seems strange to me, and I will have more investigation.
