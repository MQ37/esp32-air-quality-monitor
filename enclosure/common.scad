// common.scad — shared parameters, constants, and helper modules
// Included by case_base.scad, case_top.scad and assembly.scad.
// All dimensions in mm. Change a value here and re-export both STLs.

/* ========== Board ========== */
board_w  = 94;    // width   (X)
board_l  = 56;    // height  (Y)
board_th = 5.5;   // full stack thickness (display + PCB + back components)

/* ========== Case construction ========== */
wall      = 2.0;   // side wall thickness
floor_t   = 2.0;   // base floor thickness
lid_t       = 2.0;   // lid top-plate thickness
pad         = 10;    // cable clearance between board edge and walls (all sides)
corner_r    = 3.0;   // outer corner radius
lid_gap     = 0.5;   // gap between display surface and lid underside
under_board = 25;    // clear space below the board for sensors (mm)

/* ========== Board mounting ========== */
standoff_d    = 6.0;   // standoff post diameter
hole_inset    = 3.0;   // screw-hole centre inset from the board corner
screw_pilot_d = 2.2;   // pilot hole (M2.5 self-tapping)

/* ========== USB-C power (left wall, centred) ========== */
usb_w = 16;   // opening width along the wall — generous for alignment slop
usb_h = 6;    // opening height

/* ========== Display window (in the lid) ========== */
disp_w     = 74;   // viewable width  (X) — board width minus 2 x 10 mm margin
disp_l     = 56;   // viewable height (Y) — full board height
disp_frame = 0;    // lid overlap onto the display edge (0 = window = active area)

/* ========== Ventilation ========== */
// Wall louvers (fresh-air intake, low on the walls)
louver_w     = 2.5;
louver_h     = 2.5;
louver_gap   = 3.0;
louver_count = 6;
louver_rows  = 2;
louver_vgap  = 3.0;   // vertical gap between louver rows
// Lid exhaust slots (top strip, left of the connector)
vent_slot_w   = 2.0;
vent_slot_len = 5.0;
vent_gap      = 3.0;
vent_rows     = 5;

/* ========== Snap-fit lid ========== */
skirt_h    = 6.0;   // how deep the lid skirt reaches into the base
skirt_wall = 1.5;   // skirt wall thickness (thin enough to flex)
skirt_gap  = 0.3;   // print clearance: skirt outer face to cavity wall
snap_r     = 0.9;   // snap-bead radius (protrudes snap_r - skirt_gap past wall)
snap_len   = 24;    // bead length along the wall

/* ========== Derived ========== */
$fn = 48;

post_h  = under_board;
inner_h = post_h + board_th + lid_gap;

inner_w = board_w + 2 * pad;
inner_l = board_l + 2 * pad;

outer_w = inner_w + 2 * wall;
outer_l = inner_l + 2 * wall;
base_h  = floor_t + inner_h;                  // wall top = where the lid seats

inner_r = max(corner_r - wall, 0.5);
skirt_r = max(inner_r - skirt_gap, 0.5);

// Board origin in case coords; connector gap sits at the high-Y (top) edge.
board_x0 = wall + pad;
board_y0 = wall + pad;
board_cx = board_x0 + board_w / 2;            // = outer_w / 2
board_cy = board_y0 + board_l / 2;

// USB-C opening, centred on the left (-X) wall.
usb_zc = floor_t + post_h - 3;                // centre 3 mm below the standoff tops (board underside)

groove_r = snap_r + 0.25;                      // groove clearance over the bead
snap_z   = base_h - skirt_h + 2;               // bead/groove centre height
snap_x   = [wall, outer_w - wall];             // side walls; bead axis = cavity face

louver_z = floor_t + 1.5;                      // intake band, in the sensor space

echo("Outer footprint (mm):", outer_w, "x", outer_l);
echo("Total closed height (mm):", base_h + lid_t);
echo("Sensor space under board (mm):", post_h);

/* ========== Helpers ========== */

module rounded_rect(w, l, r) {
    offset(r = r)
        translate([r, r])
            square([w - 2 * r, l - 2 * r]);
}

module rounded_prism(w, l, h, r) {
    linear_extrude(height = h)
        rounded_rect(w, l, r);
}

// Cylinder lying along +Y, centred over the case length.
module snap_bead(x, r) {
    translate([x, outer_l / 2 - snap_len / 2, snap_z])
        rotate([-90, 0, 0])
            cylinder(r = r, h = snap_len);
}

// One row of through-slots low on a wall (fresh-air intake).
module wall_louvers(side) {
    span  = (side == "front") ? outer_w : outer_l;
    total = louver_count * louver_w + (louver_count - 1) * louver_gap;
    for (r = [0 : louver_rows - 1]) {
        z = louver_z + r * (louver_h + louver_vgap);
        for (i = [0 : louver_count - 1]) {
            off = span / 2 - total / 2 + i * (louver_w + louver_gap);
            if (side == "front")        // -Y wall, slots along X
                translate([off, -0.01, z])
                    cube([louver_w, wall + 0.02, louver_h]);
            else if (side == "left")    // -X wall, slots along Y
                translate([-0.01, off, z])
                    cube([wall + 0.02, louver_w, louver_h]);
            else if (side == "right")   // +X wall, slots along Y
                translate([outer_w - wall - 0.01, off, z])
                    cube([wall + 0.02, louver_w, louver_h]);
        }
    }
}