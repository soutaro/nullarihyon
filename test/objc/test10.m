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
  id x; // expected-note{{Variable nullability: unspecified}}
  NSString *a; // expected-note{{Variable nullability: unspecified}}

  a = (NSString * _Nonnull)x; // ok
}

- (void)test3 {
  id<NSObjectProtocol> x; // expected-note{{Variable nullability: unspecified}}
  NSString *a; // expected-note{{Variable nullability: unspecified}}

  a = (NSString * _Nonnull)x;
}

@end

