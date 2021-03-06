//
//  CHProjectDocument.m
//  Choreographer
//
//  Created by Philippe Kocher on 12.05.08.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import "CHProjectDocument.h"
#import "CHGlobals.h"
#import "EditorContent.h"
#import "ProjectWindow.h"
#import "ArrangerView.h"
#import "AudioFile.h"
#import "ToolbarController.h"
#import "PlaybackController.h"
#import "MarkersWindowController.h"

@implementation CHProjectDocument

@synthesize poolViewController;
@synthesize keyboardModifierKeys;

#pragma mark -
#pragma mark initialisation and setup
// -----------------------------------------------------------


- (id)initWithType:(NSString *)type error:(NSError **)error
{
	self = [super initWithType:type error:error];
    if (self != nil)
	{
        NSLog(@"CHProjectDocument: initWithType %@", self);

        NSManagedObjectContext *managedObjectContext = [self managedObjectContext];
        projectData = [NSEntityDescription insertNewObjectForEntityForName:@"ProjectData" inManagedObjectContext:managedObjectContext];
        
    }
    return self;	
}

- (id)init 
{
    self = [super init];
    if (self != nil)
	{
        keyboardModifierKeys = modifierNone;
		[[NSApplication sharedApplication] setValue:self forKeyPath:@"delegate.currentProjectDocument"];
	}

	NSLog(@"CHProjectDocument: init %@", self);

    return self;
}


- (NSString *)windowNibName 
{
	return @"CHProjectDocument";
}


- (void)windowControllerDidLoadNib:(NSWindowController *)theWindowController 
{
    [super windowControllerDidLoadNib:theWindowController];

	// fetch project settings from model
	[self unarchiveProjectSettings];
	[self setProjectSampleRate:[[projectSettings valueForKey:@"projectSampleRate"] intValue]];
	
	// project window
	NSWindow *window = [theWindowController window];
	if([projectSettings valueForKey:@"projectWindowFrame"])
	{
		[window setFrame:NSRectFromString([projectSettings valueForKey:@"projectWindowFrame"]) display:YES];
	}
	
	[window setExcludedFromWindowsMenu:YES];

	
	// instantiate and add pool
	NSView *splitSubview = [[splitView subviews] objectAtIndex:1];
	float width = [[projectSettings valueForKey:@"poolViewWidth"] floatValue];
	NSRect r = NSMakeRect(0, 0, width, [splitSubview frame].size.height);

	if([[projectSettings valueForKey:@"poolDisplayed"] boolValue])
		[splitView setPosition:[splitView frame].size.width - width ofDividerAtIndex:0];	
	else
		[splitView setPosition:[splitView frame].size.width ofDividerAtIndex:0];
	

	poolViewController = [[PoolViewController poolViewControllerForDocument:self] retain];
	[[poolViewController view] setFrame:r];
	[splitSubview addSubview:[poolViewController view]];
//	[poolViewController setup];
    
    [self findUnlinkedAudioFiles];
	
	// setup arranger view (rebuild from data model)
    [arrangerView setup];
		
    // playback controller
	[playbackController setValue:projectSettings forKey:@"projectSettings"];
    
    // initialize audio engine
    [[AudioEngine sharedAudioEngine] setAmbisonicsOrder:[[projectSettings valueForKey:@"ambisonicsOrder"] floatValue]];    
    int mode = [[projectSettings valueForKey:@"distanceBasedAttenuation"] boolValue] ? [[projectSettings valueForKey:@"distanceBasedAttenuationMode"] intValue] + 1 : 0;
    [[AudioEngine sharedAudioEngine] setDistanceBasedAttenuation:mode
                                                  centreZoneSize:[[projectSettings valueForKey:@"distanceBasedAttenuationCentreZoneSize"] doubleValue]
                                                  centreExponent:[[projectSettings valueForKey:@"distanceBasedAttenuationCentreExponent"] doubleValue]
                                               centreAttenuation:[[projectSettings valueForKey:@"distanceBasedAttenuationCentreDB"] doubleValue]
                                                dBFalloffPerUnit:[[projectSettings valueForKey:@"distanceBasedAttenuationDbFalloff"] doubleValue]
                                             attenuationExponent:[[projectSettings valueForKey:@"distanceBasedAttenuationExponent"] doubleValue]];

    double factor = [[projectSettings valueForKey:@"distanceBasedDelay"] boolValue] ? [[projectSettings valueForKey:@"distanceBasedDelayMilisecondsPerUnit"] doubleValue] : 0;
    [[AudioEngine sharedAudioEngine] setDistanceBasedDelay:factor];
    double halfCutoff = [[projectSettings valueForKey:@"distanceBasedFiltering"] boolValue] ? [[projectSettings valueForKey:@"distanceBasedFilteringHalfCutoffUnit"] doubleValue] : 0;
    [[AudioEngine sharedAudioEngine] setDistanceBasedFiltering:halfCutoff];

     // everything that has been done until now (setup, init...)
	// is NOT put on the undo stack
	NSManagedObjectContext *managedObjectContext = [self managedObjectContext];
	[[managedObjectContext undoManager] removeAllActions];	


	// send notifications
	[[NSNotificationCenter defaultCenter] postNotificationName:@"arrangerViewZoomFactorDidChange" object:self];


	// register for notification
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(archiveProjectSettings)
												 name:@"projectSettingsDidChange" object:nil];		
    
}

