//
//  RWTokenFieldCell.m
//  DimensionizerPref
//
//  From http://www.cocoadev.com/index.pl?NSTokenField
//

#import "RWTokenFieldCell.h"


@implementation RWTokenFieldCell

+ (void) load
{
	[RWTokenFieldCell poseAsClass:[NSTokenFieldCell class]];
}

- (void)_string:(id)fp8 tokenizeIndex:(int)fp12 inTextStorage:(id)fp16
{
	[super _string:fp8 tokenizeIndex:fp12 inTextStorage:fp16];
    
	if(![[self controlView] respondsToSelector:@selector(tokenFieldCellDidTokenizeString)]) return;
	
	[[self controlView] performSelector:@selector(tokenFieldCellDidTokenizeString) withObject:self];
}

- (void) setObjectValue:(id<NSCopying>)object
{
	[super setObjectValue:object];
	
	if(![[self controlView] respondsToSelector:@selector(tokenFieldCellDidTokenizeString)]) return;
	
	[[self controlView] performSelector:@selector(tokenFieldCellDidTokenizeString) withObject:self];
}

@end
