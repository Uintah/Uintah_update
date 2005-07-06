function [grid,q] = addGridPatch(grid,k,ilower,iupper,parentQ)
%ADDGRIDPATCH  Add a patch to the AMR grid.
%   [GRID,Q,A,B] = ADDGRIDPATCH(GRID,K,ILOWER,IUPPER,PARENTQ,A,B) updates
%   the left hand side matrix A and the right hand side B of the composite
%   grid linear system with a new patch Q at level K, whose extents
%   (without ghost cells) are ILOWER to IUPPER, under the parent patch
%   PARENTQ.
%
%   See also: ADDGRIDLEVEL, TESTDISC, UPDATESYSTEM.

globalParams;

tStartCPU           = cputime;
tStartElapsed       = clock;

if (param.verboseLevel >= 1)
    fprintf('--- addGridPatch(k = %d) BEGIN ---\n',k);
end

if (max(ilower > iupper))
    error('Cannot create patch: ilower > iupper');
end

%==============================================================
% 1. Create an empty patch
%==============================================================

grid.level{k}.numPatches    = grid.level{k}.numPatches+1;
q                           = grid.level{k}.numPatches;
P.ilower                    = ilower;
P.iupper                    = iupper;
P.size                      = P.iupper - P.ilower + 3;          % Size including ghost cells
P.parent                    = parentQ;
P.children                  = [];
P.offsetSub                 = -P.ilower+2;                      % Add to level-global cell index to get this-patch cell index. Lower left corner (a ghost cell) is (1,1) in patch indices
P.nbhrPatch                 = -ones(grid.dim,2);
P.indDel                    = [];
P.deletedBoxes              = [];
grid.level{k}.patch{q}      = P;
if (k > 1)    
    grid.level{k-1}.patch{parentQ}.children = [grid.level{k-1}.patch{parentQ}.children q];
end
if (param.verboseLevel >= 1)
    fprintf('Created level k=%3d patch q=%3d (parentQ = %3d), ilower = [%3d %3d], iupper = [%3d %3d]\n',...
        k,q,parentQ,ilower,iupper);
end

%==============================================================
% 2. Update grid hierarchy
%==============================================================

grid                        = updateGrid(grid);

tCPU        = cputime - tStartCPU;
tElapsed    = etime(clock,tStartElapsed);
if (param.verboseLevel >= 2)
    fprintf('CPU time     = %f\n',tCPU);
    fprintf('Elapsed time = %f\n',tElapsed);
end
if (param.verboseLevel >= 1)
    fprintf('--- addGridPatch(k = %d, q = %d) END ---\n',k,q);
end
