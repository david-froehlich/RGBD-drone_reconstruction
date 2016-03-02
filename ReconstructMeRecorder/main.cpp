
#include <stdio.h>
#include <string>
#include <iostream>
#include <reconstructmesdk/reme.h>
#include <conio.h>

using namespace std;
int main(int argc, char* args[])
{
	// Create a new context
	reme_context_t c;
	reme_context_create(&c);
	// Options object
	reme_options_t o;
	reme_options_create(c, &o);
	// Create a new real sensor
	reme_sensor_t s;
	reme_sensor_create(c, "openni;mskinect", true, &s);
	reme_sensor_open(c, s);
	// Create a new file recorder.
	// By default the recorder uses the first sensor.
	reme_recorder_t r;
	reme_recorder_create(c, &r);
	// Modify the recording outputs
	reme_recorder_bind_file_options(c, r, o);
	reme_options_set(c, o, "file_sensor_config", "my_file_recording.txt");
	reme_options_set(c, o, "depth_file", "my_depths.gz");
	reme_options_set(c, o, "color_file", "my_colors.avi");

	// Open the recorder. At this point the sensor
	// needs to be open as well.
	reme_recorder_open(c, r);
	// Quick viewer
	reme_viewer_t viewer;
	reme_viewer_create_image(c, "Recording data", &viewer);
	reme_image_t depth;
	reme_image_create(c, &depth);
	reme_viewer_add_image(c, viewer, depth);
	// Loop until done
	int frames = 500;
	while (!_kbhit() && REME_SUCCESS(reme_sensor_grab(c, s)) && frames--) {
		// Prepare image and depth data
		reme_sensor_prepare_image(c, s, REME_IMAGE_AUX);
		reme_sensor_prepare_image(c, s, REME_IMAGE_DEPTH);
		// Update the recorder with current sensor data
		reme_recorder_update(c, r);
		// Update the viewer
		reme_sensor_get_image(c, s, REME_IMAGE_DEPTH, depth);
		reme_viewer_update(c, viewer);
	}
	// Done with recorder, forces the files to be closed.
	reme_recorder_close(c, r);
	return 0;
}