global CoreTCL
source $CoreTCL/Filebox.tcl

itcl_class Packages/Phil_Tbon_BonoP {

    inherit Module

    protected thefont *-courier-bold-r-normal--*-200-*-*-*-*-*-*

    constructor {config} {
        set name BonoP
        set_defaults
    }


    method set_defaults {} {
	set $this-miniso 0
	set $this-maxiso 1
	set $this-res 0.001
	set $this-timesteps 10
    }

    method metaui {} {
	set meta .ui1[modname]
	if {[winfo exists $meta]} {
	    raise $meta
	    return;
	}

	toplevel $meta
	makeFilebox $meta $this-metafilename \
		"$this-c needexecute" "destroy $meta"
    }

    method ui {} {
        set w .ui[modname]
        if {[winfo exists $w]} {
            raise $w
            return;
        }


        toplevel $w
        wm minsize $w 100 50

        set n "$this-c update"

	frame $w.top -relief groove -bd 2
	frame $w.mid -relief groove -bd 2
	frame $w.bot -relief groove -bd 2

	label $w.title -text "Bono Parameters" 
	button $w.bmeta -text "Metafile..." -font $thefont -command "$this metaui"

	scale $w.top.s1 -label "Isovalue"  -font $thefont\
		-from 0 -to 1 -resolution 0.01 \
		-variable $this-isovalue \
		-length 5.5c -orient horizontal -command $n


	scale $w.top.s2 -label "Time Step"  -font $thefont \
		-from 0 -to 10  \
		-variable $this-timevalue \
		-length 5.5c -orient horizontal -command $n

	pack $w.top.s1 $w.top.s2 -in $w.top -side left -fill x -padx 2 -ipadx 2
	pack $w.bmeta $w.top -in $w -side top -fill both
    }

    method updateFrames {} {
	set w .ui[modname]
	
	destroy $w.top

	frame $w.top -relief groove -bd 2
        set n "$this-c update"

	set vars [$this-c getVars]
	set varlist [split $vars]

	scale $w.top.s1 -label "Isovalue"  -font $thefont \
		-from [lindex $varlist 0] -to [lindex $varlist 1] \
		-resolution [lindex $varlist 2] \
		-variable $this-isovalue \
		-length 5.5c -orient horizontal -command $n

	scale $w.top.s2 -label "Time Step"  -font $thefont \
		-from 0 -to [lindex $varlist 3] \
		-variable $this-timevalue \
		-length 5.5c -orient horizontal -command $n
	
	pack $w.top.s1 $w.top.s2 -in $w.top -side left -fill x -padx 2 -ipadx 2
	pack $w.bmeta $w.top -in $w -side top -fill both
    }
    
}


