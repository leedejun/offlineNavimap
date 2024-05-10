#import "MWMButtonCell.h"

@interface MWMButtonCell ()

@property(nonatomic) IBOutlet UIButton *button;
@property(weak, nonatomic) id<MWMButtonCellDelegate> delegate;

@end

@implementation MWMButtonCell

- (void)configureWithDelegate:(id<MWMButtonCellDelegate>)delegate title:(NSString *)title {
  [self.button setTitle:title forState:UIControlStateNormal];
  self.delegate = delegate;
}

- (IBAction)buttonTap {
  [self.delegate cellDidPressButton:self];
}

@end
