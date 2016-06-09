// Assigning nullable value to self is allowed.

#import "polyfill.h"

// expected-no-diagnostics

@interface Test : NSObject

@end

@implementation Test

- (instancetype)init {
  typeof(self) _Nullable this;

  self = this;

  return self;
}

@end
