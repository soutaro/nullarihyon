#import "polyfill.h"

@interface Test4 : NSObject

@end

@implementation Test4

- (NSString *)test {
  NSString * _Nonnull a;
  
  NSString * _Nonnull (^block)() = ^NSString * _Nonnull () {
    NSString * _Nullable a;
    return a; // expected-warning{{Block in -[Test4 test] expects nonnull to return}}
  };

  return a;
}

- (nonnull NSString *)test1 {
  NSString * _Nullable a;

  return a; // expected-warning{{-[Test4 test1] expects nonnull to return}}
}

+ (nonnull NSString *)foo {
  NSString * _Nullable a;

  return a; // expected-warning{{+[Test4 foo] expects nonnull to return}}
}

@end
