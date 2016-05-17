#import "SMHello.h"

@implementation SMHello

- (void)say {
  NSNumber * _Nullable number1 = @123;
  NSNumber * _Nonnull number2 = number1;
  NSNumber * _Null_unspecified number3 = number2;
  NSNumber * _Nonnull xxx = @111;

  number2 = number3;

  NSString *x = [self methodWithoutHeader];
  NSString *message = [NSString stringWithFormat:@"Hello World"];
  message = [message stringByAppendingString:@"!!"];
  NSLog(@"message = %@", [message stringByAppendingString:@"hogehoge"]);
}

- (NSString *)methodWithoutHeader {
  return @"";
}

@end
