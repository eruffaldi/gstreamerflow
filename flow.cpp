#include <iostream>
#include <thread>

#include <gst/gst.h> 
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include <memory>   
#include <vector>
//#include "pooledchannel.hpp"

// three modes of operation:
// - sink
// - source
// - sink/source
class GstreamerStep
{
public:

    GstreamerStep(std::string pipeline, bool started = true, int buffers = 10);
    ~GstreamerStep(void);

    void operator >>(std::vector<uint8_t> & out);
    void operator <<(const std::vector<uint8_t> & in);

    bool isValid() const { return pipeline != 0; }

    bool start();
    bool stop();
private:

    GstAppSink * appsink;
    GstAppSrc * appsrc;
    GstElement * pipeline = 0;
};



GstreamerStep::GstreamerStep(std::string xpipeline, bool started, int buffers)
{
	GError * error_ = 0;
	gst_init(NULL, NULL);
	char buffer[128];
    pipeline = gst_parse_launch(xpipeline.c_str(),&error_);
    if(!pipeline)
    {
    	std::cerr << "invalid gstreamer pipeline:" << xpipeline << "\n";
    	return;
    }
    GstElement * sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    if(sink && GST_IS_APP_SINK(sink))
    {
	    appsink = (GstAppSink*)(sink);
	    gst_app_sink_set_emit_signals(appsink, true);
	    gst_app_sink_set_drop(appsink, buffers < 0);
	    gst_app_sink_set_max_buffers(appsink, buffers < 0 ? -buffers : buffers);    
	}
	else if(sink)
	{
		std::cout << "not a valid sink element\n";
	}

    GstElement * source = gst_bin_get_by_name(GST_BIN(pipeline), "source");
    if(source && GST_IS_APP_SRC(source))
    {
	    appsrc = (GstAppSrc*)(source);
	    gst_app_src_set_emit_signals(appsrc, true);
	    gst_app_src_set_stream_type(appsrc, GST_APP_STREAM_TYPE_STREAM);
	}
	else if(source)
	{
		std::cout << "not a valid source element\n";
	}

    g_signal_connect(pipeline, "deep-notify", G_CALLBACK(gst_object_default_deep_notify ), NULL);             
    if(started)
	    gst_element_set_state(pipeline, GST_STATE_PLAYING);

	// bus = gst_pipeline_get_bus (GST_PIPELINE (app->pipeline));

}

GstreamerStep::~GstreamerStep()
{
	gst_element_send_event(pipeline, gst_event_new_eos());
	/*
	GstEvent* flush_start = gst_event_new_flush_start(); 
	gboolean ret = FALSE; 
	ret = gst_element_send_event(GST_ELEMENT(pipeline), flush_start); 
	GstEvent* flush_stop = gst_event_new_flush_stop(TRUE); 
	ret = gst_element_send_event(GST_ELEMENT(pipeline), flush_stop); 
	*/

	gst_element_set_state (pipeline, GST_STATE_NULL);
	std::cout << "closing\n";
	gst_object_unref (pipeline);
}
 

 
bool GstreamerStep::start()
{
	int r = 0;
	if((r = gst_element_set_state (this->pipeline, GST_STATE_PLAYING)) != GST_STATE_CHANGE_FAILURE)
		return true;
	else
	{
		std::cout << "Failed to set pipeline state to PLAYING " << r << std::endl;
		return false;
	}
}
 
bool GstreamerStep::stop()
{
	gst_element_set_state (this->pipeline, GST_STATE_NULL);
	return true;
}
 
// TODO automatic resize of out
void GstreamerStep::operator >>(std::vector<uint8_t> & out)
{
	if(!appsink)
		return;
	GstSample * sample = gst_app_sink_pull_sample(appsink);
    GstBuffer * gstImageBuffer= gst_sample_get_buffer(sample); 
    GstCaps * c = gst_sample_get_caps(sample); 
    auto q = gst_caps_to_string(c);
    std::cout << "received caps: " << q << std::endl;
    g_free(q);
	int n = gst_buffer_get_size(gstImageBuffer);
    out.resize(n);
	gst_buffer_extract(gstImageBuffer,0,&out[0],out.size());
	gst_buffer_unref(gstImageBuffer);

}

