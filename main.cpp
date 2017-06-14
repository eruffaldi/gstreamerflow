#include "flow.h"

/*
void playersink(GstreamerStep & s)
{
	while(true)
	{
		 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
*/



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
