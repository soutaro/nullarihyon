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
$ cmake .
$ cmake --build .
```

## Run the tool

There are two tools to run the program, `frontend.rb` and `xcode.rb`.

`frontend.rb` would be the good one to try. `xcode.rb` is for Xcode integration.

## Configuration

Make `.nullabilint.yml` file for configuration.
The configuration contains commandline options for compiler, including header file search paths and some options.
The file is used both `frontend.rb` and `xcode.rb`.

Typical configuration for OS X programs should be like the following:

```yaml
:commandline_options:
  - -x
  - objective-c
  - -std=gnu99
  - -fobjc-arc
  - -fobjc-exceptions
  - -fmodules
  - -fasm-blocks
  - -fstrict-aliasing
  - -resource-dir
  - /Users/soutaro/src/nullabilint/build/lib/clang/3.9.0
  - -isysroot
  - /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
```

For iOS programs, something like the following should work:

```yaml
:commandline_options:
  - -x
  - objective-c
  - -std=gnu99
  - -fobjc-arc
  - -fobjc-exceptions
  - -fmodules
  - -fasm-blocks
  - -fstrict-aliasing
  - -resource-dir
  - /Users/soutaro/src/nullabilint/build/lib/clang/3.9.0
  - -isysroot
  - /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk
```

The main difference is path to SDK.

## frontend.rb

Try the tool with `frontend.rb`:

```
$ ruby frontend.rb -e ./nullabilint-core objc/test.m
```

You can pass additional configuration for compiler with `-X` option.

```
$ ruby frontend.rb -e ./nullabilint-core -X-I -X/usr/include objc/test.m
```

## xcode.rb

`xcode.rb` is for Xcode integration.
The script is expected to be invoked during Xcode build session.

* It reads `.xcodeproj` from env var and find `.m` files
* It reads some compiler settings from env var

Add `.nullabilint.yml` in your source code directory. A typical configuration would be like:

```
:commandline_options:
  - -x
  - objective-c
  - -std=gnu99
  - -fobjc-arc
  - -fobjc-exceptions
  - -fmodules
  - -fasm-blocks
  - -fstrict-aliasing
  - -resource-dir
  - /Users/soutaro/src/nullabilint/build/lib/clang/3.9.0
  - -isysroot
  - /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator9.3.sdk
  - -isystem
  - /Users/soutaro/src/ubiregi-client/Pods/Headers/Public
  - -isystem
  - /Users/soutaro/src/ubiregi-client/Pods/Headers/Public/AWSiOSSDKv2
  - -isystem
  - /Users/soutaro/src/ubiregi-client/Pods/Headers/Public/CrittercismSDK
  - -isystem
  - /Users/soutaro/src/ubiregi-client/Pods/Headers/Public/GoogleAnalytics
  - -isystem
  - /Users/soutaro/src/ubiregi-client/Pods/Headers/Public/Helpshift
```

If you are using CocoaPods, some header directries should be explicitly added in the config file.
Most of `.framework` pods are automatically imported by the script.

Add new `Run Script` phase in your Xcode project and write something like:

```
/usr/bin/ruby /Users/soutaro/src/nullabilint/xcode.rb -e /Users/soutaro/src/nullabilint/nullabilint-core
```

The tools runs after each build, and run check for updated source code.

> The tools runs very slowly.
> On my Mac Pro 2013, analysis of project with 500 source code takes ~15 mins.
> Try with small project.

### Restriction

* It does not support prefix headers

# Nullability Check

This tool checks nullability on:

* Local variable assignment with initial value
* Local variable assignment
* Objective-C method call
* Returning value

If *expected* type has `_Nonnull` attribute, and *actual* type does not have, the tool reports warnings.

# Known Issue

* It does not check params and return types for block types (block type itself is checked)
* It checks casts changing nullability, but is not working correctly...

# Future works

* I'm planning to implement very simple *flow sensitive type*, for `if (x) { ... }` pattern.
  In *then* clause, variable in condition can be treated as `_Nonnull`.
* Performance improvement (it's too slow)
* Setup improvement (should be made easier)
