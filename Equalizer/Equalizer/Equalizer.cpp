//
//  main.cpp
//  Equalizer
//
//  Created by Dale on 5/9/22.
//
#include "Equalizer.hpp"

#include <wx/fileconf.h>

// c++ -std=c++11 -o Equalizer.x Equalizer.cpp -lGLEW `/usr/local/bin/wx-config --cxxflags --libs --gl-libs` -lGL -lGLU
// c++ -std=c++11 -o Equalizer.x Equalizer.cpp -lGLEW `/usr/local/bin/wx-config --cxxflags --libs --gl-libs` -Wno-deprecated-declarations


static int doLoopFiles=0;
static int fileLoop=0;


int sound( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData );

class sTone{
public:
    sTone(double frequency,double sampleRate);
    int doTone(short int *buffer,int frames);
	~sTone();

	float dt;
	float sino;
	float coso;
	float sindt;
	float cosdt;
};
sTone::~sTone()
{
	printf("~sTone\n");
}
sTone::sTone(double frequency,double sampleRate)
{
	double pi;
    pi=4.0*atan(1.0);
    dt=1.0/((double)sampleRate);
    sino=0;
    coso=1;
    double w=2.0*pi*(frequency);
    sindt=sin(w*dt);
    cosdt=cos(w*dt);
}

int sTone::doTone(short int *buffer,int frames)
{

	if(frames <= 0)return 1;

	for(int k=0;k<frames;++k){
		double sint=sino*cosdt+coso*sindt;
		double cost=coso*cosdt-sino*sindt;
		coso=cost;
		sino=sint;
		double v=32000*cost;
		if(v > 32000)v=32000;
		if(v < -32000)v=-32000;
		buffer[k]=(short)(v);
	}


	double r=sqrt(coso*coso+sino*sino);
	coso /= r;
	sino /= r;

	return 0;
}

class sourceBase{
public:
    virtual int doSource(short int *buffer,int frames);
};
int sourceBase::doSource(short int *buffer,int frames)
{
	//printf("doSource Base\n");
	return 1;
}
class sourceFile : public sourceBase{
public:
    virtual int doSource(short int *buffer,int frames);
    virtual ~sourceFile(){}
 	SNDFILE *infile;
};

int sourceFile::doSource(short int *buffer,int frames)
{
	//printf("doSource sourceFile\n");
	int readcount = (int)sf_readf_short(infile,(short *)buffer,frames);
	return readcount;
}

class sourceTone : public sourceBase{
public:
    virtual int doSource(short int *buffer,int frames);
    virtual ~sourceTone(){}
    sTone *tone;
};

int sourceTone::doSource(short int *buffer,int frames)
{
	//printf("doSource sourceTone\n");
	tone->doTone(buffer,frames);
	return frames;
}

class MyFrame;

class outputBase{
public:
    virtual int doOutput(float *buf1,float *buf2,int *kn);
};
int outputBase::doOutput(float *buf1,float *buf2,int *kn)
{
	//printf("dooutput Base\n");
	return 1;
}



 
struct Filters2{
    int np;
    ampmodem demodAM;
    freqdem demod;
    msresamp_crcf iqSampler;
    msresamp_rrrf iqSampler2;
    iirfilt_crcf dcFilter;
    iirfilt_crcf lowpass;
    int thread;
    float *buf1;
    float *buf2;
    float *out2;
    double amHistory;
    int short *data;
    int extraBytes;
    class Poly *p[10];
    float filterFrequencies[10];
    int countFilter;
};

