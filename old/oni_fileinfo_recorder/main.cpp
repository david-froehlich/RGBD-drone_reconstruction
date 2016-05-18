
//
// try to load the video stream from a .oni-file
//

#include <reconstructmesdk/reme.h>
#include <stdio.h>
#include <OpenNI.h>
#include <string>
#include <iostream>

#define MAX_FRAMES 500
#define DEFAULT_ONI_FILE_PATH "C:\\\\Users\\David\\Desktop\\recording.oni"

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


	// Create ReconstructMe context
	reme_context_t c;
	reme_context_create(&c);

	reme_options_t o;
	reme_options_create(c, &o);

	// Create external sensor.
	reme_sensor_t s;
	reme_sensor_create(c, "external", false, &s);

	// Since we are using an external sensor, we need to tell ReMe about its image size field of view.
	reme_sensor_bind_camera_options(c, s, o);

	// Ensure AUX stream is aligned with the DEPTH stream
	reme_options_set_bool(c, o, "enable_align_viewpoints", true);

	int image_width_depth = ni_depth.getVideoMode().getResolutionX();
	int image_height_depth = ni_depth.getVideoMode().getResolutionY();

	int image_width_color = ni_color.getVideoMode().getResolutionX();
	int image_height_color = ni_color.getVideoMode().getResolutionY();

	reme_options_set_int(c, o, "depth_stream.image_size.width", image_width_depth);
	reme_options_set_int(c, o, "depth_stream.image_size.height", image_height_depth);

	reme_options_set_int(c, o, "aux_stream.image_size.width", image_width_color);
	reme_options_set_int(c, o, "aux_stream.image_size.height", image_height_color);

	// Open the sensor like any other sensor.
	reme_sensor_open(c, s);

	reme_context_bind_reconstruction_options(c, o);

	reme_error_t err;

	// Make sure that we enable color support in the program settings.
	err = reme_options_set_bool(c, o, "data_integration.use_colors", true);

	// Use the tuning to derive a couple of related settings automatically.
	reme_context_tune_reconstruction_options(c, REME_TUNE_PROFILE_LOW_QUALITY);

	/*err = reme_options_set_int(c, o, "volume.minimum_corner.x", -500);
	err = reme_options_set_int(c, o, "volume.minimum_corner.y", -500);
	err = reme_options_set_int(c, o, "volume.minimum_corner.z", -3000);

	err = reme_options_set_int(c, o, "volume.maximum_corner.x", 500);
	err = reme_options_set_int(c, o, "volume.maximum_corner.y", 500);
	err = reme_options_set_int(c, o, "volume.maximum_corner.z", 500);*/

	reme_context_compile(c);

	// Create a new volume
	reme_volume_t v;
	reme_volume_create(c, &v);

	reme_sensor_set_prescan_position(c, s, REME_SENSOR_POSITION_INFRONT);
	reme_sensor_bind_render_options(c, s, o);
	// Use color in rendering volume image
	reme_options_set(c, o, "shade_mode", "SHADE_COLORS");

	// Apply
	reme_sensor_apply_render_options(c, s, o);

	// In order inform ReMe about external sensor data
	reme_image_t raw_depth, color;

	reme_image_t* image_containers[] = { &raw_depth, &color };

	reme_image_create(c, &color);
	reme_image_create(c, &raw_depth);

	// Create a viewer
	reme_viewer_t viewer;
	reme_viewer_create_image(c, "This is ReconstructMe SDK", &viewer);

	reme_image_t images_to_show[2];
	reme_image_create(c, &images_to_show[0]);
	reme_image_create(c, &images_to_show[1]);
	reme_viewer_add_image(c, viewer, images_to_show[0]);
	reme_viewer_add_image(c, viewer, images_to_show[1]);


	// Perform reconstruction while viewer is not closed.

	VideoStream* streams[] = { &ni_depth, &ni_color };
	_reme_sensor_image_t sensor_types[] = { REME_IMAGE_RAW_DEPTH, REME_IMAGE_RAW_AUX };
	bool viewer_done = false;
	int read_frames = 0;
	int index;

	reme_error_t ret;
	while (read_frames < MAX_FRAMES && !viewer_done)
	{
		if (openni::OpenNI::waitForAnyStream(streams, 2, &index, 5000) != STATUS_TIME_OUT) {

			streams[index]->readFrame(&ni_frame);
			const unsigned short *sensor_pixel = (const unsigned short *)ni_frame.getData();

			reme_sensor_grab(c, s);
			reme_sensor_prepare_image(c, s, sensor_types[index]);
			reme_sensor_get_image(c, s, sensor_types[index], *image_containers[index]);
			void *virtual_pixel;
			int nbytes;

			reme_image_get_mutable_bytes(c, *image_containers[index], &virtual_pixel, &nbytes);

			// Copy content
			memcpy(virtual_pixel, sensor_pixel, nbytes);

			reme_sensor_prepare_image(c, s, REME_IMAGE_VOLUME);
			ret = reme_sensor_track_position(c, s);
			if (REME_SUCCESS(ret)) {
				reme_sensor_update_volume(c, s);
			}
			else {
				if (ret == REME_ERROR_TRACK_LOST) {
					cout << "Lost tracking pose on frame " << read_frames << endl;

				}
				else {
					cout << "Unknown error occurred during reconstruction on frame " << read_frames << endl;
					getchar();
				}
				break;
			}
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
	reme_surface_remesh(c, m);

	// Remeshing as decimation might change vertex positions,
	// we should update the color information
	err = reme_surface_colorize_vertices(c, m, v);
	err = reme_surface_inpaint_vertices(c, m);

	// Save to file including colored vertices.
	reme_surface_save_to_file(c, m, "test.ply");
	// Visualize resulting surface
	reme_viewer_t viewer_surface;
	reme_viewer_create_surface(c, m, "This is ReconstructMeSDK", &viewer_surface);
	reme_viewer_wait(c, viewer_surface);
	reme_surface_destroy(c, &m);
	// Print pending errors
	reme_context_print_errors(c);
	// Make sure to release all memory acquired
	reme_context_destroy(&c);
}