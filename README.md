# Nullarihyon

Check nullability consistency in your Objective-C implementation.

Recent Objective-C allows programmers to annotate variables and methods with nullability, as `nonnull` (`_Nonnull`) and `nullable` (`_Nullable`).
However the compiler does not check if they are implemented correctly or not.

This tool checks nullability consistency on:

* Assignment on variables
* Method calls
* Returns

It helps you to find out nullability mismatch and prevent from having unexpected `nil` in your code.

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
  return y; // Warning: returning _Nonnull value is expected
}

NS_ASSUME_NONNULL_END
```

# Install

Install via brew tap.

```
$ brew tap soutaro/nullarihyon
$ brew install nullarihyon
```

# Getting Started

## From command line

`nullarihyon check` command allows checking single `.m` file.

```
$ nullarihyon check sample.m
```

You may have to specify `--sdk` option for `Foundation`, `UIKit`, or `AppKit` classes.

* For iOS apps try with `--sdk iphonesimulator`
* For Mac apps try with `--sdk macosx`

You can find available options for `--sdk` by

```
$ nullarihyon help check
```

## From Xcode

`nullarihyon xcode` command is for Xcode integration.
Add `Run Script Phase` with:

```
if which nullarihyon >/dev/null; then
  nullarihyon xcode
fi
```

Run build from Xcode and you will see tons of warnings ;-)
Thousand of warnings would not make you feel happy; try `--only-latest` option if you like.

```
nullarihyon xcode --only-latest
```

With `--only-latest` option, Xcode warnings will be flushed every time you run build.
Warnings for files which is compiled during last build will be shown.

Nullarihyon checks timestamp of compiled objects to see if the source code is updated or not.
Add the new phase after `Compile Sources` phase.

# On Boarding

If you consider using Nullarihyon to existing project, the workflow would be like the following.

1. Find working set of classes
2. Setup filtering for the classes
3. Add `NS_ASSUME_NONNULL_BEGIN` / `NS_ASSUME_NONNULL_END` to its `.h` and `.m`
4. Run the tool
5. Fix warnings

Nullarihyon would report thouthands of warnings.
It should be better to have a working set with small number of classes.
Focus tens of warnings and fix them.
And, go next classes.

Filtering is explained at [wiki page](https://github.com/soutaro/nullarihyon/wiki/Filtering). Take a look at it.

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

The tool makes you write many casts.
To prevent from writing wrong casts, Nullarihyon reports warning if the cast changes both nullability and type.

```
NSObject * _Nullable x;
NSObject * _Nonnull y = (NSObject * _Nonnull)x;   // This is ok
NSString * _Nonnull z = (NSString * _Nonnull)x;   // This reports warning
```

Casting from `id` is still allowed.

```
id _Nullable x;
NSString * _Nonnull y = (NSString * _Nonnull)x;   // This is ok because x is id
```

## Use `?:` and give default value

`?:` operator is supported.

```objc
NSString * _Nullable x;

NSString * _Nonnull y = x ?: @"";  // This is okay
```

## Declare variable `_Nonnull`

Nullability of local variables declared with initial value will be propagated from the initial value.

```objc
NSString * _Nonnull x;

NSString * y = x;  // This is _Nonnull because initial value is _Nonnull
```

When you declare variable without initial value, its nullability is `_Nullable`.
This rule discourages the following programming style.

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

For this, annotate variable declaration explicitly, and stop assigning `nil`.

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
Variable without initial value will be `nil` anyway, if you enables ARC.

## Assign the `_Nullable` value to variable, and test by `if`

Nullarihyon supports super simple form of flow-sensitive-type.

When condition clause of `if` is a variable reference, the variable will be treated as `_Nonnull` in then clause.

```objc
NSString * _Nullable x;

if (x) {
  NSString * _Nonnull y = x;  // This is okay because x is tested by if
}
```

The analyzer consider binary `&&` with variable reference.

```objc
NSString * _Nullable x;

T a = x && [self nonnullMethod:x];  // x is nonnull on method call

NSString * _Nullable y;

if (x && y) {
  // x and y are nonnull in true clause
}
```

The nullability modification works only with variable reference.

# Rules

Take a look at [wiki page](https://github.com/soutaro/nullarihyon/wiki/Rules) to see the rules of Nullarihyon.
It has some assumptions to minimize number of trivial false positives.

# Limitation

* It does not support prefix headers
* It does not support per-file build setting in Xcode
* It does not support arm architectures (just skip checking)

## Known Issue

* `[self alloc]` does not return `instancetype` but `id` (analysis by Clang compiler)
