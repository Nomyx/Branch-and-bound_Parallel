/*
  Branch and bound algorithm to find the minimum of continuous binary 
  functions using interval arithmetic.

  Sequential version

  Author: Frederic Goualard <Frederic.Goualard@univ-nantes.fr>
  v. 1.0, 2013-02-15
*/

#include <iostream>
#include <iterator>
#include <string>
#include <stdexcept>
#include "interval.h"
#include "functions.h"
#include "minimizer.h"
#include <omp.h>

using namespace std;



// Sorting function  
void tri_a_bulle( double *t, int n ){ 
	int j = 0;
	double tmp = 0;
	int en_desordre = 1; // C bool
	while ( en_desordre )
	{
		en_desordre = 0; 
		for ( j =0; j < n-1; j++ ){  
			if (t[j] > t[j+1]){
				tmp = t[j+1];
				t[j+1] = t[j];
				t[j] = tmp;
				en_desordre = 1;
			}
		}
	}
}


// Split a 2D box into four subboxes by splitting each dimension
// into two equal subparts
void split_box(const interval& x, const interval& y,
	       interval &xl, interval& xr, interval& yl, interval& yr)
{
  double xm = x.mid();
  double ym = y.mid();
  xl = interval(x.left(),xm);
  xr = interval(xm,x.right());
  yl = interval(y.left(),ym);
  yr = interval(ym,y.right());
}

// Branch-and-bound minimization algorithm
void minimize(itvfun f,  // Function to minimize
	      const interval& x, // Current bounds for 1st dimension
	      const interval& y, // Current bounds for 2nd dimension
	      double threshold,  // Threshold at which we should stop splitting
	      double& min_ub,  // Current minimum upper bound
	      minimizer_list& ml) // List of current minimizers
{
  interval fxy = f(x,y);
  
  if (fxy.left() > min_ub) { // Current box cannot contain minimum?
    return ;
  }

  if (fxy.right() < min_ub) { // Current box contains a new minimum?
    min_ub = fxy.right();
    // Discarding all saved boxes whose minimum lower bound is 
    // greater than the new minimum upper bound
    auto discard_begin = ml.lower_bound(minimizer{0,0,min_ub,0});
    ml.erase(discard_begin,ml.end());
  }

  // Checking whether the input box is small enough to stop searching.
  // We can consider the width of one dimension only since a box
  // is always split equally along both dimensions
  if (x.width() <= threshold) { 
    // We have potentially a new minimizer
    ml.insert(minimizer{x,y,fxy.left(),fxy.right()});
    return ;
  }

  // The box is still large enough => we split it into 4 sub-boxes
  // and recursively explore them
  interval xl, xr, yl, yr;
  split_box(x,y,xl,xr,yl,yr);


// prepare reduction over min_ub# 
// double min_ub0, min_ub1, min_ub2, min_ub3;

// #pragma omp parallel sections reduction (MIN:min_ub)

#pragma omp parallel sections
{
		#pragma omp section
		#pragma omp critical
		minimize(f,xl,yl,threshold,min_ub,ml);
		#pragma omp section
		#pragma omp critical
		minimize(f,xl,yr,threshold,min_ub,ml);
		#pragma omp section
		#pragma omp critical
		minimize(f,xr,yl,threshold,min_ub,ml);
		#pragma omp section
		#pragma omp critical
		minimize(f,xr,yr,threshold,min_ub,ml);
}


/*
// equivalent : reduction (MIN:min_ub) 
double * min_ub_tab = new double[4];

min_ub_tab[0] = min_ub0;
min_ub_tab[1] = min_ub1;
min_ub_tab[2] = min_ub2;
min_ub_tab[3] = min_ub3;

cout << "[";
for ( int i = 0 ; i < 4 ; i++ )
{
	cout << " " << min_ub_tab[i];
}
cout << " ]" << endl;

tri_a_bulle( min_ub_tab, 4);

min_ub = min_ub_tab[0];

delete[] min_ub_tab;
*/

}


int main(void)
{
//#ifdef _OPENMP
	omp_set_num_threads( 4 );
	omp_set_max_active_levels( 2 );
//#endif
	
  cout.precision(16);
  // By default, the currently known upper bound for the minimizer is +oo
  double min_ub = numeric_limits<double>::infinity();
  // List of potential minimizers. They may be removed from the list
  // if we later discover that their smallest minimum possible is 
  // greater than the new current upper bound
  minimizer_list minimums;
  // Threshold at which we should stop splitting a box
  double precision;

  // Name of the function to optimize
  string choice_fun;

  // The information on the function chosen (pointer and initial box)
  opt_fun_t fun;
  
  bool good_choice;

  // Asking the user for the name of the function to optimize
  do {
    good_choice = true;

    cout << "Which function to optimize?\n";
    cout << "Possible choices: ";
    for (auto fname : functions) {
      cout << fname.first << " ";
    }
    cout << endl;
    cin >> choice_fun;
    
    try {
      fun = functions.at(choice_fun);
    } catch (out_of_range) {
      cerr << "Bad choice" << endl;
      good_choice = false;
    }
  } while(!good_choice);

  // Asking for the threshold below which a box is not split further
  cout << "Precision? ";
  cin >> precision;
  
  minimize(fun.f,fun.x,fun.y,precision,min_ub,minimums);
  
  // Displaying all potential minimizers
  copy(minimums.begin(),minimums.end(),
       ostream_iterator<minimizer>(cout,"\n"));    
  cout << "Number of minimizers: " << minimums.size() << endl;
  cout << "Upper bound for minimum: " << min_ub << endl;
}
