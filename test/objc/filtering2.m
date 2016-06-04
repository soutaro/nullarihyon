// Option: -filter Foo

#import "polyfill.h"

@interface Test1 : NSObject

@end

@interface Foo : NSObject

- (NSString *)foo;

@end

@interface Bar : NSObject

- (NSString *)bar;

@end

@implementation Test1 : NSObject

- (void)test1 {
  Foo * _Nonnull foo = [[Foo alloc] init];
  Bar * _Nonnull bar = [[Bar alloc] init];

  NSString * _Nonnull fx;
  NSString * _Nullable fy;

  NSString * _Nonnull bx;
  NSString * _Nullable by;

  fx = [foo foo]; // expected-warning{{Nullability mismatch on assignment}}
  bx = [bar bar];

  fy = [foo foo];
  fx = fy;  // expected-warning{{Nullability mismatch on assignment}}

  by = [bar bar];
  bx = by;
}

@end
