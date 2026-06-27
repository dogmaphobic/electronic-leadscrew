#ifndef __NEXTION_H
#define __NEXTION_H

//-----------------------------------------------------------------------------
// Page 00 - Splash Screen

// Object Names
static const char* txt_title1     = "txt_title1";
static const char* txt_title2     = "txt_title2";

//-----------------------------------------------------------------------------
// Page 01 - Main Page

// Object Names
static const char* num_rpm_disp   = "num_rpm_disp";
static const char* txt_feed_lbl   = "txt_feed_lbl";
static const char* txt_feed_disp  = "txt_feed_disp";
static const char* btn_fwd        = "btn_fwd";
static const char* btn_rev        = "btn_rev";

// Button IDs
#define btn_fwd1_id      2
#define btn_rev1_id      3
#define btn_gear_id      4

// Index of the images for the buttons (selected)
#define btn_fwd1_img_sel_id     2
#define btn_rev1_img_sel_id     3

// Index of the images for the buttons (unselected)
#define btn_fwd1_img_unsel_id   4
#define btn_rev1_img_unsel_id   5

//-----------------------------------------------------------------------------
// Page 02 - Menu

// Object Names
static const char* btn_metric   = "btn_metric";
static const char* btn_thread   = "btn_thread";
static const char* btn_imperial = "btn_imperial";
static const char* btn_feed     = "btn_feed";

// Button IDs
#define btn_metric_id       7
#define btn_imperial_id     6
#define btn_thread_id       8
#define btn_feed_id         5
#define btn_cust_pitch_id   3
#define btn_wizard_id       4
#define btn_back2_id        2

// Index of the images for the buttons (selected)
#define btn_metric_img_sel_id       12
#define btn_imperial_img_sel_id     11
#define btn_thread_img_sel_id       13
#define btn_feed_img_sel_id         10

// Index of the images for the buttons (unselected)
#define btn_metric_img_unsel_id     16
#define btn_imperial_img_unsel_id   15
#define btn_thread_img_unsel_id     17
#define btn_feed_img_unsel_id       14

//-----------------------------------------------------------------------------
// Page 03 - Custom Thread Pitch

// Object Names
static const char* txt_title3   = "txt_title";
static const char* txt_label3   = "txt_label";
static const char* txt_pitch3   = "txt_pitch";

// Button IDs
#define btn_back3_id        4

//-----------------------------------------------------------------------------
// Page 04 - Wizard 1: Thread Start Count

// Object Names
static const char* txt_title4   = "txt_title";
static const char* num_count4   = "num_count";

// Button IDs
#define btn_back4_id        4

//-----------------------------------------------------------------------------
// Page 05 - Wizard 2/4: Move Tool to Shoulder / Start
// Shoulder and Start arrows are hidden by default. You must make them
// visible on demand (vis pic_shoulder,1)

// Object Names
static const char* txt_title5   = "txt_title";
static const char* pic_shoulder = "pic_shoulder";
static const char* pic_start    = "pic_start";

// Button IDs
#define btn_back5_id        3

//-----------------------------------------------------------------------------
// Page 06 - Wizard 3: Move Tool to Shoulder / Start
//
// This page is also used for generic alerts and info. Note that "txt_dlg" is
// a 90 characters long text field spaning 3 rows. Nextion does not center
// align the text if multi-line. That is, if you set txt_dlg.txt="foo\rvery
// long bar", you will end up with a right aligned two line text. If you
// set txt_dlg.txt="foo bar", it will properly center align that one line of
// text. If using multi-line, you must pre center align it. Also note that
// being a Windows pos, you must use "\r" for line feed.

// Object Names
static const char* txt_dlg6   = "txt_dlg";

// Button IDs
#define btn_back6_id        3

//-----------------------------------------------------------------------------
// Page 07 - Wizard 5: Run

// Object Names
static const char* txt_title7   = "txt_title";
static const char* btn_move     = "btn_move";

// Button IDs
#define btn_move_to_start_id    2
#define btn_back7_id            4

// Index of the images for the buttons (selected)
#define btn_move_to_start_img_sel_id    26

// Index of the images for the buttons (unselected)
#define btn_move_to_start_img_unsel_id  25

#endif // __NEXTION_H
