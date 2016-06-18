#import "polyfill.h"

@interface Test : NSObject

+ (nullable instancetype)new;

@end

@implementation Test

+ (void)test {
  NSObject * _Nonnull x = [NSObject new];
  Test * _Nonnull y = [Test new]; // expected-warning{{Nullability mismatch on variable declaration}}
}

+ (nullable instancetype)new {
  return [[self alloc] init];
}

@end
