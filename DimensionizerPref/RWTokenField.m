//
//  RWTokenField.m
//  DimensionizerPref
//
//  From http://www.cocoadev.com/index.pl?NSTokenField
//

#import "RWTokenField.h"


@implementation RWTokenField

+ (void) load
{
	[RWTokenField poseAsClass:[NSTokenField class]];
}

- (void) tokenFieldCellDidTokenizeString:(NSTokenFieldCell*)tokenFieldCell
{
	NSDictionary* valueBindingInformation = [self infoForBinding:@"value"];
	if(valueBindingInformation != nil)
	{
		id valueBindingObject = [valueBindingInformation objectForKey:NSObservedObjectKey];
		NSString* valueBindingKeyPath = [valueBindingInformation objectForKey:NSObservedKeyPathKey];
		
		[valueBindingObject setValue:[self objectValue] forKeyPath:valueBindingKeyPath];
	}
	
	[self sendAction:[self action] to:[self target]];
    
	if([[self delegate] respondsToSelector:@selector(tokenFieldDidTokenizeString:)])
	{
		[[self delegate] performSelector:@selector(tokenFieldDidTokenizeString:) withObject:self];
	}
}

@end
