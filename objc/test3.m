#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Test3 : NSObject

- (NSString *)test3;

@end

@implementation Test3

- (NSString *)test3 {
  NSString *nullableString = @"";
  return nullableString;
}

- (NSString *)test31 {
  return @"String Literal";
}

@end

NS_ASSUME_NONNULL_END
