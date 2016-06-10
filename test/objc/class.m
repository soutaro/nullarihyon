// expected-no-diagnostics

#import "polyfill.h"

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  Class _Nonnull klass1 = [Test class];
  Class _Nonnull klass2 = [self class];
}

@end
