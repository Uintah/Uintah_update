/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


/*
 * FILE: matfile.h
 * AUTH: Jeroen G Stinstra
 * DATE: 5 JAN 2004
 */
 
#ifndef JGS_MATLABIO_MATFILE_H
#define JGS_MATLABIO_MATFILE_H 1

/*
 * The matfile class is the basic interface for reading and
 * writing .mat files (matlab files). This class currently
 * only supports the so called matlab version 5 file format
 * which is used from version 5.0 to version 6.5, currently 
 * the lastest version of matlab available. Matlab V4 files
 * should be converted using matlab into the newer file format.
 *
 * This class handles the following aspects:
 * - opening and closing .mat files
 * - handling byteswapping
 * - reading/writing the file header
 * - reading/writing the tags in the .mat file
 *
 */


/*
 * CLASS DESCRIPTION
 * This class is an interface to a matfile and handles the raw fileformat
 * it deals with headers, byteswapping and reading and writing tags and the
 * associated data
 *
 * MEMORY MODEL
 * The class maintains its own copies of the data. Each vector, string and other
 * data unit is copied.
 * Large quantities of data are shipped in and out in matfiledata objects. These
 * objects are handles to memoryblockss and maintain their own data integrity. 
 * When copying a matfiledata object only pointers are copied, however all information
 * for freeing the object is stored inside and no memory losses should occur.
 *
 * ERROR HANDLING
 * All errors are reported as exceptions described in the matfilebase class.
 * Errors of external c library functions are caught and forwarded as exceptions.
 *
 * COPYING/ASSIGNMENT
 * Do not copy the object, this will lead to errors 
 *
 * RESOURCE ALLOCATION
 * Files are closed by calling close() or by destroying the object
 *
 */

#include <stdio.h>
#include <zlib.h>

#include <sgi_stl_warnings_off.h>
#include <string>
#include <stack>
#include <iostream> 
#include <sgi_stl_warnings_on.h>


#include "matfilebase.h"
#include "matfiledata.h" 
 
namespace MatlabIO {
       
class matfile : public matfilebase {

  private:
  
	// typedef of a struct to use for indexing where we are in
	// a file.
	
	struct matfileptr {
            long	hdrptr;		// location of tag header
            long	datptr;		// location of data segment
            long	startptr;	// location of the first tag header (to go one level up)
            long	endptr;		// location of the end of the data segment (end+1)
            long	size;		// length of data segment
            mitype type;
            };

	// This next struct is used for buffering sections of the compressed
	// data chuncks in V7 of matlab.
	// The basic idea is to decompress compressed pieces of the files and maintain a
	// list of the pieces that have been compresse
	// Using the offset in the file these pieces can be unique identified and cataloged
	

	struct compressbuffer {
			matfiledata mbuffer;	// Buffer with reference counting
			// char	*buffer;	// buffer with uncompressed data
			long	buffersize; // size of the buffer;
			long	bufferoffset; // file offset of the buffer
			};

	struct mxfile {

			long		ref_;			// Reference counter
			
			// The file can be read in two modes
			// 1) directly out of the file using the fptr_
			// 2) out of a decompressed buffer
			//
			// If fcmpbuffer_ is not equal to zero the reading is 
			// supposed to happen in the uncompressed buffer
			// The three remaining counters log where in the
			// file data has to be read
			// fcmpsize_ makes sure that no data is read going beyong the chunck of
			// unpressed data
			// fcmpoffset_ marks at which file offset the data went from uncompressed
			// raw data into a compressed data chunck, so seek can find the proper position
			// fcmpcount_ counts the number of bytes read for the mfread and mfwrite calls
			
			char		*fcmpbuffer_;   // Compression buffer
			matfiledata fcmpmbuffer_;   // Same buffer but wrapped with my memory management system
			long		fcmpsize_;		// Size of the buffer
			long		fcmpoffset_;	// Offset of the buffer
			long		fcmpcount_;		// Counter to check where next to read data
      long    fcmpalignoffset_;    // Correction for alignment problem in filess
			
