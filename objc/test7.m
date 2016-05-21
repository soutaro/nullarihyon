// alloc/class/init are assumed nonnull

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

- (void)test3 {
  Test7 * _Nonnull x = [[[self class] alloc] init];
}

- (instancetype)initWithName:(NSString *)name {
  Test7 *a = [super init];
  self = a;
  return self;
}

@end

NS_ASSUME_NONNULL_END
