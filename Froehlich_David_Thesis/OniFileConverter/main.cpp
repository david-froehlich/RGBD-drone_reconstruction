#include <stdio.h>
#include <string>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <OpenNI.h>

using namespace std;


#define MAX_FRAMES 100
#define DEFAULT_ONI_FILE_PATH "C:\\Users\\David\\Desktop\\recording.oni"

using namespace std;
using namespace openni;
int main(int argc, char* args[]) {
	const char* oni_file_path = DEFAULT_ONI_FILE_PATH;

	if (argc > 1) {
		oni_file_path = args[1];
	}

	cout << "Reading file " << oni_file_path << endl;

	
	Device ni_dev;
	VideoStream ni_depth;
	VideoStream ni_color;

	VideoFrameRef ni_frame;

	Status stat = OpenNI::initialize();
	Status open_status = ni_dev.open(oni_file_path);

	if (open_status != STATUS_OK) {
		cout << "Unable to open oni-file" << endl;
		getchar();
		exit(1);
	}


	PlaybackControl *pb_control = ni_dev.getPlaybackControl();
	pb_control->setRepeatEnabled(false);

	Status create_status_depth = ni_depth.create(ni_dev, SENSOR_DEPTH);
	Status start_status_depth = ni_depth.start();

	Status create_status_color = ni_color.create(ni_dev, SENSOR_COLOR);
	Status start_status_color = ni_color.start();


	int image_width_depth = ni_depth.getVideoMode().getResolutionX();
	int image_height_depth = ni_depth.getVideoMode().getResolutionY();

	int image_width_color = ni_color.getVideoMode().getResolutionX();
	int image_height_color = ni_color.getVideoMode().getResolutionY();


	VideoStream* streams[] = { &ni_depth };//, &ni_color
	bool viewer_done = false;
	int read_frames = 0;
	int index;
	VideoFrameRef depthFrame, colorFrame;
	cv::Mat colorcv(cv::Size(image_width_color, image_height_color), CV_8UC3, NULL);
	cv::Mat depthcv(cv::Size(image_width_depth, image_height_depth), CV_16UC1, NULL);
	//cv::namedWindow("RGB", CV_WINDOW_AUTOSIZE);
	//cv::namedWindow("Depth", CV_WINDOW_AUTOSIZE);

	VideoMode mode;
	PixelFormat format;
	cv::VideoWriter writer;
	//
	writer.open("depthstream.avi", CV_FOURCC('X', 'V', 'I', 'D'), 29, cv::Size(480, 640));
	bool stream_opened = writer.isOpened();
	
	
	while (read_frames < MAX_FRAMES && !viewer_done)
	{
		if (openni::OpenNI::waitForAnyStream(streams, 1, &index, 5000) != STATUS_TIME_OUT) {

			streams[index]->readFrame(&ni_frame);

			depthcv.data = (uchar*)ni_frame.getData();

			void *virtual_pixel;
			int nbytes = 100;
			writer << depthcv;
			
			//cv::imshow("Depth", depthcv);
		}
		else {
			viewer_done = true;
		}
		read_frames++;
	}
	writer.release();
	
}

/*


// Perform reconstruction while viewer is not closed.

// Update the viewer
reme_sensor_get_image(c, s, REME_IMAGE_DEPTH, images_to_show[0]);
reme_sensor_get_image(c, s, REME_IMAGE_VOLUME, images_to_show[1]);
reme_viewer_update(c, viewer);
reme_viewer_is_closed(c, viewer, &viewer_done);
}
else {
viewer_done = true;
}
read_frames++;
}

if (read_frames == MAX_FRAMES) {
cout << "Read max-number of frames" << endl;
}
cout << read_frames << " frames read, starting surface generation" << endl;

//// Close and destroy the sensor, it is not needed anymore
reme_sensor_close(c, s);
reme_sensor_destroy(c, &s);

//ni_dev.close();

// Create a new surface
reme_surface_t m;
reme_surface_create(c, &m);
reme_surface_generate(c, m, v);
// Colorize surface vertices.
err = reme_surface_colorize_vertices(c, m, v);
// Inpaint vertices with invalid colors.
err = reme_surface_inpaint_vertices(c, m);
// Remesh the surface to generate an isotropic tessellation
reme_surface_bind_remesh_options(c, m, o);



// The following properties define the allowed edge length range
// The tesselation will adhere to these values when splitting/collapsing
// edges according to local color and geometry change.
reme_options_set_real(c, o, "minimum_edge_length", 3);
reme_options_set_real(c, o, "maximum_edge_length", 20);
// Perform remeshing
//reme_surface_remesh(c, m);

// Remeshing as decimation might change vertex positions,
// we should update the color information
err = reme_surface_colorize_vertices(c, m, v);
err = reme_surface_inpaint_vertices(c, m);

// Save to file including colored vertices.
string ply_filename = oni_file_path;
ply_filename += "_model.ply";
reme_surface_save_to_file(c, m, ply_filename.c_str());
cout << "Wrote model file " << ply_filename << endl;

// Visualize resulting surface
reme_viewer_t viewer_surface;
reme_viewer_create_surface(c, m, "This is ReconstructMeSDK", &viewer_surface);
reme_viewer_wait(c, viewer_surface);
reme_surface_destroy(c, &m);
// Print pending errors
reme_context_print_errors(c);
// Make sure to release all memory acquired
reme_context_destroy(&c);
getchar();
}*/