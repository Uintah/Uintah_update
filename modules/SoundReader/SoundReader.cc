
/*
 *  SoundReader.cc:
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   March 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <SoundReader/SoundReader.h>
#include <Math/MinMax.h>
#include <ModuleList.h>
#include <MUI.h>
#include <NotFinished.h>
#include <SoundPort.h>
#include <iostream.h>
#include <audiofile.h>
#include <fstream.h>

static Module* make_SoundReader()
{
    return new SoundReader;
}

static RegisterModule db1("Sound", "SoundReader", make_SoundReader);

SoundReader::SoundReader()
: UserModule("SoundReader", Source)
{
    // Create the output data handle and port
    osound=new SoundOPort(this, "Sound", SoundIPort::Stream);
    add_oport(osound);

    // Set up the interface
    add_ui(new MUI_file_selection("Sound File", &filename,
				  MUI_widget::NotExecuting));
}

SoundReader::SoundReader(const SoundReader& copy, int deep)
: UserModule(copy, deep)
{
    NOT_FINISHED("SoundReader::SoundReader");
}

SoundReader::~SoundReader()
{
}

Module* SoundReader::clone(int deep)
{
    return new SoundReader(*this, deep);
}

void SoundReader::execute()
{
    // Open the sound port...
#if 0
    AFfilehandle afile=AFopenfile(filename(), "r", 0);
    if(!afile){
	cerr << "Error opening file: " << afile << endl;
	return;
    }
    long nchannels=AFgetchannels(afile, AF_DEFAULT_TRACK);
    // Setup the sampling rate...
    double rate=AFgetrate(afile, AF_DEFAULT_TRACK);
    osound->set_sample_rate(rate);

    long nsamples=AFgetframecnt(afile, AF_DEFAULT_TRACK);
    osound->set_nsamples((int)nsamples);
    long sampfmt, sampwidth;
    AFgetsampfmt(afile, AF_DEFAULT_TRACK, &sampfmt, &sampwidth);
    double mx=1./double(1<<(sampwidth-1));
    mx/=double(nchannels);
    int which;
    if(sampwidth <= 8){
	which=0;
    } else if(sampwidth <= 16){
	which=1;
    } else {
	which=2;
    }
#endif
#if 1
    double rate=8000;
    osound->set_sample_rate(rate);
    ifstream in(filename());
    int nsamples=int(10*rate);
#endif
#if 0
    int sample=0;
    int size=(int)(rate/20);
    signed char* sampc=new char[size*nchannels];
    short* samps=new short[size*nchannels];
    long* sampl=new long[size*nchannels];
    int done=0;
    switch(which){
    case 0:
	break;
    case 1:
	while(!done){
	    long status=AFreadframes(afile, AF_DEFAULT_TRACK,
				     (void*)samps, size);
	    if(status==0){
		done=1;
	    } else if(status < 0){
		error("Error in AFreadsamps");
		done=1;
	    } else {
		for(int i=0;i<status;i++){
		    double s=0;
		    for(int j=0;j<nchannels;j++)
			s+=double(samps[i+j])*mx;
		    osound->put_sample(s);
		}
		sample+=status;
		update_progress(sample++, (int)nsamples);
	    }
	}
	break;
    case 2:
	break;
#endif
#if 1
	int sample=0;
	while(1){
	    signed char c1;
	    in.get(c1);
	    if(!in)break;
	    double s=c1/128.0;
	    osound->put_sample(s);
	    
	    // Tell everyone how we are doing...
	    update_progress(sample++, (int)nsamples);
	    if(sample >= nsamples)
		sample=0;
#endif
    }
}

void SoundReader::mui_callback(void*, int)
{
    want_to_execute();
}

