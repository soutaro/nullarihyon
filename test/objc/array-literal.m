// 

#import "polyfill.h"

@interface NSArray ()

+ (_Null_unspecified instancetype)arrayWithObjects:(const id _Nonnull [])objects count:(NSUInteger)count;

@end

@interface Test12 : NSObject

@end

@implementation Test12

- (void)test {
  NSString * _Nullable x;
  NSArray<NSString *> * _Nonnull array = @[x]; // expected-warning{{Array element should be nonnull}}
}

- (void)test1 {
  NSString * _Nonnull x;
  NSArray<NSString *> * _Nonnull array = @[x]; //okay
}


@end

