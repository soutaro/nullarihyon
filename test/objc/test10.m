// If cast changes nullability, it cannot change class

#import "polyfill.h"

@interface Test10 : NSObject
@end

@implementation Test10

- (void)test1 {
  // If cast does not change nullability, it behaves as Objective-C native semantics.

  NSString * _Nullable x;
  NSString *a;
  NSNumber *b;
  NSNumber *c;

  a = (NSString * _Nonnull)x; // ok
  b = (NSNumber *)x; // ok
  c = (NSNumber * _Nonnull)x; // expected-warning{{Cast on nullability cannot change base type}}
}

- (void)test2 {
  id x;
  NSString *a;

  a = (NSString * _Nonnull)x; // ok
}

- (void)test3 {
  id<NSObject> x;
  NSString *a;

  a = (NSString * _Nonnull)x;
}

- (void)test4 {
  NSString * _Nonnull x;
  NSString * _Nonnull y = (NSString * _Nonnull)x; // expected-warning{{Redundant cast to nonnull}}

  NSString *a = @"hello";  // expected-remark{{Variable nullability: nonnull}}
  NSString * _Nonnull b = (NSString * _Nonnull)a; // expected-warning{{Redundant cast to nonnull}}

  id _Nonnull s;
  NSString * _Nonnull t = (NSString * _Nonnull)s; // expected-warning{{Redundant cast to nonnull}}
}

@end
