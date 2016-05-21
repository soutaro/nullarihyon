// Test for loop variables
// 
// Since most commonly used form of `for x in ...` is with NSArray or NSDictionary,
// we assume loop variables to be `_Nonnull` if no nullability is specified.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Test6 : NSObject

@end

@implementation Test6

- (void)loopTest {
  NSArray<NSString *> *array = @[@"a", @"b", @"c"];

  for (NSString *x in array) {
    NSString * _Nonnull y = x;
  }
}

@end

NS_ASSUME_NONNULL_END
