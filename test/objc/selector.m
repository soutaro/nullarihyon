// expected-no-diagnostics

#import "polyfill.h"

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  SEL _Nonnull sel = @selector(test);
}

@end
