#import "polyfill.h"

@interface Test3 : NSObject

@end

@implementation Test3

- (nonnull NSString *)test3 {
  NSString * _Nullable nullableString;
  return nullableString; // expected-warning{{-[Test3 test3] expects nonnull to return}}
}

- (nonnull NSString *)test31 {
  return @"String Literal"; // no warning
}

@end
