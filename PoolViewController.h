//
//  PoolViewController.h
//  Choreographer
//
//  Created by Philippe Kocher on 26.06.09.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "TrajectoryItem.h"

@interface PoolViewController : NSViewController
{
    NSPersistentDocument *document;
    NSManagedObject *projectSettings;
	
	IBOutlet NSOutlineView *userOutlineView;
	IBOutlet NSTableView *audioItemTableView;
	IBOutlet NSTableView *trajectoryTableView;
	
	IBOutlet NSTreeController *treeController;
	IBOutlet NSArrayController *audioItemArrayController;
	IBOutlet NSArrayController *trajectoryArrayController;
	
	IBOutlet NSSegmentedControl *tabControl;
	IBOutlet NSTabView *tabView;
	
	// new trajectory
	IBOutlet NSPanel *newTrajectorySheet;
	TrajectoryType newTrajectoryType;
	NSString *newTrajectoryName;
	NSSet *regionsForNewTrajectoryItem;

	// Context Menu
	IBOutlet NSMenu *contextMenu;
	IBOutlet id dropOrderMenu;

	// drag and drop
    NSArray *draggedNodes;
}

@property (retain) NSString *newTrajectoryName;

+ (PoolViewController *)poolViewControllerForDocument:(NSPersistentDocument *)document;

//- (void)setup;

// IB actions
- (IBAction)poolAddFolder:(id)sender;
- (IBAction)importAudioFiles:(id)sender;
- (IBAction)newTrajectory:(id)sender;
- (IBAction)SpatDifImportTrajectories:(id)sender;
- (IBAction)SpatDifExportTrajectories:(id)sender;
- (IBAction)deleteSelected:(id)sender;
- (IBAction)renameSelected:(id)sender;
- (IBAction)poolTab:(id)sender;
- (IBAction)showTrajectoryInspector:(id)sender;

// actions
- (id)importFile:(NSURL *)absoluteFilePath;
- (void)showSheetForNewTrajectoryItem:(NSString *)name;
- (void)newTrajectorySheetOK;
- (void)newTrajectorySheetCancel;
- (void)newTrajectorySheetDidEnd:(NSPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
- (void)importSpatDIF:(NSURL *)absoluteFilePath;
- (TrajectoryItem *)createNewTrajectoryItem;

- (BOOL)recursivelyDeleteNode:(id)node;
- (void)prelisten:(id)sender index:(int)i;

// selection
- (void)adaptSelection:(NSSet *)selectedAudioRegions;
- (NSArray *)selectedTrajectories;

// notification
- (void)refresh:(NSNotification *)notification;


- (NSString *)nodeImageName:(id)node;
- (NSColor *)nodeTextColor:(id)node;
- (BOOL)treeNode:(NSTreeNode *)aNode isDescendantOfNodeInArray:(NSArray *)nodes;

@end


@interface UserTreeController : NSTreeController
- (void)updateSortIndex;

@end

@interface AudioItemArrayController : NSArrayController {}
@end

@interface TrajectoryArrayController : NSArrayController {}
@end

