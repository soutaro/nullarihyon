// If local variable does not have nullability annotation, and with init value,
// its nullability is copied from init value.

#import "polyfill.h"

@interface Test : NSObject
@end

@implementation Test

- (void)argumentIsNonnull:(nonnull NSString *)x {}
- (void)argumentIsNullable:(nullable NSString *)x {}

- (void)test2 {
  NSString *x = @""; // expected-remark{{Variable nullability: nonnull}}
  NSString * _Nullable y = @""; // y is nullable
  NSString *z = y; // expected-remark{{Variable nullability: nullable}}

  [self argumentIsNonnull:x]; // ok
  [self argumentIsNonnull:y]; // expected-warning{{Nullability mismatch on method call argument}}
  [self argumentIsNonnull:z]; // expected-warning{{Nullability mismatch on method call argument}}
}

@end

