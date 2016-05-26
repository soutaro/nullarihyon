// Block expr is nonnull

// expected-no-diagnostics

#import "polyfill.h"

@interface Test8 : NSObject
@end

@implementation Test8

- (void)test:(nonnull void(^)())block {

}

- (void)test2 {
  [self test:^{}];
}

@end

