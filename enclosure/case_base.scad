// case_base.scad — ESP32 air-quality monitor enclosure: BASE
//
// Open-top box; the board screws onto four corner standoffs (M2.5 self-tapping
// screws into 2.2 mm pilot holes), with 25 mm of space underneath for sensors.
// USB-C opening centred on the left wall; fresh-air louvers on three walls.
// Snap grooves in the side walls accept the lid's snap beads.
//
// Export:  openscad -o stl/case_base.stl case_base.scad

include <common.scad>

module standoffs() {
    for (x = [board_x0 + hole_inset, board_x0 + board_w - hole_inset],
         y = [board_y0 + hole_inset, board_y0 + board_l - hole_inset])
        translate([x, y, floor_t])
            difference() {
                cylinder(d = standoff_d, h = post_h);
                translate([0, 0, -0.01])
                    cylinder(d = screw_pilot_d, h = post_h + 0.02);
            }
}

module base() {
    difference() {
        rounded_prism(outer_w, outer_l, base_h, corner_r);

        // hollow the cavity (overshoot the top to leave only the floor)
        translate([wall, wall, floor_t])
            rounded_prism(inner_w, inner_l, base_h, inner_r);

        // USB-C power opening in the left wall, centred (generous for slop)
        translate([-0.01, outer_l / 2 - usb_w / 2, usb_zc - usb_h / 2])
            cube([wall + 0.02, usb_w, usb_h]);

        // fresh-air intake louvers
        wall_louvers("front");
        wall_louvers("left");
        wall_louvers("right");

        // snap grooves in the side walls
        for (x = snap_x) snap_bead(x, groove_r);
    }
    standoffs();
}

base();