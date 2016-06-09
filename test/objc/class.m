// expected-no-diagnostics

#import "polyfill.h"

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  Class _Nonnull klass = [Test class];
}

@end
