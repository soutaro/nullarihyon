// Class alloc is assumed nonnull

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface Test7 : NSObject
@end

@implementation Test7

- (void)test1 {
  NSString * _Nonnull x1 = [[NSString alloc] init];
}

- (void)test2 {
  Class _Nonnull klass = [NSString class];
  NSString * _Nonnull x2 = [[klass alloc] init];
}

@end

NS_ASSUME_NONNULL_END
