//
//  LoopCounterView.h
//  Choreographer
//
//  Created by Philippe Kocher on 25.03.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "CounterView.h"

@interface LoopCounterView : CounterView
{}

- (void)update:(NSNotification *)notification;
@end
