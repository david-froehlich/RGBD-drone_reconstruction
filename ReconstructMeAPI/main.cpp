
#include <stdio.h>
#include <OpenNI.h>
#include <string>
#include <algorithm>
#include <map>
#include <iterator>
#include <iostream>

#define DEFAULT_ONI_FILE_PATH "C:\\\\Users\\David\\Desktop\\recording.oni"
//time in microseconds
#define TIME_BETWEEN_FRAMES_THRESHHOLD_MS 80000


using namespace std;
using namespace openni;

struct StreamData {
	VideoStream *stream;
	string stream_name;
	SensorType sensor_type;
	int read_frames = 0;
	uint64_t last_frame_timestamp = 0;

	StreamData(VideoStream *stream, string stream_name, SensorType sensor_type) {
		this->stream = stream;
		this->stream_name = stream_name;
		this->sensor_type = sensor_type;
	}
};

int main(int argc, char* args[]) {
	const char* oni_file_path = DEFAULT_ONI_FILE_PATH;
	
	if (argc > 1) {
		oni_file_path = args[1];
	}
	
	cout << "Reading file " << oni_file_path << endl;

	Device ni_dev;
	VideoStream ni_color, ni_depth;
	
	StreamData* stream_data[] = {
		new StreamData(&ni_depth, "depth", openni::SENSOR_DEPTH),
		new StreamData(&ni_color, "color", openni::SENSOR_COLOR)
	};
	
	VideoFrameRef ni_frame;

	Status stat = openni::OpenNI::initialize();
	Status open_status = ni_dev.open(oni_file_path);
	if (open_status != STATUS_OK) {
		cout << "couldn't open oni-file..." << endl;
		getchar();
	}

	StreamData *current_data;
	int size = sizeof stream_data / sizeof(stream_data[0]);
	for (int i = 0; i < size; i++) {
		current_data = stream_data[i];
		current_data->stream->create(ni_dev, current_data->sensor_type);
		Status status = current_data->stream->start();
		if (status != STATUS_OK) {
			cout << "opening stream '" << current_data->stream_name << "' failed...";
			getchar();
		}
		VideoMode current_mode = current_data->stream->getVideoMode();
		cout << "Stream info: " << current_data->stream_name << endl;
		cout << "FPS: " << current_mode.getFps() << endl;
		cout << "Resolution: X: " << current_mode.getResolutionX() << "; Y: " << current_mode.getResolutionY() << endl;
	}
	cout << "-------------------------------------------------------------------" << endl;
	
	PlaybackControl *pb_control = ni_dev.getPlaybackControl();
	pb_control->setRepeatEnabled(false);
	
	//not really used...
	VideoFrameRef temp_frame;

	VideoStream* streams[2] = { stream_data[0]->stream, stream_data[1]->stream };
	cout << "Seeking to end of file..." << endl;
	bool viewer_done = false;

	int frame_sizes[2] = { -1, -1 };
	
	uint64_t first_timestamp = 0, last_timestamp;
	
	std::map<int, uint64_t> high_times_between_frames;
	uint64_t time_between_frames, max_time_between_frames = 0;
	while (!viewer_done)
	{
		// Grab from the real sensor
		int index;
		Status status = OpenNI::waitForAnyStream(streams, 2, &index, 5000);
		if (status == openni::STATUS_OK) {
			stream_data[index]->stream->readFrame(&temp_frame);
			stream_data[index]->read_frames++;
			int frame_size = temp_frame.getDataSize();
			if (frame_size != frame_sizes[index]) {
				if (frame_sizes[index] != -1) {
					cout << "frame-size of stream " << stream_data[index]->stream_name << " was " << frame_sizes[index] << ", now is " << frame_size << endl;
				}
				else {
					frame_sizes[index] = frame_size;
				}
			}
			if (first_timestamp == 0) {
				first_timestamp = temp_frame.getTimestamp();
			}
			time_between_frames = temp_frame.getTimestamp() - stream_data[index]->last_frame_timestamp;
			if (time_between_frames > TIME_BETWEEN_FRAMES_THRESHHOLD_MS && stream_data[index]->read_frames > 0) {
				high_times_between_frames[stream_data[index]->read_frames] = time_between_frames;
			}
			max_time_between_frames = max_time_between_frames > time_between_frames ? max_time_between_frames : time_between_frames;

			stream_data[index]->last_frame_timestamp = temp_frame.getTimestamp();
		}
		else {
			viewer_done = TRUE;
		}
		last_timestamp = temp_frame.getTimestamp();
	}

	cout << "The following frames have a time-difference greater than the threshhold of " << TIME_BETWEEN_FRAMES_THRESHHOLD_MS << " microseconds:" << endl;
	for (auto it = high_times_between_frames.cbegin(); it != high_times_between_frames.cend(); ++it)
	{
		std::cout << "i: " << it->first << ": time: " << it->second << " ms" << endl;
	}
	cout << stream_data[0]->stream_name << ": " << stream_data[0]->read_frames << " frames, with " << frame_sizes[0] << " bytes each" << endl;
	cout << stream_data[1]->stream_name << ": " << stream_data[1]->read_frames << " frames, with " << frame_sizes[1] << " bytes each" << endl;
	cout << "Maximum time between two frames in microseconds:" << max_time_between_frames << endl;
	cout << "Time between first- and last frame: " << (last_timestamp - first_timestamp) << " ms" << endl;
	getchar();
}
