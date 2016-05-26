// Reading nonnull global variable

// expected-no-diagnostics

#import "polyfill.h"

NSString * _Nonnull const GlobalVariable;

@interface TestClass : NSObject

@end

@implementation TestClass

- (void)test {
  NSString * _Nonnull a = GlobalVariable; // no warning
}

@end

