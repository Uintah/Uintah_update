/*
 *  MakeScalarField.cc:  Unfinished modules
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   March 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Dataflow/Ports/ColumnMatrixPort.h>
#include <Dataflow/Ports/MeshPort.h>
#include <Dataflow/Ports/ScalarFieldPort.h>
#include <Core/Datatypes/ScalarFieldUG.h>
#include <Core/Geometry/Point.h>
#include <Core/Malloc/Allocator.h>

namespace SCIRun {


class MakeScalarField : public Module {
    MeshIPort* inmesh;
    ColumnMatrixIPort* inrhs;
    ScalarFieldOPort* ofield;
public:
    MakeScalarField(const clString& id);
    virtual ~MakeScalarField();
    virtual void execute();
};

extern "C" Module* make_MakeScalarField(const clString& id)
{
    return scinew MakeScalarField(id);
}

MakeScalarField::MakeScalarField(const clString& id)
: Module("MakeScalarField", id, Filter)
{
    inmesh=scinew MeshIPort(this, "Mesh", MeshIPort::Atomic);
    add_iport(inmesh);
    inrhs=scinew ColumnMatrixIPort(this, "RHS", ColumnMatrixIPort::Atomic);
    add_iport(inrhs);
    // Create the output port
    ofield=scinew ScalarFieldOPort(this, "ScalarField", ScalarFieldIPort::Atomic);
    add_oport(ofield);
}

MakeScalarField::~MakeScalarField()
{
}

void MakeScalarField::execute()
{
    MeshHandle mesh;
    if(!inmesh->get(mesh))
	return;
    ColumnMatrixHandle rhshandle;
    ScalarFieldUG* sf;
    if(!inrhs->get(rhshandle))
	return;
    ColumnMatrix& rhs=*rhshandle.get_rep();
    if (rhs.nrows() == mesh->nodesize()) {
	cerr << "Using nodal values";
	sf=scinew ScalarFieldUG(ScalarFieldUG::NodalValues);
    }
    else
	if (rhs.nrows() == mesh->elemsize()) {
	    cerr << "Using element values";
	    sf=scinew ScalarFieldUG(ScalarFieldUG::ElementValues);
	}
	else {
	    cerr << "The ColumnMatrix size, " << rhs.nrows() << ", does not match the number of nodes, " << mesh->nodesize() << ", nor does it match the number of elements, " << mesh->elemsize();
	    return;
	}
    sf->mesh=mesh;
    sf->data.resize(rhs.nrows());
    for(int i=0;i<rhs.nrows();i++){
	if (rhs.nrows() == mesh->nodesize())
	    if(mesh->node(i).bc)
		sf->data[i]=mesh->node(i).bc->value;
	    else	
		sf->data[i]=rhs[i];
	else
	    sf->data[i]=rhs[i];
    }
    ofield->send(ScalarFieldHandle(sf));
}

} // End namespace SCIRun


