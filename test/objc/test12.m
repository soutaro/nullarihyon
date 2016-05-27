// Revise error message

#import "polyfill.h"

@interface Test12 : NSObject

@end

@implementation Test12

- (nonnull NSObject *)doSomethingWithString:(nonnull NSString *)string {
  id _Nonnull x;

  return x;
}

+ (void)doSomethingWithString:(nonnull NSString *)string {
}

- (void)test {
  NSString * _Nullable x;

  [self doSomethingWithString:x]; // expected-warning{{-[Test12 doSomethingWithString:] expects nonnull argument}}

  [Test12 doSomethingWithString:x]; // expected-warning{{+[Test12 doSomethingWithString:] expects nonnull argument}}
}


@end
