/* ======================================================================*/
#include "cpgplot.h"
#include "parameters.h"
#include "nrutil+.h"
#include <stdio.h>
#include <math.h>
/* 
 Function:  plot_contour--VISUALIZATION: Generates a contour plot.
 Filename:  plot_contour.c

 Purpose: generate a contour plot

 History:
    Version   Programmer         Date       Description
    -------   ----------         ----       -----------
       1.0     Todd Harman       07/29/99   Written   

 IN args/commons        Units      Description
 ---------------        -----      -----------
 label                  char        label to put on plot
 
  steps:
    1)  calculate the contour levels, and make sure that none of them
        are integers.  If they are then offset them slightly.  This prevents
        white space from appearing.
    2)  Loop through each of the contour levels and plot that contour.
    
Note: For details regarding the pgplot functions see the pgplot documentation
_______________________________________________________________________*/

    void plot_contour( 
    int     xLoLimit,      
    int     xHiLimit,
    int     yLoLimit,
    int     yHiLimit,       
    float   data_min,
    float   data_max,
    const   float   *data )
{            
    float   *contour_levels,
            small_number;
    static float    TR[6] = {-1.0, 1.0, 0.0, -1.0, 0.0, 1.0};

/*     static float    TR[6] = {-0.5, 1.0, 0.0, -0.5, 0.0, 1.0}; */
            
    int     i, 
            xLo, yLo,
            xHi, yHi,
            n_contours,
            color;
    double  test;
/*______________________________________________________________________
*   Define some constants
*_______________________________________________________________________*/
    n_contours  = 100;
    small_number = (data_max - data_min)/n_contours;
    /*__________________________________
    *   If you start at xLo or yLo = 0 then
    *   it doesn't work you need to offset
    *   the indicies by 1
    *___________________________________*/
    xHi       = xHiLimit;
    yHi       = yHiLimit;
    xLo       = xLoLimit;
    yLo       = yLoLimit;
    if (xLoLimit ==0 || yLoLimit ==0)
    {  
        xHi       = xHiLimit+1;
        yHi       = yHiLimit+1;
        xLo       = xLoLimit+1;
        yLo       = yLoLimit+1;
    }
/*__________________________________
*   
*___________________________________*/

    contour_levels  = vector_nr(0, n_contours+2);
/*__________________________________
*   Step 1
*   Set the contour levels
*___________________________________*/
    for (i =2; i <= n_contours -1; i++)
    {
        contour_levels[i] = data_min + 
                            (float)i * (data_max - data_min)/(float)n_contours;
        /*__________________________________
        *   If one of the contour levels has a 
        *   fractional value of 0 then offset
        *   that value slightly
        *___________________________________*/
        test = fmod((double)contour_levels[i], 1.0);
        if(  test == 0.0) 
        {
            contour_levels[i] = contour_levels[i] + small_number;
        }
    }    
    contour_levels[1]           = data_min - small_number;
    contour_levels[n_contours]  = data_max + small_number;
          
/*__________________________________
*   Step 2
*   Now loop through each contour level
*___________________________________*/
    color = 0;   
    for( i = 1; i <= n_contours-1; i++)
    {
        color = (int)(i*NUM_COLORS/n_contours) + 1;
        
        cpgsci(color);
        cpgconf(&data[1],   xHi,  yHi,
                            xLo,  xHi,
                            yLo,  yHi,
                            contour_levels[i], contour_levels[i+1],TR);
    }
/*______________________________________________________________________
*   Deallocate the memory
*_______________________________________________________________________*/
    free_vector_nr(contour_levels, 0, n_contours+2);
    
}
/*STOP_DOC*/



/* ======================================================================*/
#include "cpgplot.h"
#include "parameters.h"
#include "nrutil+.h"
#include <stdio.h>
#include <math.h>
/* 
 Function:  plot_contour_checkerboard--VISUALIZATION: Generates a checkerboard contour plot.
 Filename:  plot_contour.c

 Purpose: generates a checker board contour plot that represents the
 cell-centered values as squares of a certain color

 History:
    Version   Programmer         Date       Description
    -------   ----------         ----       -----------
       1.0     Todd Harman       11/17/99   Written   
 
  steps:
    1)  calculate the contour levels, and make sure that none of them
        are integers.  If they are then offset them slightly.  This prevents
        white space from appearing.
    2)  Loop through each of the contour levels and draw rectangle.
    
Note: For details regarding the pgplot functions see the pgplot documentation
_______________________________________________________________________*/

    void plot_contour_checkerboard( 
    int     xLoLimit,      
    int     xHiLimit,
    int     yLoLimit,
    int     yHiLimit,       
    float   data_min,
    float   data_max,
    const   float   *data )
{            
    float   *contour_levels,
            small_number;
            
    int     i, j, c, counter,
            n_contours,
            color;
    float   x1, x2, y1, y2,
            scaleX, scaleY;
    double  test;
/*______________________________________________________________________
*   Define some constants
*_______________________________________________________________________*/
    
    scaleX      = 1.0 - 1.0/xHiLimit;
    scaleY      = 1.0 - 1.0/yHiLimit;
    n_contours  = 100;
    small_number = (data_max - data_min)/n_contours;
/*__________________________________
*   
*___________________________________*/

    contour_levels  = vector_nr(0, n_contours+2);
/*__________________________________
*   Step 1
*   Set the contour levels
*___________________________________*/
    for (i =2; i <= n_contours -1; i++)
    {
        contour_levels[i] = data_min + 
                            (float)i * (data_max - data_min)/(float)n_contours;
        /*__________________________________
        *   If one of the contour levels has a 
        *   fractional value of 0 then offset
        *   that value slightly
        *___________________________________*/
        test = fmod((double)contour_levels[i], 1.0);
        if(  test == 0.0) 
        {
            contour_levels[i] = contour_levels[i] + small_number;
        }
    }    
    contour_levels[1]           = data_min - small_number;
    contour_levels[n_contours]  = data_max + small_number;
          
/*__________________________________
*   Step 2
*   Now loop through each contour level
*___________________________________*/
    color = 0;   
    for( c = 1; c <= n_contours-1; c++)
    {

        color   = (int)(c*NUM_COLORS/n_contours) + 1;
        cpgsci(color);
        counter = 0;

        for(j = yLoLimit; j <= yHiLimit; j++)
        {
            for(i = xLoLimit; i <= xHiLimit; i++)
            {
                counter ++;
                if ((data[counter] >= contour_levels[c]) && (data[counter] <= contour_levels[c+1]))
                {
                    x1 = (float) scaleX* i;
                    x2 = (float) scaleX* (i+1);
                    y1 = (float) scaleY* (j);
                    y2 = (float) scaleY* (j+1);
                    cpgrect(x1, x2, y1, y2);
                }
            }
        }
    }

/*______________________________________________________________________
*   Deallocate the memory
*_______________________________________________________________________*/
    free_vector_nr(contour_levels, 0, n_contours+2); 
}
/*STOP_DOC*/
