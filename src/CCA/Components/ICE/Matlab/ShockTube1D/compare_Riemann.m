
function [Lnorm] = compare_Riemann(Ncells, t, dx, x_CC, rho_CC, vel_CC, press_CC, temp_CC) 

  exactRiemann = './exactRiemann' 
  %_____________________________________________________________
  % Function: compare_Riemann
  %clear all;
  close all;
  format short e;

  extraCells = 1;

  %__________________________________
  % default user inputs
  symbol   = {'b:+;computed;','*r','xg'}; 
  variable    = 'press_CC';
  makePlot    = true;
  output_file = 'L2norm';


  %______________________________
  % compute the exact solution for each variable
  % The column format is
  %  X    Rho_CC    vel_CC    Press_CC    Temp_CC
  inputFile = sprintf('test1.in');
  c = sprintf('%s %s %s %i %g',exactRiemann, inputFile, 'exactSol', Ncells, t)

  [s, r]   = unix(c);
  exactSol = importdata('exactSol');
  x_ex     = exactSol(:,1);

  %______________________________
  % Load the matlab solution into simSol
  variables = { 'rho_CC' 'vel_CC' 'press_CC' 'temp_CC'};

  interiorCells = 2:length(x_CC) - 1;
  nrows     = length(interiorCells);
  ncols     = length(variables);
  ML_sol    = zeros(length(interiorCells),ncols);


  % load the precomputed matlab solution into ML_sol
  x_CC      = transpose(x_CC);
  rho_CC    = transpose(rho_CC);
  vel_CC    = transpose(vel_CC);
  press_CC  = transpose(press_CC);
  temp_CC   = transpose(temp_CC);
  
  x_CC        = x_CC(interiorCells);
  ML_sol(:,1) = rho_CC(interiorCells);
  ML_sol(:,2) = vel_CC(interiorCells);
  ML_sol(:,3) = press_CC(interiorCells);
  ML_sol(:,4) = temp_CC(interiorCells);

  % bulletproofing
  test = sum (x_CC - x_ex);
  if(test > 1e-10)
    display('ERROR: compute_Riemann: The results cannot be compared')
    return
  end

  % compute the difference/L-norm for each of the variables
  d = zeros(nrows,ncols);
  for v=1:length(variables)
    d(:,v) = ( ML_sol(:,v) - exactSol(:,v+1) );
    L_norm(v) = dx * sum( abs( d(:,v) ) );
  end
  
  Lnorm = L_norm(3);

  % write L_norm to a file
  nargv = length(output_file);
  if (nargv > 0)
    fid = fopen(output_file, 'w');
    for v=1:length(variables)
      fprintf(fid,'%g ',L_norm(v));
    end
    fprintf(fid,'\n');
    fclose(fid);
  end
  
  %write exact data to a file
  fn = sprintf('exact_%g.dat',Ncells);
  fid = fopen(fn, 'w');

  for c=1:length(x_ex)
    fprintf(fid,'%g, ',x_ex(c))
    for v=1:length(variables)
      fprintf(fid,'%g, ',exactSol(c,v));
    end
    fprintf(fid, '\n')
  end

  fclose(fid);


  % Make a plot of 
  %______________________________
  if(makePlot)
    for v=1:length(variables)
      subplot(2,1,1), plot(x_CC,ML_sol(:,v),'b+', x_ex, exactSol(:,v+1),'r');
      xlabel('x')
      legend('exact','computed')
   
      ylabel(variables{v})

      tmp = sprintf('Toro Test (%s) L1 norm: %f, time: %f', inputFile, L_norm(v),t);
      title(tmp);
      grid on;

      subplot(2,1,2),plot(x_ex,d(:,v), 'b:+');
      ylabel('Difference'); 
      xlabel('x');
      grid on;
      fname = sprintf('%g_%s.eps',Ncells,variables{v});
      print ( fname, '-deps');
      % pause
    end
  end

end
