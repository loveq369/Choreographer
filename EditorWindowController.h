//
//  EditorWindowController.h
//  Choreographer
//
//  Created by Philippe Kocher on 25.10.08.
//  Copyright 2011 Zurich University of the Arts. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "EditorContent.h"


@interface EditorWindowController : NSWindowController
{
	IBOutlet NSTextField	*infoTextField;
}

// notifications
- (void)updateContent:(NSNotification *)notification;

// abstract methods
- (void)refreshView;

@end