- (void)findUnlinkedAudioFiles
{
	NSManagedObjectContext *managedObjectContext = [self managedObjectContext];
    NSFetchRequest *fetchRequest = [[[NSFetchRequest alloc] init] autorelease];
    NSError *fetchError = nil;
    NSArray *fetchResults;
	
    NSEntityDescription *entityDescription = [NSEntityDescription entityForName:@"AudioFile" inManagedObjectContext:managedObjectContext];
    
    [fetchRequest setEntity:entityDescription];
 	[fetchRequest setReturnsObjectsAsFaults:NO];
	fetchResults = [managedObjectContext executeFetchRequest:fetchRequest error:&fetchError];

    for(AudioFile *audioFile in fetchResults)
    {
        if(0==[audioFile audioFileID])
        {
            [audioFile handleUnlinkedFile];
            // set the document's dirty flag
            [self updateChangeCount:NSChangeDone];
        }
    }
}

/* versioning:
*/
- (BOOL)configurePersistentStoreCoordinatorForURL:(NSURL *)url
										   ofType:(NSString *)fileType
							   modelConfiguration:(NSString *)configuration
									 storeOptions:(NSDictionary *)storeOptions
											error:(NSError **)error
{

	NSDictionary *newOptions = [NSDictionary dictionaryWithObjectsAndKeys:
							 [NSNumber numberWithBool:YES], NSMigratePersistentStoresAutomaticallyOption,
							 [NSNumber numberWithBool:YES], NSInferMappingModelAutomaticallyOption, nil];
					
	BOOL result = [super configurePersistentStoreCoordinatorForURL:url
															ofType:fileType
												modelConfiguration:configuration
													  storeOptions:newOptions
															 error:error];
	
//	[newOptions release];
	return result;
}

- (void)canCloseDocumentWithDelegate:(id)delegate shouldCloseSelector:(SEL)shouldCloseSelector contextInfo:(void *)contextInfo
{
	NSLog(@"CHProjectDocument: can close ");

	// stop playback
    [playbackController stopPlayback];
    
	// empty the engine's schedule
	[[AudioEngine sharedAudioEngine] deleteAllAudioRegions];
    
	[super canCloseDocumentWithDelegate:delegate shouldCloseSelector:shouldCloseSelector contextInfo:contextInfo]; 
}


- (void)close
{
	NSLog(@"CHProjectDocument: close %@", self);

	[[NSNotificationCenter defaultCenter] removeObserver:self]; // here, not in dealloc!

	if([[NSDocumentController sharedDocumentController] currentDocument] == nil)
	{
		[self synchronizeEditors:NO];
	}
	
	[arrangerView close];
	[[NSApplication sharedApplication] setValue:nil forKeyPath:@"delegate.currentProjectDocument"];
	[super close];
}


- (void)saveDocument:(id)sender
{
	// store window scroll position	
	[projectSettings setValue:[NSNumber numberWithFloat:[[arrangerView superview] bounds].origin.x] forKey:@"arrangerScrollOriginX"];
	[projectSettings setValue:[NSNumber numberWithFloat:[[arrangerView superview] bounds].origin.y] forKey:@"arrangerScrollOriginY"];
    
    // store the project settings
    [self archiveProjectSettings];
	
	[super saveDocument:sender];
}


