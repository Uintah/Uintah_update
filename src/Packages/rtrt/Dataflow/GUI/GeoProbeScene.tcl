#
#  GeoProbeScene.tcl
#
#  Written by:
#   David Weinstein
#   Department of Computer Science
#   University of Utah
#   January 2003
#
#  Copyright (C) 2003 SCI Group
#

itcl_class rtrt_Scenes_GeoProbeScene {
    inherit Module

    constructor {config} {
	set name GeoProbeScene
	set_defaults
    }
    
    method set_defaults {} {
	global $this-isoval
	set $this-isoval 100
	global $this-xa
	set $this-xa 0.1
	global $this-xb
	set $this-xb 0.9
	global $this-ya
	set $this-ya 0.1
	global $this-yb
	set $this-yb 0.9
	global $this-za
	set $this-za 0.1
	global $this-zb
	set $this-zb 0.9
	global $this-gpfilename
	set $this-gpfilename "/usr/sci/data/Seismic/BP/k12bvox.vol"

	global $this-xa-active
	set $this-xa-active 1
	global $this-xa-usemat
	set $this-xa-usemat 1

	global $this-color-r
	global $this-color-g
	global $this-color-b
	set $this-color-r 0.5
	set $this-color-g 0.5
	set $this-color-b 0.5

	global $this-iso_min
	global $this-iso_max
	global $this-iso_val
	set $this-iso_min 0
	set $this-iso_max 255
	set $this-iso_val 40
    }

    method raiseColor {swatch color msg} {
	 global $color
	 set window .ui[modname]
	 if {[winfo exists $window.color]} {
	     raise $window.color
	     return;
	 } else {
	     toplevel $window.color
	     makeColorPicker $window.color $color \
		     "$this setColor $swatch $color $msg" \
		     "destroy $window.color"
	 }
    }

    method setColor {swatch color msg} {
	 global $color
	 global $color-r
	 global $color-g
	 global $color-b
	 set ir [expr int([set $color-r] * 65535)]
	 set ig [expr int([set $color-g] * 65535)]
	 set ib [expr int([set $color-b] * 65535)]

	 set window .ui[modname]
	 $swatch config -background [format #%04x%04x%04x $ir $ig $ib]
	 $this-c $msg
    }

    method addColorSelection {w text color msg} {
	 #add node color picking 
	 global $color
	 global $color-r
	 global $color-g
	 global $color-b
	 set ir [expr int([set $color-r] * 65535)]
	 set ig [expr int([set $color-g] * 65535)]
	 set ib [expr int([set $color-b] * 65535)]
	 
	 frame $w.frame
	 frame $w.frame.swatch -relief ridge -borderwidth \
		 4 -height 0.8c -width 1.0c \
		 -background [format #%04x%04x%04x $ir $ig $ib]
	 
	 set cmd "$this raiseColor $w.frame.swatch $color $msg"
	 button $w.frame.set_color -text $text -command $cmd
	 
	 #pack the node color frame
	 pack $w.frame.set_color $w.frame.swatch -side left
	 pack $w.frame -side top

    }

    # this is the main function which creates the initial window
    method ui {} {
	set w .ui[modname]
	if {[winfo exists $w]} {
	    wm deiconify $w
	    raise $w
	    return;
	}
	toplevel $w
	wm minsize $w 100 50
	set n "$this-c needexecute"

	frame $w.cut
	
	frame $w.cut.xa
	scale $w.cut.xa.scale -variable $this-xa \
	    -label "X Low" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 1 \
	    -tickinterval 0.25 -resolution 0.01 \
	    -command "$this-c update_cut xa"
	checkbutton $w.cut.xa.act -text "Active" \
	    -variable $this-xa-active -command "$this-c update_active xa"
	checkbutton $w.cut.xa.mat -text "Use Material" \
	    -variable $this-xa-usemat -command "$this-c update_usemat xa"
	
	pack $w.cut.xa.scale -side left -expand 1
	pack $w.cut.xa.act $w.cut.xa.mat -side top -anchor w -fill x -expand 1
	
	scale $w.cut.xb -variable $this-xb \
	    -label "X High" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 1 \
	    -tickinterval 0.25 -resolution 0.01 \
	    -command "$this-c update_cut xb"
	scale $w.cut.ya -variable $this-ya \
	    -label "Y Low" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 1 \
	    -tickinterval 0.25 -resolution 0.01 \
	    -command "$this-c update_cut ya"
	scale $w.cut.yb -variable $this-yb \
	    -label "Y High" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 1 \
	    -tickinterval 0.25 -resolution 0.01 \
	    -command "$this-c update_cut yb"
	scale $w.cut.za -variable $this-za \
	    -label "Z Low" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 1 \
	    -tickinterval 0.25 -resolution 0.01 \
	    -command "$this-c update_cut za"
	scale $w.cut.zb -variable $this-zb \
	    -label "Z High" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 1 \
	    -tickinterval 0.25 -resolution 0.01 \
	    -command "$this-c update_cut zb"
	pack $w.cut.xa $w.cut.xb $w.cut.ya $w.cut.yb $w.cut.za $w.cut.zb \
	    -side top -expand 1 -fill x
	
	frame $w.isosurface
	scale $w.isosurface.slider -variable $this-iso_val \
	    -label "Isovalue" -showvalue true -orient horizontal \
	    -relief groove -length 200 -from 0 -to 255 \
	    -tickinterval 80 -resolution 0.01 \
	    -command "$this-c update_isosurface_value"

	frame $w.isosurface.color
	addColorSelection $w.isosurface.color "Isosurface Color" $this-color \
	    "update_isosurface_material"

	pack $w.isosurface.slider $w.isosurface.color \
	    -side top -expand 1 -fill x


	frame $w.buttons
	button $w.buttons.dismiss -text "Dismiss" -command "wm withdraw $w"
	pack $w.buttons.dismiss \
		-side left -padx 10

	pack $w.cut $w.isosurface $w.buttons -side top -pady 5
    }
}
