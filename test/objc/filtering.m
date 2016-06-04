// Option: -filter Foo

#import "polyfill.h"

@protocol Prot

- (void)foo:(nonnull NSString *)x;

@end

@interface Foo : NSObject<Prot>

- (void)foobar:(nonnull NSString *)x;
- (nullable instancetype)init;

@end

@interface Bar : NSObject<Prot>
@end

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  NSString * _Nullable x;

  Foo * _Nonnull foo;
  // This is warning on Foo's method, should be reported.
  [foo foo:x]; // no warning

  [foo foobar:x]; // expected-warning{{-[Foo foobar:] expects nonnull argument}}

  Bar * _Nonnull bar;
  [bar foo:x]; // The method is not of Foo

  id<Prot> _Nonnull p = foo;
  [p foo:x]; // The method is from Prot

  id<Prot> _Nonnull q = bar;
  [q foo:x];
}

- (nonnull Foo *)test2 {
  Foo * _Nullable foo = [[Foo alloc] init];
  return foo; // This is not related to Foo
}

@end

@implementation Foo

- (void)test {
  NSString * _Nonnull x;
  NSString * _Nullable y;

  // This is in Foo implementation, should be reported.
  x = y; // expected-warning{{Nullability mismatch on assignment}}
}

- (void)foo:(nonnull NSString *)x {}

- (void)foobar:(nonnull NSString *)x {}

- (nullable instancetype)init { return [super init]; }

@end
