// expected-no-diagnostics  

#import "polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  NSArray<NSString *> * _Nonnull a;
  id _Nonnull x = a[0];  // array subscript is nonnull

  NSDictionary<NSString *, NSString *> * _Nonnull b;
  id _Nullable y = b[@"hoge"];  // keyd subscript is nullable
}

@end

NS_ASSUME_NONNULL_END
