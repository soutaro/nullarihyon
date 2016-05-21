// If local variable does not have nullability annotation, and with init value,
// its nullability is copied from init value.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject
@end

@implementation Test

- (void)test:(NSString *)x y:(nullable NSString *)y z:(NSString *)z {
}

- (void)test2 {
  NSString *x = @""; // x is nonnull
  NSString * _Nullable y = @""; // y is nullable
  NSString *z = y; // z is nullable

  [self test:x y:y z:z];
}

@end

NS_ASSUME_NONNULL_END