//- (void)revertDocumentToSaved:(id)sender
//{
//	NSLog(@"CHProjectDocument: revert %@", self);
//	[super revertDocumentToSaved:sender];
//}

- (BOOL)revertToContentsOfURL:(NSURL *)inAbsoluteURL ofType:(NSString *)inTypeName error:(NSError **)outError
{
	NSLog(@"CHProjectDocument: revert to %@ of type %@", inAbsoluteURL, inTypeName);
	
	// manual implementation of the revert process
	[self close];
	[[NSDocumentController sharedDocumentController] openDocumentWithContentsOfURL:inAbsoluteURL display:YES error:outError];

	return YES;
}

- (void)dealloc
{
	NSLog(@"CHProjectDocument: dealloc");
	[projectSettings release];
	[poolViewController release];

	[super dealloc];
}

#pragma mark -
#pragma mark project settings
// -----------------------------------------------------------

- (void)archiveProjectSettings
{
//    NSLog(@"archiveProjectSettings");

    NSMutableData *data;
	NSKeyedArchiver *archiver;
	
	data = [NSMutableData data];
	archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData:data];
	[archiver encodeObject:projectSettings forKey:@"data"];
	[archiver finishEncoding];
	
	// settings are stored in the model
    // (but undo disabled)
    //[[self managedObjectContext] processPendingChanges];
	[[[self managedObjectContext] undoManager] disableUndoRegistration];
	[projectData setValue:data forKey:@"settings"];
	[[self managedObjectContext] processPendingChanges];
	[[[self managedObjectContext] undoManager] enableUndoRegistration];
    
    // but set the document's dirty flag
    [self updateChangeCount:NSChangeDone];

	[archiver release];
}

- (void)unarchiveProjectSettings
{
//    NSLog(@"unarchiveProjectSettings");

	NSManagedObjectContext *managedObjectContext = [self managedObjectContext];
    NSFetchRequest *fetchRequest = [[[NSFetchRequest alloc] init] autorelease];
    NSError *fetchError = nil;
    NSArray *fetchResults;
	
    NSEntityDescription *entityDescription = [NSEntityDescription entityForName:@"ProjectData" inManagedObjectContext:managedObjectContext];

    [fetchRequest setEntity:entityDescription];
 	[fetchRequest setReturnsObjectsAsFaults:NO];
	fetchResults = [managedObjectContext executeFetchRequest:fetchRequest error:&fetchError];
	
    if ((fetchResults != nil) && ([fetchResults count] == 1) && (fetchError == nil))
	{
        projectData = [[fetchResults objectAtIndex:0] retain];
    }
	else
    {
        if (fetchError != nil)
        {
            [self presentError:fetchError];
        }
        
        projectSettings = [[ProjectSettings alloc] initWithDefaults];
        return;
    }

	NSMutableData *data;
	NSKeyedUnarchiver* unarchiver;
	
	data = [projectData valueForKey:@"settings"];
	if(!data)
	{
        projectSettings = [[ProjectSettings alloc] initWithDefaults];
        return;
    }
	else
	{
		unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
		
        [projectSettings release];
        projectSettings = [[unarchiver decodeObjectForKey:@"data"] retain];
		[unarchiver finishDecoding];
		[unarchiver release];
	}
}	


#pragma mark -
#pragma mark actions
// -----------------------------------------------------------

- (IBAction)bounceToDisk:(id)sender
{
	[bounceToDiskController bounceToDisk:self];
}

- (IBAction)xZoomIn:(id)sender
{
	float zoomFactorX = [[projectSettings valueForKey:@"arrangerZoomFactorX"] floatValue];
	zoomFactorX *= 1.2;
	zoomFactorX = zoomFactorX > 25 ? 25 : zoomFactorX;

	[projectSettings setValue:[NSNumber numberWithFloat:zoomFactorX] forKey:@"arrangerZoomFactorX"];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:@"arrangerViewZoomFactorDidChange" object:self];	
}