			FILE		*fptr_;			// File pointer
			std::string fname_;			// Filename
			std::string fmode_;			// File access mode: "r" or "w"
        
			long	    flength_;		// File length	
        
			char	    headertext_[118]; 	// The text in the header of the matfile
			int32_t		subsysdata_[2];		// NEW IN VERSION 7
			short 	  version_;			// Version of the matfile.
			short     byteswap_;			// =1 if bytes need to be swapped
      bool      compressmode_;  // whether we are in compressed mode
    
			std::stack<matfileptr> ptrstack_;	// Stack containing information on where we are
												// currently reading and writing, the stack contains
												// all te pointers to locations in the file, for
												// the parent levels of the current one
												// A matfile is like a directory (tree structure)
			matfileptr curptr_;					// current pointer
            
			// The next list contains the compressed buffers that were allocated
			// Hence compressed variables have to be decompressed only once, which
			// should boost the performance, but will cost memory
			
			std::deque<compressbuffer> cmplist_;	// maintain a list of segments that have already been decompressed
			};
			
	mxfile *m_;
										                            
   	// private functions;
	
	// When reading data this function swaps the bytes
	// To further optimize the performance, loops should be
	// unrolled in this function.
	// currently it only supports certain element sizes
   	void mfswapbytes(void *buffer,long elsize,long size);
	
	// test byte swapping
	bool byteswap();
	bool byteswapmachine();
	
	// Reading the matlab-file header
	// This header contains text (126 bytes) explaining the origin of the data
	// Can be an arbritrary text
	 
   	void mfreadheader();
	void mfwriteheader();
	
	// two versions of reading and writing algorithms that deal with the byte swapping
	// issue. Actually only the reader does byte swapping, but this way there is a 
	// consistent interface.
	// The offset version start reading at an certain location (includes a fseek at the start)
	  
  	void mfread(void *buffer,long elsize,long size);	// read data and do byte swapping
	void mfread(void *buffer,long elsize,long size,long offset);
   	
	void mfwrite(void *buffer,long elsize,long size);
	void mfwrite(void *buffer,long elsize,long size,long offset); 

  public:
  	// constructors
  	matfile(std::string filename,std::string mode);
  	matfile();
  	
	// destructors
	virtual ~matfile();
    
	matfile(const matfile &m); // copy constructor
	matfile& operator= (const matfile &m); // assignment	
			    
  	// open and close file
  	virtual void open(std::string filename,std::string mode);
  	virtual void close();

  	// matfile header
  	void 	    setheadertext(std::string text);
 	std::string getheadertext();
 	
	// read/write data to file
        	
	void readtag(matfiledata& mfd);
	void readdat(matfiledata& mfd);
        
	void writetag(matfiledata& mfd);
	void writedat(matfiledata& mfd);

	
	// openchild:
	// opens the current datablock as a substructure
	// it creates a new pointer and limits access to the file to
	// that datablock. All functions read now in the substructure
	// closechild:
	// return from substructure and restore all file pointers
	
	bool openchild();
	void closechild();
    
	// open and close a compressed section of data
	// Compression is only used for reading matlab V7 files
	// Data is exported as non compressed data to be compatible with
	// version 5 and 6.
	
	bool opencompression();
	void closecompression();	
			    
	// navigation through file:
	// firsttag:
	//   go back to the first data block
	// nexttag:
	//   go to the next data block
	// gototag:
	//   goto a previous marked block
	// firsttag and nexttag return a number indicating 
	// where the datablock is in the file (offset in file)
	// if they return 0, there is no datablock.
	// rewind:
	//  Go to the first tag at the top level
	 
	long firsttag();
	long nexttag();
	long gototag(long tag);
	void rewind();
		       
	// A quick test to see what kind of access to the
	// file is used. Some functions only work for reading and
	// others for writing, these function allows for checking
	// the proper access
	
	bool isreadaccess();
	bool iswriteaccess();
};
 
} // end namespace 
#endif 