void GstreamerStep::operator <<(const std::vector<uint8_t> & out)
{
	static int q = 0;
	q++;
	if(!appsrc)
		return;
	if(out.empty())
	{
		// NOTE: appsink receives EOS using GstAppSinkCallbacks
		gst_app_src_end_of_stream (appsrc);
	}
	else
	{
		//std::cout << "pushing " << out.size() << std::endl;
	   	GstCaps * c = gst_caps_from_string("video/x-raw, format=(string)RGB, width=(int)640, height=(int)480, framerate=(fraction)30/1, pixel-aspect-ratio=(fraction)1/1, interlace-mode=(string)progressive"); 
		gpointer p = g_malloc(out.size());
		memcpy(p,&out[0],out.size());
		GstBuffer * buffer = gst_buffer_new_wrapped(p,out.size());
		buffer->duration = 25000000 + 1000000*(q % 5); // 
		gst_app_src_set_caps(appsrc,c);
		gst_app_src_push_buffer(appsrc,buffer);
		/*
		// sample not needed
		GstSample *	sample = gst_sample_new (buffer,c,NULL,NULL);
		gst_app_src_push_sample(appsrc,sample);
		*/
	}
}


/*
	if(xpipeline == 0)
	{int width, int height, int device_id, bool h264 , const char * xpipeline): height_(height), width_(width), device_id_(device_id), pipeline(0),bus(0),appsink(0)

		if(h264)
			sprintf(buffer,"v4l2src device=/dev/video%d ! queue ! video/x-h264, width=(int)%d, height=(int)%d, fframerate=30/1 ! h264parse ! avdec_h264 ! video/x-raw,format=RGB, width=%d,height=%d ! appsink name=sink",device_id,width,height,width,height);
		else
			sprintf(buffer,"v4l2src device=/dev/video%d ! queue ! video/x-raw,format=RGB, width=%d,height=%d ! appsink name=sink",device_id,width,height);
		xpipeline =buffer;
	}
*/

void testminimal()
{
	GstreamerStep a("appsrc name=source ! appsink name=sink");
	std::vector<uint8_t> data(10);
	std::vector<uint8_t> data2(10);
	for(int i = 0; i < 10; i++)
	{
		data[0] = i;
		a << data;
		a >> data2;
		std::cout << "received " << data2.size() << " " << data2[0] << std::endl;
	}	
}

/*
void playersink(GstreamerStep & s)
{
	while(true)
	{
		 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
*/

int main(int argc, char * argv[])
{
	// creo 2 pipeline ferme:   
	//   camera h264 -> appsink
	// 	 appsrc -> decompress -> appsink
	// 
	// le inserisco in un loop e ci metto una delle pipe protette
	//
	// gstreamer1 -> pooledchannel -> gstreamer2
		//testminimal();
	GstreamerStep src("videotestsrc ! video/x-raw,format=(string)RGB,width=640,height=480 ! appsink name=sink");
	GstreamerStep dst("appsrc name=source ! queue ! appsink name=sink");
	GstreamerStep ter("appsrc name=source ! videoconvert ! vtenc_h264 ! mp4mux ! filesink location=x.mp4");
	std::vector<uint8_t> buffer;
	std::vector<uint8_t> buffer2;
	//std::thread tx(playersink,std::ref(dst));
	for(int i = 0; i < 100; i++)
	{
		src >> buffer;
		std::cout << buffer.size() << std::endl;
		dst << buffer;
		dst >> buffer2;
		std::cout << "!" << buffer2.size() << std::endl;
		ter << buffer2;
	}
	// EOS
	buffer.clear();
	dst << buffer; 
	ter << buffer;
	return 0;
}