- (IBAction)xZoomOut:(id)sender
{
	float zoomFactorX = [[projectSettings valueForKey:@"arrangerZoomFactorX"] floatValue];
	zoomFactorX /= 1.2;
	zoomFactorX = zoomFactorX < 0.0001 ? 0.0001 : zoomFactorX;

	[projectSettings setValue:[NSNumber numberWithFloat:zoomFactorX] forKey:@"arrangerZoomFactorX"];

	[[NSNotificationCenter defaultCenter] postNotificationName:@"arrangerViewZoomFactorDidChange" object:self];	
}

- (IBAction)yZoomIn:(id)sender
{	
	float zoomFactorY = [[projectSettings valueForKey:@"arrangerZoomFactorY"] floatValue];
	zoomFactorY *= 1.2;
	zoomFactorY = zoomFactorY > 10 ? 10 : zoomFactorY;

	[projectSettings setValue:[NSNumber numberWithFloat:zoomFactorY] forKey:@"arrangerZoomFactorY"];

	[[NSNotificationCenter defaultCenter] postNotificationName:@"arrangerViewZoomFactorDidChange" object:self];	
}

- (IBAction)yZoomOut:(id)sender
{	
	float zoomFactorY = [[projectSettings valueForKey:@"arrangerZoomFactorY"] floatValue];
	zoomFactorY /= 1.2;
	zoomFactorY = zoomFactorY < 0.1 ? 0.1 : zoomFactorY;

	[projectSettings setValue:[NSNumber numberWithFloat:zoomFactorY] forKey:@"arrangerZoomFactorY"];

	[[NSNotificationCenter defaultCenter] postNotificationName:@"arrangerViewZoomFactorDidChange" object:self];	
}

- (IBAction)zoomToFitContent:(id)sender
{
    [arrangerView zoomToFitContent];
}

- (IBAction)zoomToFitSelection:(id)sender
{
    [arrangerView zoomToFitSelection];
}



- (IBAction)importAudioFiles:(id)sender
{
	[poolViewController importAudioFiles:sender];
}

- (IBAction)newTrajectory:(id)sender
{
	[poolViewController newTrajectory:sender];	
}

- (void)newTrajectoryItem:(NSString *)name forRegions:(NSSet *)regions
{
	[poolViewController setValue:regions forKey:@"regionsForNewTrajectoryItem"];
	[poolViewController showSheetForNewTrajectoryItem:name];
}

- (IBAction)showPool:(id)sender
{
	BOOL poolDisplayed = ![[projectSettings valueForKey:@"poolDisplayed"] boolValue];
	
	[projectSettings setValue:[NSNumber numberWithBool:poolDisplayed] forKey:@"poolDisplayed"];
	
	if(poolDisplayed)
	{
		[sender setState:NSOnState];
		[splitView setPosition:[splitView frame].size.width - [[projectSettings valueForKey:@"poolViewWidth"] floatValue] ofDividerAtIndex:0];
	}
	else
	{
		[sender setState:NSOffState];
		[splitView setPosition:[splitView frame].size.width ofDividerAtIndex:0];
	}
}


#pragma mark -
#pragma mark accessors
// -----------------------------------------------------------
- (float)zoomFactorX
{ 
	return [[projectSettings valueForKey:@"arrangerZoomFactorX"] floatValue];
}

- (float)zoomFactorY
{
	return [[projectSettings valueForKey:@"arrangerZoomFactorY"] floatValue];
}

// If the document exists on disk, return the file name. Otherwise return the default ("Untitled Project").
// This is used for the window title and for the default name when saving. 

- (NSString *)displayName
{
    if ([self fileURL])
	{
        return [super displayName];
    }
	else
	{
        return [[super displayName] stringByReplacingOccurrencesOfString:@"Untitled" withString:@"Untitled Project"];
	}
}

- (void)setProjectSampleRate:(NSUInteger)val
{
	if (val != [[projectSettings valueForKey:@"projectSampleRate"] intValue])
		[projectSettings setValue:[NSNumber numberWithInt:val] forKey:@"projectSampleRate"];
	
	[projectSampleRateTextField setStringValue:[NSString stringWithFormat:@"%d Hz", [[projectSettings valueForKey:@"projectSampleRate"] longValue]]];
    [[AudioEngine sharedAudioEngine] setSampleRate:val];
}


