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

- (void)test3 {
  NSString * _Nullable x;
  NSString * _Nullable y;

  if (x && y) {
    NSString * _Nonnull z;

    z = x; // ok
    z = y; // ok
  }

  if (x || y) {
    NSString * _Nonnull z;

    z = x; // expected-warning{{Nullability mismatch on assignment}}
    z = y; // expected-warning{{Nullability mismatch on assignment}}
  }

  if (x && !y) {
    NSString * _Nonnull z;

    z = x; // ok
    z = y; // expected-warning{{Nullability mismatch on assignment}}
  }

  if (!x && y) {
    NSString * _Nonnull z;

    z = x; // expected-warning{{Nullability mismatch on assignment}}
    z = y; // ok
  }
}

- (void)test4 {
  NSString * _Nullable x;
  NSString * _Nullable y;
  NSString * _Nullable z;

  BOOL a = x && [self expectingNonnull:x]; // ok
  BOOL b = x || [self expectingNonnull:x]; // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}

  BOOL c = x && y && [self expectingNonnull:x];
  BOOL d = x && y && [self expectingNonnull:y];

  BOOL e = x && [self expectingNonnull:y] && y; // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}

  BOOL f1 = (x || y) && [self expectingNonnull:x]; // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}
  BOOL f2 = (x || y) && [self expectingNonnull:y]; // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}

  BOOL g1 = (x || (y && z)) && [self expectingNonnull:y]; // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}
  BOOL g2 = !(x && y) && [self expectingNonnull:x]; // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}

  BOOL h1 = (x && (y || (z && [self expectingNonnull:x])));
  BOOL h2 = (x && (y || (z && [self expectingNonnull:y]))); // expected-warning{{-[Test11 expectingNonnull:] expects nonnull argument}}
  BOOL h3 = (x && (y || (z && [self expectingNonnull:z])));
}

- (BOOL)expectingNonnull:(nonnull id)paramater {
  return YES;
}

@end
