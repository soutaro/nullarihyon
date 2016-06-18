// Polyfill without nullability annotation

@protocol NSObject
@end

__attribute__((objc_root_class))
@interface NSObject<NSObject>

+ (instancetype)alloc;
+ (Class)class;
+ (instancetype)new;

- (instancetype)init;
- (Class)class;

@end

@protocol NSCopying

- (id)copy;

@end
