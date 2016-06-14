#import "polyfill.h"

@interface Test : NSObject
@end

@implementation Test

- (void)test {
  NSString * _Nullable a;
  NSString * _Nonnull b;
  BOOL c;

  NSString * _Nonnull x = a ?: @""; // ok; cond is nullable
  NSString * _Nonnull y = b ?: @""; // expected-warning{{Conditional operator looks redundant}}
  BOOL z = c ?: NO; // ok; cond is not an object
}

- (void)test2 {
  NSString * _Nonnull a;
  id _Nonnull b;
  id<NSObject> _Nonnull c;
  void (^ _Nonnull d)();

  id _Nonnull x;
  x = a ?: @""; // expected-warning{{Conditional operator looks redundant}}
  x = b ?: @""; // expected-warning{{Conditional operator looks redundant}}
  x = c ?: @""; // expected-warning{{Conditional operator looks redundant}}
  x = d ?: ^{}; // expected-warning{{Conditional operator looks redundant}}
}

@end
