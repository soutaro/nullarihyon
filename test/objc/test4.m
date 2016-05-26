#import "polyfill.h"

@interface Test4 : NSObject

@end

@implementation Test4

- (NSString *)test {
  NSString * _Nonnull a;
  
  NSString * _Nonnull (^block)() = ^NSString * _Nonnull () {
    NSString * _Nullable a;
    return a; // expected-warning{{Nullability mismatch on return}}
  };

  return a;
}

@end
