function q = setBoundaryConditions(q,whichvar)
%SETBOUNDARYCONDITIONS Set the boundary condition for one function.
%   Q = SETBOUNDARYCONDITIONS(Q,WHICHVAR) sets the ghost cells for the
%   quantity Q (updated in the return value), based on the boundary
%   condition at the near domain boundary. WHICHVAR is a string specifing
%   which variable is Q. Options are:
%
%   WHICHVAR = 'XVEL_CC'    Q = x-component of the velocity at cell centers
%   WHICHVAR = 'TEMP_CC'    Q = temperature at cell centers
%   WHICHVAR = 'RHO_CC'     Q = density at cell centers
%   WHICHVAR = 'PRESS_CC'   Q = pressure at cell centers
%   
%   See also ?.

globalParams;                               % Load global parameters

switch (lower(whichvar))
    %______________________________________________________________________
    % CC Variables
    
case 'rho_cc'                               % Homogeneous Neumann B.C. for rho at left boundary
    q(ghost_Left)      = q(ghost_Left+1);   % Left boundary
    q(ghost_Right)     = q(ghost_Right-1);  % Right boundary
    
case 'xvel_cc'                              % Homogeneous Dirichlet B.C. for u at left boundary    
    q(ghost_Left)     = q(ghost_Left+1);    % Left boundary               
    q(ghost_Right)    = q(ghost_Right-1);   % Right boundary              
    
case 'temp_cc'                              % Homogeneous Neumann B.C. for T at left boundary
    q(ghost_Left)   = q(ghost_Left+1);      % Left boundary
    q(ghost_Right)  = q(ghost_Right-1);     % Right boundary
    
case 'press_cc'                             % Homogeneous Neumann B.C. for p at left boundary
    q(ghost_Left)   = q(ghost_Left+1);      % Left boundary
    q(ghost_Right)  = q(ghost_Right-1);     % Right boundary

    %______________________________________________________________________
    % FC Variables
    
case 'xvel_fc'                              % Homogeneous Dirichlet B.C. for u at face centers
    %q(ghost_Left)   =q(ghost_Left+1);       % Left boundary. This is a FIRST ORDER approximation! (could be bad)               
    %q(ghost_Right)  =q(ghost_Right-1);      % Right boundary. This is a FIRST ORDER approximation! (could be bad)              
    
otherwise
    error('setBoundaryConditions: unknown variable');
end