//- (NSSet *)selectedRegions { return [arrangerView valueForKey:@"selectedAudioRegions"]; }
//- (NSMutableArray *)selectedTrajectoriesInPool { return [poolViewController valueForKey:@"selectedTrajectories"]; }


#pragma mark -
#pragma mark selection management
// -----------------------------------------------------------

//- (void)selectedTimeSpanDidChange
//{
////	[arrangerView synchronizeMarquee];
//}

- (void)selectionInPoolDidChange
{
	[self synchronizeEditors:YES];
	[arrangerView synchronizeSelection];
}

- (void)selectionInArrangerDidChange
{
	[self synchronizeEditors:YES];
	[poolViewController adaptSelection:[arrangerView valueForKey:@"selectedAudioRegions"]];
}

- (void)synchronizeEditors:(BOOL)flag
{
	if(flag)
	{
		[[EditorContent sharedEditorContent] synchronizeWithArranger:arrangerView pool:poolViewController];
        [[MarkersWindowController sharedMarkersWindowController] synchronizeWithProject:self];
	}
	else
	{
		// document is being closed
		[[EditorContent sharedEditorContent] synchronizeWithArranger:nil pool:nil];
        [[MarkersWindowController sharedMarkersWindowController] synchronizeWithProject:nil];
	}
    
}

#pragma mark -
#pragma mark window delegate methods
// -----------------------------------------------------------


- (void)windowDidResize:(NSNotification *)notification
{
	NSWindow *window = [notification object];
	[projectSettings setValue:NSStringFromRect([window frame]) forKey:@"projectWindowFrame"];
}

- (void)windowDidMove:(NSNotification *)notification
{
	NSWindow *window = [notification object];
	[projectSettings setValue:NSStringFromRect([window frame]) forKey:@"projectWindowFrame"];
}


#pragma mark -
#pragma mark split view delegate methodes
// ----------------------------------------------------------- 
// prevent subviews from disappearing

- (CGFloat)splitView:(NSSplitView *)sender constrainMinCoordinate:(CGFloat)proposedMin ofSubviewAt:(NSInteger)offset
{
 	if([[projectSettings valueForKey:@"poolDisplayed"] boolValue])
		return [sender frame].size.width - 450;
	else
		return [sender frame].size.width;
}

- (CGFloat)splitView:(NSSplitView *)sender constrainMaxCoordinate:(CGFloat)proposedMax ofSubviewAt:(NSInteger)offset
{
 	if([[projectSettings valueForKey:@"poolDisplayed"] boolValue]) 
		return proposedMax - 180;
	else
		return proposedMax;
}

- (void)splitView:(NSSplitView *)sender resizeSubviewsWithOldSize:(NSSize)oldSize
{
	// resize the split view so that the left frame stays at a constant size
	
	// get the two subviews and the divider thickness
	NSView *left = [[sender subviews] objectAtIndex:0];      
    NSView *right = [[sender subviews] objectAtIndex:1];
    float dividerThickness = [sender dividerThickness];
	
	// get the new size of the whole splitView
    NSRect newFrame = [sender frame];                           
	
	// get the current size of the subviews
    NSRect leftFrame = [left frame];                            
    NSRect rightFrame = [right frame];
	
    // resize the height
	rightFrame.size.height = newFrame.size.height;
    leftFrame.size.height = newFrame.size.height;
	
    // resize the width
    leftFrame.size.width = newFrame.size.width - rightFrame.size.width - dividerThickness;
	
	//leftFrame.origin = NSMakePoint(0,0);							// don't think this is needed
    rightFrame.origin.x = leftFrame.size.width + dividerThickness;
	
    [left setFrame:leftFrame];
    [right setFrame:rightFrame];
}

- (CGFloat)splitView:(NSSplitView *)view constrainSplitPosition:(CGFloat)proposedPosition ofSubviewAt:(NSInteger)index
{
	if([[projectSettings valueForKey:@"poolDisplayed"] boolValue])
	{
		float width = [splitView frame].size.width - proposedPosition;

		[projectSettings setValue:[NSNumber numberWithFloat:width] forKey:@"poolViewWidth"];
	}
	return proposedPosition;
}

/*- (BOOL)splitView:(NSSplitView *)sender canCollapseSubview:(NSView *)subview
 {
 return YES;
 }
 */


@end