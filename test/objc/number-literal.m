// expected-no-diagnostics

#include "polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  NSNumber * _Nonnull a = @123;
  NSNumber * _Nonnull b = @(YES);
  NSNumber * _Nonnull c = @(1.0);
}

@end

NS_ASSUME_NONNULL_END
