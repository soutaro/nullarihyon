// expected-no-diagnostics

#import "../objc/polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject

@property (nonatomic, readonly) NSString *name;

@end

@implementation Test

- (instancetype)init __attribute__((annotate("nlh_initializer"))) {
	self = [super init];

	[self setup];

	return self;
}

- (void)setup __attribute__((annotate("nlh_initializer"))) {
	_name = @"";
}

@end

NS_ASSUME_NONNULL_END
