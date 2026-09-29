// case_top.scad — ESP32 air-quality monitor enclosure: LID (top)
//
// Drops in with a skirt; a rounded snap-bead on each side wall
// clicks into a matching groove in the base → press to close, pry to open.
// Display window centred over the board; exhaust slots in the top strip.
//
// Export:  openscad -o stl/case_top.stl case_top.scad
// (exported skirt-down, i.e. in the assembled orientation moved to z = 0;
//  flip it in the slicer and print it flat top on the bed)

include <common.scad>

module lid() {
    translate([0, 0, base_h]) {
        difference() {
            rounded_prism(outer_w, outer_l, lid_t, corner_r);

            // display window, centred over the board
            translate([board_cx - disp_w / 2 + disp_frame,
                       board_cy - disp_l / 2 + disp_frame,
                       -0.01])
                cube([disp_w - 2 * disp_frame, disp_l - 2 * disp_frame, lid_t + 0.02]);

            // exhaust slots in the top strip, left of the connector
            for (i = [0 : vent_rows - 1])
                translate([wall + 3 + i * (vent_slot_w + vent_gap),
                           board_cy + disp_l / 2 + 1,
                           -0.01])
                    cube([vent_slot_w, vent_slot_len, lid_t + 0.02]);
        }
    }

    // skirt ring hanging below the top plate
    translate([wall + skirt_gap, wall + skirt_gap, base_h - skirt_h])
        difference() {
            rounded_prism(inner_w - 2 * skirt_gap, inner_l - 2 * skirt_gap, skirt_h, skirt_r);
            translate([skirt_wall, skirt_wall, -0.01])
                rounded_prism(inner_w - 2 * skirt_gap - 2 * skirt_wall,
                              inner_l - 2 * skirt_gap - 2 * skirt_wall,
                              skirt_h + 0.02,
                              max(skirt_r - skirt_wall, 0.3));
        }

    // snap beads on the skirt
    for (x = snap_x) snap_bead(x, snap_r);
}

// Shift the lid down to the floor so it is not floating in the air.
// The bottom of the skirt (originally at z = base_h - skirt_h) lands at z = 0.
translate([0, 0, -(base_h - skirt_h)])
    lid();