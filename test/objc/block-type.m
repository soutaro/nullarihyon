#import "polyfill.h"

typedef NSString * _Nonnull (^CallbackType1)(NSString * _Nullable);
typedef NSString * _Nullable (^CallbackType2)(NSString * _Nonnull);

@interface Test : NSObject
@end

@implementation Test

- (void)test1 {
  [self foo:^NSString * _Nonnull (NSString * _Nullable x) { return @""; }]; // ok

  // Parameter type of CallbackType is nullable, but block expr has nonnull parameter
  [self foo:^NSString * _Nonnull (NSString * _Nonnull x) { return @""; }]; // expected-warning{{Argument does not have expected block type to -[Test foo:]}}

  // Return type of CallbackType is nonnull, but block expr returns nullable
  [self foo:^NSString * _Nullable (NSString * _Nullable x) { return @""; }]; // expected-warning{{Argument does not have expected block type to -[Test foo:]}}
}

- (void)test2 {
  NSString * _Nullable (^block1)(NSString * _Nonnull) = [self bar];

  // Parameter type of block2 is nullable, but CallbackType2 has nonnull parameter
  NSString * _Nullable (^block2)(NSString * _Nullable) = [self bar]; // expected-warning{{Nullability mismatch inside block type on variable declaration}}

  // Return type of block3 is nonnull, but CallbackType2 has nullable parameter
  NSString * _Nonnull (^block3)(NSString * _Nonnull) = [self bar]; // expected-warning{{Nullability mismatch inside block type on variable declaration}}
}

- (void)foo:(nonnull CallbackType1)block {
}

- (nonnull CallbackType2)bar {
  return ^NSString * _Nullable (NSString * _Nonnull x) { return @""; };
}

@end
