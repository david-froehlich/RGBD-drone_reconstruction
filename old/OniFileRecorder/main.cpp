

//
// try to write an .oni file
//

#include <stdio.h>
#include <OpenNI.h>
#include <string>
#include <iostream>
#include <ctime>

#define VIDEO_RESOLUTION_WIDTH 320
#define VIDEO_RESOLUTION_HEIGHT 240

#define VIDEO_FPS 30

//the duration in seconds
#define VIDEO_DEFAULT_DURATION 15

#define MAX_TIMESTAMP_DIFFERENCE_MS 100

using namespace openni;
using namespace std;

int video_duration = VIDEO_DEFAULT_DURATION;

int getParams(int arvc, char **argv) {
  int index;
  int c;
  while ((c = getopt (argc, argv, "d:")) != -1)
    switch (c)
      {
      case 'd':
        video_duration = atoi(optarg);
        if (video_duration == 0) {
            fprintf(stderr, "Please pass a valid time (in seconds) for the video duration");
            return 1;
        }
        break;
      case '?':
        if (video_duration == 0) {
          fprintf(stderr, "Please pass a valid time (in seconds) for the video duration");
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }
  return 0;
}

string getDateTime() {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer[80];

  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer,80,"%Y-%m-%d_%H-%M-%S",timeinfo);
  std::string str(buffer);

  return str;
}

string video_outfile_name = "recording_" + getDateTime() + ".oni";

int main(int argc, char **argv) {
    openni::OpenNI::initialize();
	int max_clocks_per_frame = MAX_TIMESTAMP_DIFFERENCE_MS * CLOCKS_PER_SEC / 1000;

    Array<DeviceInfo> device_infos;
    OpenNI::enumerateDevices(&device_infos);
    openni::Device device;
    openni::Status status_device = device.open(openni::ANY_DEVICE);
	//device.setDepthColorSyncEnabled(TRUE);
    if (status_device != openni::STATUS_OK) {
        cout << "Unable to open device" << endl;
        getchar();
        exit(1);
    }

    ImageRegistrationMode mode = IMAGE_REGISTRATION_DEPTH_TO_COLOR;
	//device.setImageRegistrationMode(mode);
    openni::VideoStream depth_stream, rgb_stream;
    Status status_depth_stream_create = depth_stream.create(device, openni::SENSOR_DEPTH);
    Status status_rgb_stream_create = rgb_stream.create(device, openni::SENSOR_COLOR);

	const SensorInfo *depth_info = device.getSensorInfo(openni::SENSOR_DEPTH);
	const SensorInfo *rgb_info = device.getSensorInfo(openni::SENSOR_COLOR);

	VideoMode rgb_video_mode = rgb_stream.getVideoMode();
	VideoMode depth_video_mode = depth_stream.getVideoMode();
	
    if (status_depth_stream_create != openni::STATUS_OK
            || status_rgb_stream_create != openni::STATUS_OK) {
        cout << "Unable to open streams" << endl;
        getchar();
        exit(1);
    }
	
	rgb_video_mode.setResolution(VIDEO_RESOLUTION_WIDTH, VIDEO_RESOLUTION_HEIGHT);
	rgb_video_mode.setFps(VIDEO_FPS);
	rgb_video_mode.setPixelFormat(openni::PIXEL_FORMAT_RGB888);
	Status video_mode_rgb_status = rgb_stream.setVideoMode(rgb_video_mode);
	rgb_video_mode = rgb_stream.getVideoMode();

	depth_video_mode.setResolution(VIDEO_RESOLUTION_WIDTH, VIDEO_RESOLUTION_HEIGHT);
	depth_video_mode.setFps(VIDEO_FPS);
	depth_video_mode.setPixelFormat(PIXEL_FORMAT_DEPTH_1_MM);
	Status video_mode_depth_status = depth_stream.setVideoMode(depth_video_mode);
	depth_video_mode = depth_stream.getVideoMode();

	if (video_mode_depth_status != STATUS_OK
		|| video_mode_rgb_status != STATUS_OK) {
		cout << "Failed to set video mode. rgb-status: " << video_mode_rgb_status << ", depth-status: " << video_mode_depth_status << endl;
		getchar();
		exit(-1);
	}
	else {
		cout << "stream-info rgb: " << rgb_video_mode.getResolutionX()
			<< "x" << rgb_video_mode.getResolutionY() << " at " << rgb_video_mode.getFps() << " FPS" << endl;

		cout << "stream-info depth: " << depth_video_mode.getResolutionX()
			<< "x" << depth_video_mode.getResolutionY() << " at " << depth_video_mode.getFps() << " FPS" << endl;

		cout << "Press enter to start recording...";
		getchar();
	}

    openni::VideoStream** streams = new openni::VideoStream*[2];
    streams[0] = &depth_stream;
    streams[1] = &rgb_stream;

    Recorder rec;
    rec.create(video_outfile_name.c_str());
    rec.attach(rgb_stream);
    rec.attach(depth_stream);

    rec.start(); 
    rgb_stream.start();
    depth_stream.start();
    if (!device.isValid()) {
        cout << "Invalid device..." << endl;
        getchar();
        exit(1);
    }

	//* 2 because each depth or rgb frame is counted individually
    int required_frames = video_duration * VIDEO_FPS * 2;	
    clock_t begin_time = clock();

	int captured_frames[] = { 0, 0 };
    int captured_frames_total = 0;

	//toggles between 1 and 0
	int stream_index = 0;
	uint64_t last_timestamp[] = { 0, 0 };
    uint64_t systime_between_frames = 0;
    while (captured_frames_total++ < required_frames) {
		stream_index = (stream_index + 1) % 2;
        
		VideoFrameRef frame;
		streams[stream_index]->readFrame(&frame);
  //      const clock_t end_time = clock();
		//
  //      systime_between_frames = (end_time - begin_time);
		////cout << frame.getTimestamp() << endl;
		//if (systime_between_frames > max_clocks_per_frame) {
		//	uint64_t timestamp_difference = frame.getTimestamp() - last_timestamp[stream_index];
		//	cout << "stream: " << stream_index << ", frame: " << captured_frames[stream_index] 
		//		<< ", systime between frames: " << systime_between_frames << ", timestamp difference: " << timestamp_difference << endl;
		//}
		/*if ((captured_frames[stream_index] % 50) == 0) {
			cout << "captured frame number " << (captured_frames[stream_index]) << " from stream "
				<< stream_index << " took " << (systime_between_frames / 1000) << "ms" << endl;

		}*/
		//last_timestamp[stream_index] = frame.getTimestamp();
		captured_frames[stream_index]++;
        
        //begin_time = end_time;
    }
    rec.stop();
    rec.destroy();

    depth_stream.stop();
    depth_stream.destroy();
    rgb_stream.stop();
    rgb_stream.destroy();
    device.close();
    OpenNI::shutdown();
	cout << "---------------------------------------DONE-------------------------------------------" << endl;
	cout << "---------------------------------------DONE-------------------------------------------" << endl;
	cout << "---------------------------------------DONE-------------------------------------------" << endl;
	cout << "---------------------------------------DONE-------------------------------------------" << endl;
	cout << "---------------------------------------DONE-------------------------------------------" << endl;
	cout << "---------------------------------------DONE-------------------------------------------" << endl;
	cout << "---------------------------------------DONE-------------------------------------------" << endl;

}
