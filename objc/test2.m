#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NSString * _Nonnull const GlobalVariable = @"Hello World";

@interface TestClass : NSObject

@end

@implementation TestClass

- (void)test {
  NSString * _Nonnull a = GlobalVariable;
}

@end

NS_ASSUME_NONNULL_END
