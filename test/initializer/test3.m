#import "../objc/polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject

@property (nonatomic, readonly) NSString *name;

@end

@implementation Test

- (instancetype)init __attribute__((annotate("nlh_initializer"))) { // expected-warning{{Nonnull ivar should be initialized: _name}}
	self = [super init];

	return self;
}

@end

NS_ASSUME_NONNULL_END
