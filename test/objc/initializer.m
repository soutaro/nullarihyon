#import "polyfill.h"

NS_ASSUME_NONNULL_BEGIN

#define NLH_MEMBER_INITIALIZER __attribute__((annotate("nullarihyon_member_initializer")))

@interface Test : NSObject

@property (nonatomic, readonly) NSString *name;
@property (nonatomic, nullable, readonly) NSString *address;

- (instancetype)initWithName:(NSString *)name address:(nullable NSString *)address NLH_MEMBER_INITIALIZER;
- (instancetype)initWithName:(NSString *)name NLH_MEMBER_INITIALIZER;
- (instancetype)initWithName2:(NSString *)name NLH_MEMBER_INITIALIZER;

@end

@implementation Test

- (instancetype)initWithName:(NSString *)name address:(nullable NSString *)address {
  self = [super init];

  _name = name;
  _address = address;

  return self;
}

- (instancetype)initWithName:(NSString *)name {
  self = [super init];

  _name = name;
  _address = nil;

  return self;
}

- (instancetype)initWithName2:(NSString *)name { // expected-warning{{The method should initialize nonnull-readonly members}}
  self = [super init];

  return self;
}

@end

NS_ASSUME_NONNULL_END