class MyFrame : public wxFrame
{

public:
    MyFrame(wxFrame *frame, const wxString& title, const wxPoint& pos,
            const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);
	~MyFrame();
    void OnMenuFileOpen(wxCommandEvent& event);
    void OnMenuFileExit(wxCommandEvent& event);
    void OnMenuHelpAbout(wxCommandEvent& event);
    void OnMenuAudio(wxCommandEvent& event);
    void OnMenuRecentMenu(wxCommandEvent& event);
    void doLoop(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnScroll(wxCommandEvent& event);
    void DeleteRow( wxCommandEvent& event );
    void OpenFile( wxString filename);
    void resized(wxSizeEvent& evt);
    int setFilters();
    int SetGauges();
    int sound( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status);
    wxMenu *SetAudio();
    wxSlider *slider[10];
    wxGauge *g[10];
	int channelCount;
	wxPanel *panel;
	wxPanel *panel2;
	wxButton *button2;
	wxButton *button;
	int audiodevice;
	RtAudio *dac;
	float samplerate;
	float faudio;
	int idLast;
 	int ncut;
 	int nchannels;
 	class sTone *tone;
 	struct Filters2 f;
	
 	SNDFILE *infile;
    SF_INFO sfinfo;
    wxSlider *sliderTime;
    wxSlider *sliderGain;
    float gain;
    volatile int wait;
    long long CurrentFrame;
    wxString fileList[10];
    int fileCount;
    wxMenu *subMenu;
    wxMenu *playMenu;
    wxMenu *subMenu2;
	class sourceBase *source;
	class sourceFile *sf;
  	class sourceTone *st;
 	
	class outputBase *output;
	class outputFile *of;
	class outputTone *ot;

private:

    
    wxDECLARE_EVENT_TABLE();
};

class outputFile : public outputBase{
public:
    virtual int doOutput(float *buf1,float *buf2,int *kn);
    virtual ~outputFile(){}
 	class MyFrame *f;
};

int outputFile::doOutput(float *buf1,float *buf2,int *kni)
{

	int kn=*kni;
	
	unsigned int num=(unsigned int)kn;
	unsigned int num2=0;
	if(f->f.iqSampler2){
		msresamp_rrrf_execute(f->f.iqSampler2, (float *)buf1, num, (float *)buf2, &num2);  // interpolate
		for(unsigned int n=0;n<num2;++n){
			buf1[n] = buf2[n];
		}
	//	printf("num %d num2 %d\n",num,num2);
		kn=num2;
	}


	for(int n=0;n<f->f.countFilter;++n){
		if(n==0){
			f->f.p[n]->forceCascadeRun(buf1,buf2,kn,0);
		}else{
			f->f.p[n]->forceCascadeRun(buf1,buf2,kn,1);
		}
	}

	for(int n=0;n<kn;++n){
		buf1[n] = buf2[n];
	}

	*kni=kn;

	return 0;
}

class outputTone : public outputBase{
public:
    virtual int doOutput(float *buf1,float *buf2,int *kn);
    virtual ~outputTone(){}
};

int outputTone::doOutput(float *buf1,float *buf2,int *kn)
{
	//printf("dooutput outputTone\n");
	return 0;
}


class MyApp: public wxApp
{
    virtual bool OnInit();
	MyFrame *frame;
          
public:

};


bool MyApp::OnInit()
{
	if (!wxApp::OnInit())
	return false;

	frame = new MyFrame(NULL, "Equalizer",
				 wxPoint(50,50), wxSize(700,600));
				 
	return true;
}

wxIMPLEMENT_APP(MyApp);


// ---------------------------------------------------------------------------
// MyFrame
// ---------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(wxID_OPEN, MyFrame::OnMenuFileOpen)
    EVT_MENU(wxID_EXIT, MyFrame::OnMenuFileExit)
    EVT_MENU(wxID_HELP, MyFrame::OnMenuHelpAbout)
    EVT_MENU(wxID_ABOUT, MyFrame::OnMenuHelpAbout)
    EVT_MENU_RANGE(AUDIO_MENU,AUDIO_MENU+1000,MyFrame::OnMenuAudio)
    EVT_MENU_RANGE(OPEN_RECENT,OPEN_RECENT+11,MyFrame::OnMenuRecentMenu)
    EVT_MENU(PLAYBACK_MENU, MyFrame::doLoop)
    EVT_SLIDER( SCROLL_01,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_02,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_03,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_04,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_05,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_06,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_07,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_08,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_09,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_10,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_TIME,MyFrame::OnScroll)
    EVT_SLIDER( SCROLL_GAIN,MyFrame::OnScroll)
    EVT_BUTTON(ID_PLAY,MyFrame::DeleteRow )
    EVT_BUTTON(ID_PAUSE,MyFrame::DeleteRow )
    EVT_SIZE(MyFrame::resized)
    EVT_IDLE(MyFrame::OnIdle)


wxEND_EVENT_TABLE()


void MyFrame::OnIdle(wxIdleEvent& event)
{
	
	for(int n=0;n<f.countFilter;++n){
		if(f.p[n] && f.p[n]->vmax > 0){
		    if(g[n]){
		    	//float value=(f.p[n]->runningSum);
		    	float value=(f.p[n]->vmax);
		    	if(value <= 1.0)value=1.0;
		    	value=log10(value);
		    	value=100*value/4.5;
		    	//printf("n %d value %g sum %g\n",n,value,f.p[n]->runningSum);
				int range=g[n]->GetRange();
				if(value > range)value=range;
		    	g[n]->SetValue(value);
		    }
		
		}  
		Refresh();
	}
	
	if(sliderTime){
		if(sfinfo.samplerate > 0){
			sliderTime->SetValue(CurrentFrame/sfinfo.samplerate);
		}else{
			sliderTime->SetValue(0);
		}
	}
	
	if(doLoopFiles == 2){
		doLoopFiles=1;
		if(fileLoop >= fileCount){
			fileLoop=0;
		}
		OpenFile(fileList[fileLoop]);
		++fileLoop;
	}
	
}

MyFrame::~MyFrame()
{
	// printf("~MyFrame\n");
	
	try {
	// Stop the stream
		dac->stopStream();
	}
	catch (RtAudioError& e) {
		e.printMessage();
	}
	if ( dac->isStreamOpen() ) dac->closeStream();
	
    if(f.buf1)free((char *)f.buf1);
    
	if(f.buf2)free((char *)f.buf2);

    if(f.out2)free((char *)f.out2);
    
    if(f.data)free((char *)f.data);
    
    for(int n=0;n<10;++n){
    	if(f.p[n])delete f.p[n];
    	f.p[n]=NULL;
    }
	
	if(tone)delete tone;
	tone=NULL;
	
	delete dac;
	
	wxFileConfig cfg(wxT("Equalizer"), wxT("dir"));

	for(int n=0;n<fileCount;++n){
		wxString ns = wxString::Format(wxT("%d"),n);
		if ( cfg.Write("File"+ns, fileList[n]) )
		{
			//printf("write %s\n",fileList[n].ToUTF8().data());
		}else{
			//printf("write false\n");
		}
	}
	
	for(int n=fileCount;n<10;++n){
		wxString ns = wxString::Format(wxT("%d"),n);
		if ( cfg.DeleteEntry("File"+ns) )
		{
			//printf("do delete OK\n");
		}else{
			//printf("do delete) false\n");
		}
	}
	
	if(sf)delete sf;
	if(st)delete st;
	if(of)delete of;
	if(ot)delete ot;
	
}

int MyFrame::sound( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status)
{
	
	if ( status )printf("Stream underflow detected!\n");

	short int *buffer = (short int *) outputBuffer;

	float *buf1= (float *) f.buf1;
	short *sbuf1=(short *) f.buf1;
	float *buf2=(float *) f.buf2;
	float *out2=(float *) f.out2;
	
    int tosend=nBufferFrames;
    
    if(!infile || wait){
		for (unsigned int i=0; i<nBufferFrames; i++ ) {
			  *buffer++ = 0;
		}
		return 0;    
    }
   // printf("1 extraBytes %d tosend %d\n",f->f.extraBytes,tosend);
    if(f.extraBytes > 0){
    	if(tosend <= f.extraBytes){
   			for (int n=0; n<tosend; n++ ) {
				  buffer[n]= (short)out2[n];
			}
			int nn=0;
   			for (int n=tosend; n<f.extraBytes; n++ ) {
				  out2[nn++]= out2[n];
			}
			f.extraBytes -= tosend;
			return 0;
    	}else{
			for (int n=0; n<f.extraBytes; n++ ) {
			  	buffer[n]= (short)out2[n];
			}
			tosend -= f.extraBytes;
			buffer +=  f.extraBytes;
			f.extraBytes=0;
    	}
    
    }
    
   // printf("2 extraBytes %d tosend %d\n",f->f.extraBytes,tosend);
    
	while(1){
  		int readcount=0;
  		if(infile){
  			//readcount = (int)sf_readf_short(infile,(short *)buf1,4800);
  			
  			readcount=source->doSource((short *)buf1,4800);
  					
  			//printf("readcount %d\n",readcount);
  			if(readcount <= 0){
					//printf("1 doLoopFiles %d readcount %d\n",doLoopFiles,readcount);
				if(doLoopFiles){
				    doLoopFiles=2;
					//printf("2 doLoopFiles readcount %d\n",readcount);
					return 1;
				}else{
					sf_seek(infile, 0, SEEK_SET);
					readcount = (int)sf_readf_short(infile,(short *)buf1,4800);
					CurrentFrame=0;
    			}
			}
  			if(readcount <= 0){
  				break; // should never happen
  			}
  			
  			CurrentFrame += readcount;

			int k=readcount;
		
	//		printf("gain %g k %d\n",f->gain,k);
			
			for(int n=(int)(k-1);n >= 0;--n){
				if(nchannels == 2){
					buf1[n]=gain*(sbuf1[2*n]+sbuf1[2*n+1])*0.5;
				}else{
					buf1[n]=gain*sbuf1[n];
				}
    		}
    		
    	//	printf("3 k %d tosend %d\n",k,tosend);

			int kn=k;
			
			output->doOutput(buf1,buf2,&kn);

			
    		//printf("4 kn %d tosend %d\n",kn,tosend);
    		
			if(tosend-kn <= 0){
				for (int n=0; n<tosend; n++ ) {
			  		buffer[n]= (short)buf1[n];
//			  		printf("n %d buffer[n] %d\n",n,buffer[n]);
			  	}
			    if(kn - tosend > 0){
					unsigned int nn=0;
					for (int n=tosend; n<kn; n++ ) {
						  out2[nn++]= buf1[n];
					}
					f.extraBytes=nn;
				}else{
					f.extraBytes=0;
				}
			  	return 0;
			}else{
				for (int n=0; n<kn; n++ ) {
			  		buffer[n]= (short)buf1[n];
			  	}
			  	buffer += kn;
			  	tosend -= kn;
			}			
  		}else{
			for (unsigned int i=0; i<nBufferFrames; i++ ) {
				  *buffer++ = 0;
			}
			return 0;
  		}
  		
	} 		
  	
	return 0;
}
int sound1( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
         double streamTime, RtAudioStreamStatus status, void *userData )
{
	MyFrame *f=(MyFrame *)userData;

	return f->sound(outputBuffer,inputBuffer,nBufferFrames,
         streamTime,status);
}
int MyFrame::setFilters()
{
    float As = 60.0f;
    
    //float ratio= (float)(sfinfo.samplerate/samplerate);

    float ratio= (float)(samplerate/(float)sfinfo.samplerate);

    //printf("ratio %f file samplerate %d output samplerate %f \n",ratio,sfinfo.samplerate,samplerate);
    
    f.iqSampler2 = msresamp_rrrf_create(ratio, As);
	
    double rate=samplerate;
    
    int size=(int)(rate/10.0);
        
    size = 48000.0*3;
    if(ratio > 1.0)size = (size+1)*ratio;
    
    if(f.buf1)free((char *)f.buf1);
    f.buf1=(float *)calloc(2*size*sizeof(float),1);
    if(!f.buf1){
        fprintf(stderr,"1 cMalloc Errror %ld\n",(long)(2*size*sizeof(float)));
        return 1;
    }
    
    if(f.buf2)free((char *)f.buf2);
    f.buf2=(float *)calloc(2*size*sizeof(float),1);
    if(!f.buf2){
        fprintf(stderr,"2 cMalloc Errror %ld\n",(long)(2*size*sizeof(float)));
        return 1;
    }
    
    if(f.out2)free((char *)f.out2);
    f.out2=(float *)calloc(2*size*sizeof(float),1);
    if(!f.out2){
        fprintf(stderr,"2 cMalloc Errror %ld\n",(long)(2*size*sizeof(float)));
        return 1;
    }
    
    
    
    if(f.data)free((char *)f.data);
    f.data=(short *)calloc(2*size*sizeof(float),1);
    if(!f.data){
        fprintf(stderr,"2 cMalloc Errror %ld\n",(long)(2*size*sizeof(float)));
        return 1;
    }
    
    class Poly *pold[10];
    for(int n=0;n<10;++n){
        pold[n]=f.p[n];
    }
    
 
    f.p[0]=new Poly((float)sfinfo.samplerate);
    f.p[0]->Clowpass("butter",16,1.0,0.5*(f.filterFrequencies[0]+f.filterFrequencies[1]));
    
    for(int n=1;n<f.countFilter-1;++n){
    	f.p[n]=new Poly((float)sfinfo.samplerate);
        f.p[n]->Cbandpass("butter",16,1.0,0.5*(f.filterFrequencies[n-1]+f.filterFrequencies[n]),
                         0.5*(f.filterFrequencies[n]+f.filterFrequencies[n+1]));
    }

    f.p[f.countFilter-1]=new Poly((float)sfinfo.samplerate);
    f.p[f.countFilter-1]->Chighpass("butter",16,1.0,f.filterFrequencies[f.countFilter-1]);

    for(int n=0;n<f.countFilter;++n){
       f.p[n]->forceCascadeStart();
       if(pold[n])f.p[n]->gain=pold[n]->gain;
    }

    for(int n=0;n<10;++n){
        if(pold[n])delete pold[n];
        pold[n]=NULL;
    }

    return 0;
    
}

void MyFrame::doLoop(wxCommandEvent& event)
{	
	//	int id=event.GetId();
	
	//printf("53 doLoop %d %d IsChecked %d\n",event.GetSelection(),id,playMenu->IsChecked(PLAYBACK_MENU));
	
	bool check=playMenu->IsChecked(PLAYBACK_MENU);
	if(check){
		doLoopFiles=1;
	   // printf("check on\n");
	}else{
	   // printf("check off\n");
		doLoopFiles=0;
	}
	playMenu->Check(PLAYBACK_MENU,check);

	//Refresh();

}
void MyFrame::OnMenuRecentMenu(wxCommandEvent& event)
{
	
	int id=event.GetId();
	id=id-OPEN_RECENT;
	
	if(id <= 0){
	
		for(int n=0;n<fileCount;++n){
			subMenu->Delete(OPEN_RECENT+n+1);
		}
		
		fileCount=0;
		
		return;
	}
	
	id -= 1;
	
	//printf("OnMenuRecentMenu %d id %d %s\n",event.GetSelection(),id,fileList[id].ToUTF8().data());

	OpenFile(fileList[id]);
}

void MyFrame::OnMenuAudio(wxCommandEvent& event)
{
	
	int id=event.GetId();
	
	idLast=id;
	
//	printf("OnMenuAudio %d %d\n",event.GetSelection(),id);


	audiodevice=(id-AUDIO_MENU)/100;

	int speed=id-AUDIO_MENU-audiodevice*100;
	
	RtAudio::DeviceInfo info;
	
	info=dac->getDeviceInfo(audiodevice);
	
	samplerate=faudio=info.sampleRates[speed];

	//printf("OnMenuAudio audiodevice %d speed %d sampleRates %f\n",audiodevice,speed,faudio);
	
	if(infile){  
		setFilters();
	}
	   
	//if(tone)delete tone;
	//tone= new sTone(500.0,faudio);
	   
	try {
	// Stop the stream
		if (dac->isStreamOpen())dac->stopStream();
	}
	catch (RtAudioError& e) {
		e.printMessage();
	}
	if (dac->isStreamOpen())dac->closeStream();

	RtAudio::StreamParameters parameters;
	parameters.deviceId = audiodevice;
	parameters.nChannels = 1;
	parameters.firstChannel = 0;
	unsigned int bufferFrames = (unsigned int)(faudio/ncut);


	try {
		dac->openStream( &parameters, NULL, RTAUDIO_SINT16,
					(unsigned int)faudio, &bufferFrames, &sound1, (void *)this);
		dac->startStream();
	}
	catch ( RtAudioError& e ) {
		e.printMessage();
		exit( 0 );
	}
}
wxMenu *MyFrame::SetAudio()
{
	
	wxMenu *pMenu = new wxMenu;
	
	dac=new RtAudio;


    audiodevice=0;

	int deviceCount=dac->getDeviceCount();

	if (deviceCount < 1 ) {
		printf("\nNo audio devices found!\n");
		exit( 0 );
	}

		printf("\nAudio device Count %d default output device %d audiodevice %d\n",deviceCount,dac->getDefaultOutputDevice(),audiodevice);
	
		RtAudio::DeviceInfo info;
		for (int i=0; i<deviceCount; i++) {
		
			try {
				info=dac->getDeviceInfo(i);
				if(info.outputChannels > 0){
				wxMenu *subMenu = new wxMenu;
				pMenu->AppendSubMenu(subMenu, info.name, wxT("Description?"));
				// Print, for example, the maximum number of output channels for each device
					printf("audio device = %d : output  channels = %d Device Name = %s",i,info.outputChannels,info.name.c_str());
					if(info.sampleRates.size()){
						printf(" sampleRates = ");
						for (unsigned int ii = 0; ii < info.sampleRates.size(); ++ii){
						    char buff[256];
						    sprintf(buff," %d ",info.sampleRates[ii]);
						    printf(" %d ",info.sampleRates[ii]);
								subMenu->AppendRadioItem(AUDIO_MENU+i*100+ii, buff, wxT("Description?"));
				   		}
					}
					printf("\n");
				 }
			 
				if(info.inputChannels > 0){
				// Print, for example, the maximum number of output channels for each device
					printf("audio device = %d : input   channels = %d Device Name = %s",i,info.inputChannels,info.name.c_str());
					 if(info.sampleRates.size()){
						printf(" sampleRates = ");
						for (unsigned int ii = 0; ii < info.sampleRates.size(); ++ii){
							printf(" %d ",info.sampleRates[ii]);

					   }
					}
					printf("\n");
			   }

			}
			catch (RtAudioError &error) {
				error.printMessage();
				break;
			}
		
		}
		
		RtAudio::StreamParameters parameters;
		audiodevice = dac->getDefaultOutputDevice();
		wxCommandEvent event(wxEVT_MENU,AUDIO_MENU+100*audiodevice);
		event.SetInt(1);
		OnMenuAudio(event);
	
		printf("\n");
	
	return pMenu;
}

void MyFrame::resized(wxSizeEvent& evt)
{
    OnSize(evt);
    SetGauges();
   	 Refresh();
}

int MyFrame::SetGauges()
{
    for(int n=0;n<channelCount;++n){
		wxPoint pp;
		int x,y,xs,ys;
		slider[n]->GetPosition (&x, &y);
		slider[n]->GetSize (&xs, &ys);
		//printf("slider6 %d %d\n",x,y);
		pp=g[n]->GetPosition();
#ifdef MENU_SHIFT
		pp.x=x-MENU_SHIFT;
//		printf("MENU_SHIFT %d\n",MENU_SHIFT);
#else
//		printf("MENU_SHIFT no\n");
		pp.x=x-5;
#endif
		g[n]->SetPosition(pp);
   }
	return 0;
}
void MyFrame::OpenFile( wxString filename)
{
	const char *file=filename.ToUTF8().data();
		
	int fileFound=-1;
	for(int n=0;n<fileCount;++n){
		if(filename == fileList[n])fileFound=n;
	}
	if(fileFound < 0){
		for(int n=8;n >= 0;--n){
			fileList[n+1]=fileList[n];
		}
		if(++fileCount >= 10)fileCount=10;
		fileList[0]= filename;
		
		for(int n=0;n<fileCount-1;++n){
			subMenu->Delete(OPEN_RECENT+n+1);  // update recent list
		}
		
		for(int n=0;n<fileCount;++n){
			subMenu->AppendRadioItem(OPEN_RECENT+n+1,fileList[n], wxT("Description?"));
		}
	}
	// printf("OpenFile %s fileCount %d\n",file,fileCount);

	wait=1;
	
	
	sourceFile *s = dynamic_cast<sourceFile*>(source);
	if(s){
		s->infile=NULL;
		//printf("1 dynamic_cast\n");
	}
	
	if(infile)sf_close(infile) ;
	infile=NULL;
	
    if ((infile = sf_open (file, SFM_READ, &sfinfo))  == NULL)
    {
        printf ("Not able to open input file %s\n", file) ;
        infile=NULL;
        return;
    }
    
	if(s){
		s->infile=infile;
		//printf("2 dynamic_cast\n");
	}
    
    SetTitle(filename);

    nchannels=sfinfo.channels;
    
    //printf("sfinfo.frames %lld\n",(long long)sfinfo.frames);
    
    if(sfinfo.samplerate > 0){
    	sliderTime->SetMax(sfinfo.frames/sfinfo.samplerate); 
    }else{
     	sliderTime->SetMax(50); 
   }
    sliderTime->SetValue(0);
    CurrentFrame=0;


	wxCommandEvent event(wxEVT_MENU,idLast);
	event.SetInt(1);
	OnMenuAudio(event);
	wait=0;


}
void MyFrame::DeleteRow( wxCommandEvent &event)
{
    int id =event.GetId(); 
//    printf("DeleteRow %d %d %d\n",event.GetInt(),event.GetSelection(),id);
    
    if(id == ID_PLAY){
 //   	printf("ID_PLAY\n");
    	wait=0;
    }else if(id == ID_PAUSE){
   // 	printf("ID_PAUSE\n");
    	wait=1;
    }
	//SetGauges();
 }
 
