// If cast changes nullability, it cannot change class

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Test10 : NSObject
@end

@implementation Test10

- (void)test {
  NSString * _Nullable x = @"non null value";
  NSString * _Nonnull y = (NSString * _Nonnull)x;  // ok
  NSNumber * _Nonnull z = (NSNumber * _Nonnull)x;  // NG, it is generally bad cast
  NSObject * _Nonnull o = (NSObject * _Nonnull)x;  // NG, it is generally OK, but prohibited
}

- (void)test2 {
  // If cast does not change nullability, it behaves as Objective-C native semantics.

  NSString *x = @"";
  NSNumber * xO = (NSNumber *)x;

  NSString * _Nullable y = @"";
  NSNumber * _Nullable yO = (NSNumber * _Nullable)y;

  NSString * _Nonnull z = @"";
  NSNumber * _Nonnull zO = (NSNumber * _Nonnull)z;
}

@end

NS_ASSUME_NONNULL_END

