#pragma once

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



inline GstreamerStep::GstreamerStep(std::string xpipeline, bool started, int buffers)
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

inline  GstreamerStep::~GstreamerStep()
{
	if(appsrc)
	{
		gst_app_src_end_of_stream(appsrc); 
	}
	gst_element_send_event(pipeline, gst_event_new_eos());

	//wait for EOS to trickle down the pipeline. This will let all elements finish properly
    if(appsrc)
    {
	    GstBus* bus = gst_element_get_bus(pipeline);
	    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
	    if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR)
	    {
	        if(msg != NULL)
	        {
	            gst_message_unref(msg);
	        }
	    }
	    g_object_unref(G_OBJECT(bus));
	}

    auto status = gst_element_set_state (pipeline, GST_STATE_NULL);
    if (status == GST_STATE_CHANGE_ASYNC)
    {
        // wait for status update
        GstState st1;
        GstState st2;
        status = gst_element_get_state(pipeline, &st1, &st2, GST_CLOCK_TIME_NONE);
    }
    if (status == GST_STATE_CHANGE_FAILURE)
    {
        // handleMessage (pipeline);
    }
    gst_object_unref (GST_OBJECT (pipeline));

}
 

 
inline  bool GstreamerStep::start()
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
 
inline bool GstreamerStep::stop()
{
	gst_element_set_state (this->pipeline, GST_STATE_NULL);
	return true;
}
 
// TODO automatic resize of out
inline void GstreamerStep::operator >>(std::vector<uint8_t> & out)
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

inline  void GstreamerStep::operator <<(const std::vector<uint8_t> & out)
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