 static int setgain(float value,class Poly *p)
{
	if(!p)return 1;
	
    if(value > -0.9 && value < 0.9){
        p->gain=1.0;
    }else if(value > 1){
        p->gain=value;
    }else if(value < -1.0){
        p->gain=1.0/fabs(value);
    }
    return 0;
}


void MyFrame::OnScroll(wxCommandEvent &event)
{
	int which=event.GetId()-SCROLL_01;
	float value=event.GetSelection();
	if(event.GetId() == SCROLL_GAIN){
		//printf("OnScroll value %g %d \n",value,event.GetId());
		gain=value;
		return;
	}else if(event.GetId() == SCROLL_TIME){
		// printf("OnScroll value %g %d \n",value,event.GetId());
		sf_count_t ret=sf_seek(infile, value*sfinfo.samplerate, SEEK_SET);
        if(ret < 0)printf("sf_seek error\n");
		CurrentFrame=value*sfinfo.samplerate;
		return;
	}
	
	setgain(value,f.p[which]);
}

MyFrame::MyFrame(wxFrame *frame, const wxString& title, const wxPoint& pos,
    const wxSize& size, long style)
    : wxFrame(frame, wxID_ANY, title, pos, size, style)
{

#ifdef __WXMAC__
    // we need this in order to allow the about menu relocation, since ABOUT is
    // not the default id of the about menu
    wxApp::s_macAboutMenuItemId = wxID_ABOUT;
#endif

	fileCount=0;
	faudio=48000;
	infile=NULL;
	idLast=AUDIO_MENU;
 	ncut=10;
 	nchannels=1;
 	wait=0;
 	gain=1.0;
 	CurrentFrame=0ll;
 	tone=NULL;
 	
 	sf=new sourceFile;
 
 	st=new sourceTone;
 	
 	st->tone= new sTone(16000.0,48000);
 
 	source=sf;
 	
 	of=new outputFile;
 	
 	of->f=this;
 
 	ot=new outputTone;
 	 
 	output=of;
 	
	memset((char *)&f,0,sizeof(f));

    wxMenu *fileMenu = new wxMenu;
    fileMenu->Append(wxID_OPEN, "&Open...");
    
	subMenu = new wxMenu;
	fileMenu->AppendSubMenu(subMenu,"Open Recent", wxT("Description?"));
	
	subMenu->AppendRadioItem(OPEN_RECENT,"History Clear", wxT("Description?"));
	
	subMenu ->AppendSeparator();
	
	wxFileConfig cfg(wxT("Equalizer"), wxT("dir"));

	for(int n=0;n<10;++n){
		wxString ns = wxString::Format(wxT("%d"),n);
		if ( cfg.Read("File"+ns, &fileList[n]) )
		{
			//printf("Read %s\n",fileList[n].ToUTF8().data());
			subMenu->AppendRadioItem(OPEN_RECENT+n+1,fileList[n], wxT("Description?"));
			fileCount++;
		}
	}
	
 
    
    
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tALT-X");
    
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(wxID_HELP, "&About");

    playMenu = new wxMenu;
	subMenu2 = new wxMenu;
	playMenu->AppendSubMenu(subMenu2,"Options", wxT("Description?"));
    subMenu2->AppendCheckItem(PLAYBACK_MENU, "Loop Files", wxT("Description?"));

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");
    menuBar->Append(SetAudio(),"&Audio");
    menuBar->Append(playMenu, "&Playback");
    
    SetMenuBar(menuBar);
    
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	channelCount=10;
   
	panel= new wxPanel(this,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_SUNKEN);
   
	wxStaticText *t[10];

	char *s[]={(char *)"32 HZ",(char *)"64 HZ",(char *)"125 HZ",(char *)"300 HZ",(char *)"500 HZ",
			   (char *)"1 KHZ",(char *)"2 KHZ",(char *)"4 KHZ",(char *)"8 KHZ",(char *)"16 KHZ"};
			   
	float lfilterFrequencies[10]={32,64,125,300,500,1000,2000,4000,8000,16000};
	for(int n=0;n<channelCount;++n){
	    f.filterFrequencies[n]=lfilterFrequencies[n];
	}
	f.countFilter=channelCount;
		   
	for(int n=0;n<channelCount;++n){
		t[n]= new wxStaticText(panel,wxID_STATIC,s[n],wxDefaultPosition,wxSize(-1,20),wxALIGN_CENTER);
	}
   
	for(int n=0;n<channelCount;++n){
		slider[n]=new wxSlider(panel,SCROLL_01+n,0,-12,12,wxPoint(-1,30),wxSize(-1,150),wxSL_VERTICAL | wxSL_AUTOTICKS | wxSL_LABELS);
	}

	int x=20;
	for(int n=0;n<channelCount;++n){
		g[n]=new wxGauge(panel,SCROLL_01,100,wxPoint(x,2),wxSize(-1,150),wxGA_VERTICAL);
		x += 65;
		g[n]->SetValue(50);
	}


    wxFlexGridSizer* const sizer8 = new wxFlexGridSizer(3,channelCount,0,0);
    
    for(int n=0;n<channelCount;++n){
    	sizer8->Add(t[n], 1, wxEXPAND);
    }
    for(int n=0;n<channelCount;++n){
    	sizer8->Add(slider[n], wxSizerFlags(2).Expand().Border());
	}
	
    for(int n=0;n<channelCount;++n){
    	sizer8->Add(g[n], 1, wxEXPAND);
    }
	
    panel->SetSizer(sizer8);
    
   	sizer->Add(panel, 0, wxALL,2);

    wxBoxSizer* const sizer9 = new wxBoxSizer(wxVERTICAL);
    
    sizer9->Add(sizer, 1, wxEXPAND);
        
    wxPanel *panel3= new wxPanel(this,wxID_ANY,wxDefaultPosition,wxSize(650,200),wxBORDER_SUNKEN,wxT("Equalizer"));
    
    wxFlexGridSizer* const sizer10 = new wxFlexGridSizer(2,4,0,0);

    wxStaticText *tt2=new wxStaticText(panel3,wxID_STATIC,wxT("Time(sec)"),wxDefaultPosition,wxDefaultSize,wxALIGN_CENTER);
    sizer10->Add(tt2, 1, wxEXPAND);
    sizer10->AddSpacer(20);
    
    wxStaticText *tt=new wxStaticText(panel3,wxID_STATIC,wxT("Gain"),wxDefaultPosition,wxDefaultSize,wxALIGN_CENTER);
    sizer10->Add(tt, 1, wxEXPAND);
    
    button=new wxButton(panel3,ID_PLAY,wxT("Play"), wxPoint(10,70),wxSize(150,50));
  	 sizer10->Add(button, 1, wxEXPAND);


    sliderTime=new wxSlider(panel3,SCROLL_TIME,5,0,50,wxPoint(20,40),wxSize(240,-1),wxSL_HORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS);
    sliderTime->SetValue(0);
    //sliderTime->SetBackgroundColour(wxColour(0, 0, 1));
    sizer10->Add(sliderTime, 1, wxEXPAND);
    sizer10->AddSpacer(20);

    sliderGain=new wxSlider(panel3,SCROLL_GAIN,5,0,50,wxPoint(20,40),wxSize(240,-1),wxSL_HORIZONTAL | wxSL_AUTOTICKS | wxSL_LABELS);
    sliderGain->SetValue(1);
    sizer10->Add(sliderGain, 1, wxEXPAND);

  	button2=new wxButton(panel3,ID_PAUSE,wxT("Pause"), wxPoint(10,100),wxSize(150,50));
  	sizer10->Add(button2, 1, wxEXPAND);
  	
  	panel3->SetSizer(sizer10);
  	
    sizer9->Add(panel3, 1, wxALL,2);

    SetSizer(sizer9);
    SetAutoLayout(true);
    

    Show(true);
    
}
void MyFrame::OnMenuFileOpen( wxCommandEvent& WXUNUSED(event) )
{
    wxString filename = wxFileSelector("Choose Audio File", "", "", "",
        "Audio File All files (*.*)|*.*",
        wxFD_OPEN);
    if (!filename.IsEmpty())
    {
    	OpenFile(filename);
    }
}


// File|Exit command
void MyFrame::OnMenuFileExit( wxCommandEvent& WXUNUSED(event) )
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnMenuHelpAbout( wxCommandEvent& WXUNUSED(event) )
{
    wxMessageBox("Equalizer(c) 2022 Dale Ranta");
}



