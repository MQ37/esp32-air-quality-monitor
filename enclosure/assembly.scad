// assembly.scad — preview only (not for printing): base and lid together.
//
//   openscad assembly.scad                          # exploded view
//   openscad -D 'view="assembled"' assembly.scad    # lid snapped on
//   openscad -D 'view="base"' assembly.scad         # base only
//   openscad -D 'view="lid"' assembly.scad          # lid only, flipped to show the skirt and snap beads
//
// Print the parts from case_base.scad / case_top.scad (or the files in stl/).

include <common.scad>
use <case_base.scad>
use <case_top.scad>

view    = "exploded";   // "assembled" | "exploded" | "base" | "lid"
explode = 30;           // lid lift in the exploded view (mm)

if (view == "assembled") {
    color("#e8e6e1") base();
    color("#d6d1c7") lid();
} else if (view == "base") {
    color("#e8e6e1") base();
} else if (view == "lid") {
    // upside down, so the skirt and the snap beads face the camera
    color("#d6d1c7")
        translate([0, outer_l, base_h + lid_t]) rotate([180, 0, 0]) lid();
} else {
    color("#e8e6e1") base();
    color("#d6d1c7") translate([0, 0, explode]) lid();
}
