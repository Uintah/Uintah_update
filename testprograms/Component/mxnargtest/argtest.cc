/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in compliance
  with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.
  
  The Original Source Code is SCIRun, released March 12, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
  University of Utah. All Rights Reserved.
*/


/*
 *  pingpong.cc
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1999
 *
 *  Copyright (C) 1999 U of U
 */

#include <iostream>
#include <mpi.h>
#include <Core/CCA/Component/PIDL/PIDL.h>
#include <Core/CCA/Component/PIDL/MxNArrayRep.h>
#include <Core/CCA/Component/PIDL/MalformedURL.h>
#include <testprograms/Component/mxnargtest/argtest_sidl.h>
#include <Core/Thread/Time.h>

using std::cerr;
using std::cout;
using std::string;
using argtest::Server;
using namespace SCIRun;

void usage(char* progname)
{
    cerr << "usage: " << progname << " [options]\n";
    cerr << "valid options are:\n";
    cerr << "  -server  - server process\n";
    cerr << "  -client URL  - client process\n";
    cerr << "\n";
    exit(1);
}

void init(CIA::array2<int>& a, int sta1, int fin1, int sta2, int fin2)
{
  a.resize(fin1-sta1,fin2-sta2);
  for(int j=sta2;j<fin2;j++)
    for(int i=sta1;i<fin1;i++) {
      a[j-sta2][i-sta1]=(j*100)+i;
//      std::cerr << "SLIDING IN a[" << j-sta2 << "][" << i-sta1 << "] = " << a[j-sta2][i-sta1] << "\n"; 
//      std::cerr << "ARRAY sample arr[0][7] = " << a[0][7] << "\n";
    }
//   std::cerr << "ARRAY sample arr[0][7] = " << a[0][7] << "\n";

}

bool test(CIA::array2<int>& a)
{
  bool success = true;
 
  for(unsigned int i=0;i<a.size1();i++)
    for(unsigned int j=0;j<a.size2();j++)
      if((a[i][j] % 100) != j)
	success = false;

  return success;
}

class Server_impl : public argtest::Server {
public:
  Server_impl();
  virtual ~Server_impl();
	
  virtual CIA::array2< int> return_arr();
  virtual void in_arr(const CIA::array2< int>& a);
  virtual void out_arr(CIA::array2< int>& a);
  virtual void inout_arr(CIA::array2< int>& a);
  virtual bool getSuccess();
private:
  bool success;
};

Server_impl::Server_impl()
{
  success = true;
}

Server_impl::~Server_impl()
{
}

CIA::array2< int> Server_impl::return_arr()
{
}

void Server_impl::in_arr(const CIA::array2< int>& a)
{
}

void Server_impl::out_arr(CIA::array2< int>& a)
{
  init(a,0,100,0,5); 
}

void Server_impl::inout_arr(CIA::array2< int>& a)
{
}

bool Server_impl::getSuccess() 
{
}

int main(int argc, char* argv[])
{
    CIA::array2<int> arr;

    int myrank = 0;
    int mysize = 0;
    int arrsize, sta1, fin1;

    try {
        PIDL::initialize(argc,argv);
        
        MPI_Init(&argc,&argv);

	bool client=false;
	bool server=false;
	vector <URL> server_urls;
	int reps=1;

        MPI_Comm_size(MPI_COMM_WORLD,&mysize);
        MPI_Comm_rank(MPI_COMM_WORLD,&myrank);

	for(int i=1;i<argc;i++){
	    string arg(argv[i]);
	    if(arg == "-server"){
		if(client)
		    usage(argv[0]);
		server=true;
	    } else if(arg == "-client"){
		if(server)
		    usage(argv[0]);
                i++;
                for(;i<argc;i++){		
                  string url_arg(argv[i]);
		  server_urls.push_back(url_arg);
                }
		client=true;
                break;
	    } else if(arg == "-reps"){
		if(++i>=argc)
		    usage(argv[0]);
		reps=atoi(argv[i]);
	    } else {
		usage(argv[0]);
	    }
	}
	if(!client && !server)
	    usage(argv[0]);

	if(server) {
	  Server_impl* serv=new Server_impl;

          //Set up server's requirement of the distribution array 
	  Index** dr = new Index* [2]; 
	  dr[0] = new Index(myrank,99,mysize);
          dr[1] = new Index(myrank,5,mysize);
	  MxNArrayRep* arrr = new MxNArrayRep(2,dr);
	  serv->setCalleeDistribution("D",arrr);

	  cerr << "Waiting for MxN argtest connections...\n";
	  cerr << serv->getURL().getString() << '\n';

	} else {

          //Creating a multiplexing proxy from all the URLs
	  Object::pointer obj=PIDL::objectFrom(server_urls,mysize,myrank);
	  Server::pointer serv=pidl_cast<Server::pointer>(obj);
	  if(serv.isNull()){
	    cerr << "serv_isnull\n";
	    abort();
	  }

	  //Set up the array and the timer  
	  double stime=Time::currentSeconds();
          arrsize = 100 / mysize;
          sta1 = myrank * arrsize;
          fin1 = (myrank * arrsize) + arrsize - 1;
          init(arr,sta1,fin1+1,0,2);
	  std::cerr << "ARRAY sample arr[0][7] = " << arr[0][7] << "\n";
	  //Inform everyone else of my distribution
          //(this sends a message to all the callee objects)
          Index** dr = new Index* [2];
          dr[0] = new Index(sta1,fin1,1);
          dr[1] = new Index(0,1,1);
          MxNArrayRep* arrr = new MxNArrayRep(2,dr);
	  serv->setCallerDistribution("D",arrr); 

          //serv->in_arr(arr);
          serv->out_arr(arr);
          //serv->inout_arr(arr);

	  double dt=Time::currentSeconds()-stime;
	  cerr << reps << " reps in " << dt << " seconds\n";
	  double us=dt/reps*1000*1000;
	  cerr << us << " us/rep\n";
	}
    } catch(const MalformedURL& e) {
	cerr << "pingpong.cc: Caught MalformedURL exception:\n";
	cerr << e.message() << '\n';
    } catch(const Exception& e) {
	cerr << "pingpong.cc: Caught exception:\n";
	cerr << e.message() << '\n';
	abort();
    } catch(...) {
	cerr << "Caught unexpected exception!\n";
	abort();
    }
    PIDL::serveObjects();

    MPI_Finalize();

    return 0;
}

