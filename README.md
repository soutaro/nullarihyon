# Nullarihyon

Check nullability consistency in your Objective-C implementation.

Recent Objective-C allows programmers to annotate variables and methods with nullability, like `nonnull`, `_Nonnull`, `nullable`, or `_Nullable`.
However the compiler does not check if they are implemented correctly or not.

This tool checks nullability consistency on:

* Assignment on variables
* Method call
* Return

It helps you to notice nullability mismatch and prevent from having unexpected `nil` in your code.

## Example

The following is an example of programs and warnings.

```objc
NS_ASSUME_NONNULL_BEGIN

@implementation SomeClass

- (void)example {
  NSNumber * _Nullable a = @123;
  NSString * _Nonnull b = [self doSomethingWithString:a]; // Warning: parameter of doSomethingWithString: is _Nonnull
}

- (NSString *)doSomethingWithString:(NSNumber *)x {
  NSString * _Nullable y = x.stringValue;
  return y; // Warning: Returns _Nullable but the method declares to be _Nonnull
}

NS_ASSUME_NONNULL_END
```

# Install

Install nullarihyon via brew tap.

```
$ brew tap soutaro/homebrew-nullarihyon
$ brew install nullarihyon
```

# Getting Started

## From command line

`nullarihyon check` command allows checking single `.m` file.

```
$ nullarihyon check sample.m
```

You may have to specify `--sdk` option for `UIKit` classes or `AppKit` classes.

* For iOS apps try with `--sdk iphonesimulator`
* For Mac apps try with `--sdk macosx`

## From Xcode

`nullarihyon xcode` command is for Xcode integration.
Add `Run Script Phase` with:

```
if which nullarihyon >/dev/null; then
  nullarihyon xcode
fi
```

Run build and you will see so many warnings ;-)
Thousand of warnings would not make you feel happy; try `--only-latest` option if you like.

```
nullarihyon xcode --only-latest
```

With `--only-latest` option, Xcode warnings will be flushed every time you run build.
And warnings on actually compiled files between the build will be shown.

# Fixing Warnings

To fix nullability warnings, there are things you can do.

## Add explicit cast

This is the last resort.
When you are sure the value cannot be `nil`, add explicit cast.

```objc
NSString * _Nullable x = ...;

// Warning here
NSString * _Nonnull y = x;

// Add explicit cast
NSString * _Nonnull z = (NSString * _Nonnull)x;
```

## Use `?:` and give default value

`?:` operator is supported.

```objc
NSString * _Nullable x;

NSString * _Nonnulle y = x ?: @"";  // This is okay
```

## Declare variable `_Nonnull`

Nullability of local variables declared with initial value will be propagated from the initial value.

```objc
NSString * _Nonnull x;

NSString * y = x;  // This is _Nonnull because initial value is _Nonnull
```

When you declare variable without initial value, the nullability is `_Nullable`.
This rule makes a programming style discouraged.

```objc
NSString *x = nil;

if (a == 1) {
  x = @"one";
} else {
  x = @"other";
}

NSString * _Nonnull y = x;  // x cannot be nil here
```

Clearly the value of `x` after `if` statement cannot be `nil`.
However, Nullarihyon reports warning on the assignment to `y` since `x` is `_Nullable` because of its declaration.

For this, annotate variable declaration explicitly, and stop assigning `nil` explicitly.

```objc
NSString * _Nonnull x;

if (a == 1) {
  x = @"one";
} else {
  x = @"other";
}

NSString * _Nonnull y = x;
```

Nothing will go wrong.
Variable without initial value will be `nil` anyway, by if you enables ARC.

## Assign the `_Nullable` value to variable, and test by `if`

Nullarihyon supports super simple form of flow-sensitive-type.

When condition clause of `if` is a variable reference, the variable will be treated as `_Nonnull` in then clause.

```objc
NSString * _Nullable x;

if (x) {
  NSString * _Nonnull y = x;  // This is okay because x is tested by if
}
```

The condition clause of `if` have to be exactly one variable reference.
Any other forms including conjunction `&&` and property reference are not supported.

# Limitation

* It does not support prefix headers
* It does not support per-file build setting in Xcode

# Nullability Check

This tool checks nullability on:

* Local variable assignment with initial value
* Local variable assignment
* Objective-C method call
* Returning value

If *expected* type has `_Nonnull` attribute, and *actual* type does not have, the tool reports warnings.

## Known Issue

* It does not check params and return types for block types (block type itself is checked)
* It checks casts changing nullability, but is not working correctly...

