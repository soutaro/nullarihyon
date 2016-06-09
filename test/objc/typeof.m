#import "polyfill.h"

@interface Test : NSObject

- (nullable instancetype)init;

@end

@implementation Test

- (void)test {
  typeof(self) this;

  Test * _Nonnull x = [this init]; // expected-warning{{Nullability mismatch on variable declaration}}
}

- (nullable instancetype)init {
  return [super init];
}

@end
