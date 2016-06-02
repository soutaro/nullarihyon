// 

#import "polyfill.h"

@interface NSDictionary

+ (instancetype)dictionaryWithObjects:(const id _Nonnull [])objects
                              forKeys:(const id<NSCopying> _Nonnull [])keys
                                count:(NSUInteger)count;

@end

@interface Test : NSObject

@end

@implementation Test

- (void)test {
  NSString * _Nullable x;
  NSString * _Nonnull y;

  NSDictionary * _Nonnull a = @{ @"key": x }; // expected-warning{{Dictionary value should be nonnull}}
  NSDictionary * _Nonnull b = @{ @"key": y }; // ok
}

- (void)test1 {
  NSString * _Nullable x;
  NSString * _Nonnull y;

  NSDictionary * _Nonnull a = @{ x: @"value" }; // expected-warning{{Dictionary key should be nonnull}}
  NSDictionary * _Nonnull b = @{ y: @"value" }; // ok
}

@end

