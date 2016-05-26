// If cast changes nullability, it cannot change class

#import "polyfill.h"

@interface Test10 : NSObject
@end

@implementation Test10

- (void)test {
  NSString * _Nullable x = @"non null value";
  NSString * _Nonnull y = (NSString * _Nonnull)x;  // ok
  NSNumber * _Nonnull z = (NSNumber * _Nonnull)x;  // expected-warning{{Cast on nullability cannot change base type}}
  NSObject * _Nonnull o = (NSObject * _Nonnull)x;  // expected-warning{{Cast on nullability cannot change base type}}
}

- (void)test2 {
  // If cast does not change nullability, it behaves as Objective-C native semantics.

  NSString *x = @""; // expected-note{{Variable nullability: nonnull}}
  NSNumber *xO = (NSNumber *)x; // expected-note{{Variable nullability: unspecified}}

  NSString * _Nullable y = @"";  // expected-note{{Variable nullability: nullable}}
  NSNumber * _Nullable yO = (NSNumber * _Nullable)y;  // expected-note{{Variable nullability: nullable}}

  NSString * _Nonnull z = @"";  // expected-note{{Variable nullability: nonnull}}
  NSNumber * _Nonnull zO = (NSNumber * _Nonnull)z; // expected-note{{Variable nullability: nonnull}}
}

@end

