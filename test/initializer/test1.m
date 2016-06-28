// expected-no-diagnostics

#import "../objc/polyfill.h"

NS_ASSUME_NONNULL_BEGIN

@interface Test : NSObject

@property (nonatomic) NSString *name;
@property (nonatomic, nullable) NSString *address;
@property (nonatomic) NSString *country;
@property (nonatomic, readonly) NSString *gender;

@end

@interface Test()

@property (nonatomic) NSNumber *extensionProperty;

@end

@implementation Test

- (instancetype)init __attribute__((annotate("nlh_initializer"))) {
	self = [super init];

	self.name = @"John Smith";
	_gender = @"male";
	self.extensionProperty = @3;
	_country = @"";

	return self;
}

@end

NS_ASSUME_NONNULL_END
