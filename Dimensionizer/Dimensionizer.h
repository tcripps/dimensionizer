/*
 *  Dimensionizer.h
 *  Dimensionizer
 *
 *  Created by Travis Cripps on 1/16/08.
 *  Copyright 2008 Hivelogic. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <QuickTime/QuickTime.h>



// -----------------------------------------------------------------------------
//  Constants
// -----------------------------------------------------------------------------

#define kDimensionizerCMPBundleIdentifier CFSTR("com.hivelogic.Dimensionizer")

#define kDimensionizerCMPlugIn_FactoryID (CFUUIDGetConstantUUIDWithBytes(NULL, \
0xD5, 0x7A, 0xA7, 0x59, 0x30, 0xC3, 0x46, 0x4A, \
0xA0, 0x6C, 0xCD, 0x06, 0x5C, 0x08, 0xB7, 0x3E))
// D5 7A A7 59 - 30 C3 - 46 4A - A0 6C - CD 06 5C 08 B7 3E


// The values of these constants MUST match those in the preference application.
#define PRIMARY_OUTPUT_FORMAT_KEY CFSTR("ComHivelogicDimensionizerPrimaryOutputFormat")
#define SECONDARY_OUTPUT_FORMAT_KEY CFSTR("ComHivelogicDimensionizerSecondaryOutputFormat")
#define PRIMARY_CUSTOM_FORMAT CFSTR("ComHivelogicDimensionizerPrimaryCustomFormat")
#define SECONDARY_CUSTOM_FORMAT CFSTR("ComHivelogicDimensionizerSecondaryCustomFormat")

#define HEIGHT_TOKEN CFSTR("^__HEIGHT__^")
#define WIDTH_TOKEN CFSTR("^__WIDTH__^")
#define NAME_TOKEN CFSTR("^__NAME__^")


// -----------------------------------------------------------------------------
//  typedefs
// -----------------------------------------------------------------------------

// Output formats
enum OutputFormat {
    Menu = 0,
    HTML,
    CSS,
    Custom
};

// Log levels
enum LogLevel {
    LogLevelVerbose,
    LogLevelDebug,
    LogLevelInfo,
    LogLevelWarn,
    LogLevelError
};


// The layout for an instance of DimensionizerCMPlugin_struct.
typedef struct DimensionizerCMPlugin_struct {
	ContextualMenuInterfaceStruct	*cmInterface;
	CFUUIDRef						factoryID;
	UInt32							refCount;
} DimensionizerCMPlugIn_rec, *DimensionizerCMPlugIn_ptr;




// -----------------------------------------------------------------------------
//  Function Declarations
// -----------------------------------------------------------------------------

extern void *DimensionizerCMPlugIn_Factory(CFAllocatorRef allocator, CFUUIDRef typeID);



/* COM interface */
static HRESULT DimensionizerCMPlugIn_QueryInterface(void *thisInstance, REFIID iid, LPVOID *ppv);
static ULONG DimensionizerCMPlugIn_AddRef(void *thisInstance);
static ULONG DimensionizerCMPlugIn_Release(void *thisInstance);



/* COM utility functions */
static DimensionizerCMPlugIn_ptr DimensionizerCMPlugIn_Alloc(CFUUIDRef factoryID);
static void DimensionizerCMPlugIn_Dealloc(DimensionizerCMPlugIn_ptr thisInstance);



/* Contextual menus interface */
static OSStatus DimensionizerCMPlugIn_ExamineContext(void *thisInstance, const AEDesc *context, AEDescList *outCommandPairs);
static OSStatus DimensionizerCMPlugIn_HandleSelection(void *thisInstance, AEDesc *context, SInt32 commandID);
static void  DimensionizerCMPlugIn_PostMenuCleanup(void *thisInstance);



/* Main functions */
static OSStatus CreateMenuWithWithContext(const AEDesc *context, AEDescList *commandList);

static CFArrayRef GetImageInfoForQualifiedItems(const AEDesc *inContext);
static CFDictionaryRef CreateImageInfoDictionaryWithFSRef(FSRefPtr theFSRef);
static CFStringRef CreateStringFromImageInfoDictWithOutputFormat(CFDictionaryRef imageInfoDict, enum OutputFormat format);
static CFStringRef CreateCustomOutputFormatStringFromImageInfoDictWithOutputType(CFDictionaryRef imageInfoDict, Boolean isMainType);

static Boolean InsertCommandIntoCommandListWithOptionsSubmenu(CFStringRef commandName, SInt32 commandID, AEDescList* commands, MenuItemAttributes inAttributes, UInt32 inModifiers, AEDescList *submenuToAttach);

static OSStatus AddStringToPasteboard(PasteboardRef pasteboard, CFStringRef theString);

static int OutputFormatPreferenceForKey(const CFStringRef key);



/* Utility functions */
static Boolean DescIsOfTypeOrCanBeCoercedToType(const AEDesc *desc, OSType desiredType);

static Boolean QTUtil_IsImageFile(const FSSpecPtr fileSpec);
static ImageDescriptionPtr QTUtil_GetImageDescription(const FSSpecPtr fileSpec);

static Boolean GetCString(char **cStringToGet, CFStringRef cfString, CFStringEncoding encoding);

static void LogString(const enum LogLevel logLevel, CFStringRef formatString, ...);
static void LogContext(const AEDescList *context);
void fss2path(char *path, FSSpec *fss);


/* The Contextual Menu Interface function table. */
static ContextualMenuInterfaceStruct gDimensionizerCMInterface =
{
    // Required padding for COM
    NULL, 
    
    // These three are the required COM functions
    DimensionizerCMPlugIn_QueryInterface, 
    DimensionizerCMPlugIn_AddRef, 
    DimensionizerCMPlugIn_Release, 
    
    // Interface implementation
    DimensionizerCMPlugIn_ExamineContext, 
    DimensionizerCMPlugIn_HandleSelection, 
    DimensionizerCMPlugIn_PostMenuCleanup
};


