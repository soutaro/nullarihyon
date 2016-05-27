// When `if` with variable reference, the variable can be nonnull in *then* clause.

#import "polyfill.h"

@interface Test11 : NSObject

@end

@implementation Test11

- (void)test {
  NSString * _Nullable x;
  NSString * _Nonnull y;

  if (x) {
    y = x; // ok
  } else {
    y = x; // expected-warning{{Nullability mismatch on assignment}}
  }

  y = x; // expected-warning{{Nullability mismatch on assignment}}
}

- (void)test2 {
  NSString * _Nullable x;
  NSString * _Nonnull y;
  NSString * _Nullable z;

  if (x) {
    // x is nonnull here, and assigning z (nullable) reports warning.
    x = z; // expected-warning{{Nullability mismatch on assignment}}
  }
}

@end
