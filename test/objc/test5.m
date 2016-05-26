#import "polyfill.h"

@interface Test5 : NSObject

@end

@implementation Test5

- (void)test {
  Test5 * _Nonnull this = [[Test5 alloc] init]; // no warning, alloc and init returns nil
}

@end

@interface Test6 : NSObject

- (nullable instancetype)init;

@end

@implementation Test6

- (nullable instancetype)init {
  return [super init];
}

- (void)test {
  // Take account of nullability annotation on init
  Test6 * _Nonnull this = [[Test6 alloc] init]; // expected-warning{{Nullability mismatch on variable declaration}}
}

@end
