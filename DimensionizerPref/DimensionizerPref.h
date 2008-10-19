//
//  DimensionizerPref.h
//  DimensionizerPreferencePane
//
//  Created by Travis Cripps on 2/8/08.
//  Copyright (c) 2008 Hivelogic. All rights reserved.
//

#import <PreferencePanes/PreferencePanes.h>


#define PRIMARY_OUTPUT_FORMAT @"primaryOutputFormat"
#define SECONDARY_OUTPUT_FORMAT @"secondaryOutputFormat"
#define PRIMARY_CUSTOM_FORMAT @"primaryCustomFormat"
#define SECONDARY_CUSTOM_FORMAT @"secondaryCustomFormat"

#define HEIGHT_TOKEN @"^__HEIGHT__^"
#define WIDTH_TOKEN @"^__WIDTH__^"
#define NAME_TOKEN @"^__NAME__^"



enum OutputFormat {
    HTML = 1,
    CSS,
    Custom
};


@interface DimensionizerPref : NSPreferencePane {
    
    IBOutlet NSTokenField *primaryCustomFormatField;
    IBOutlet NSTokenField *secondaryCustomFormatField;
    
    IBOutlet NSTokenField *predefinedHeightField;
    IBOutlet NSTokenField *predefinedHeightField2;
    IBOutlet NSTokenField *predefinedNameField;
    IBOutlet NSTokenField *predefinedNameField2;
    IBOutlet NSTokenField *predefinedWidthField;
    IBOutlet NSTokenField *predefinedWidthField2;
    
}



+ (void) setupDefaults;
- (void) mainViewDidLoad;

- (void) registerAsObserver;
- (void) unregisterForChangeNotification;

- (void) observeValueForKeyPath: (NSString *)keyPath
                       ofObject: (id)object
                         change: (NSDictionary *)change
                        context: (void *)context;



@end
