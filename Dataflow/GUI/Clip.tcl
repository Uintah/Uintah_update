#
#  The contents of this file are subject to the University of Utah Public
#  License (the "License"); you may not use this file except in compliance
#  with the License.
#  
#  Software distributed under the License is distributed on an "AS IS"
#  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#  License for the specific language governing rights and limitations under
#  the License.
#  
#  The Original Source Code is SCIRun, released March 12, 2001.
#  
#  The Original Source Code was developed by the University of Utah.
#  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
#  University of Utah. All Rights Reserved.
#

itcl_class SCIRun_Fields_Clip {
    inherit Module
    constructor {config} {
        set name Clip

	global $this-runmode

        set_defaults
    }

    method set_defaults {} {
	set $this-runmode 0
    }

    method replace {} {
	set $this-runmode 1
	$this-c needexecute
    }

    method ui {} {
        set w .ui[modname]
        if {[winfo exists $w]} {
            raise $w
            return
        }
        toplevel $w

	frame $w.row1

	pack $w.row1 -side top -e y -f both -padx 5 -pady 5
	
	button $w.row1.execute -text "Execute" -command "$this replace"
	pack $w.row1.execute -side top -e n -f both
    }
}


