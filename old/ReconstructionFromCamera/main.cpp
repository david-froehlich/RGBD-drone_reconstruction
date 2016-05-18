
#include <stdio.h>
#include <string>
#include <iostream>
#include <reconstructmesdk/reme.h>

using namespace std;
int main(int argc, char* args[]) {
	reme_context_t c;
	reme_context_create(&c);
	// Create a license object
	reme_license_t l;
	reme_license_create(c, &l);
	// Create options
	reme_options_t o;
	reme_options_create(c, &o);
	// Create a new sensor.
	reme_sensor_t s;
	reme_sensor_create(c, "openni;mskinect;file", true, &s);
	reme_sensor_bind_camera_options(c, s, o);
	// Ensure AUX stream is aligned with the DEPTH stream
	reme_options_set_bool(c, o, "enable_align_viewpoints", true);
	reme_sensor_open(c, s);
	reme_context_bind_reconstruction_options(c, o);
	// Make sure that we enable color support in the program settings.
	reme_options_set_bool(c, o, "data_integration.use_colors", true);
	// Use the tuning to derive a couple of related settings automatically.
	reme_context_tune_reconstruction_options(c, REME_TUNE_PROFILE_MID_QUALITY);
	// Compile for OpenCL device using defaults
	reme_context_compile(c);
	// Create a new volume
	reme_volume_t v;
	reme_volume_create(c, &v);
	// Position sensor outside of volume
	reme_sensor_set_prescan_position(c, s, REME_SENSOR_POSITION_INFRONT);
	reme_sensor_bind_render_options(c, s, o);
	// Use color in rendering volume image
	reme_options_set(c, o, "shade_mode", "SHADE_COLORS");
	// Apply
	reme_sensor_apply_render_options(c, s, o);
	reme_viewer_t viewer;
	reme_viewer_create_image(c, "This is ReconstructMe SDK", &viewer);
	reme_image_t volume, aux;
	reme_image_create(c, &volume);
	reme_image_create(c, &aux);
	reme_viewer_add_image(c, viewer, aux);
	reme_viewer_add_image(c, viewer, volume);
	// Perform reconstruction until no more frames are left
	int time = 0;
	while (time < 600 && REME_SUCCESS(reme_sensor_grab(c, s))) {
		reme_sensor_prepare_images(c, s);
		if (REME_SUCCESS(reme_sensor_track_position(c, s))) {
			reme_sensor_update_volume(c, s);
		}
		reme_sensor_get_image(c, s, REME_IMAGE_AUX, aux);
		reme_sensor_get_image(c, s, REME_IMAGE_VOLUME, volume);
		reme_viewer_update(c, viewer);
		time += 1;
	}
	// Browse volume directly without generating a mesh.
	reme_viewer_t viewer2;
	reme_viewer_create_volume(c, v, s, "This is ReconstructMe SDK", &viewer2);
	reme_viewer_wait(c, viewer2);
	// Close and destroy the sensor, it is not needed anymore
	reme_sensor_close(c, s);
	reme_sensor_destroy(c, &s);
	// Create a new surface
	reme_surface_t m;
	reme_surface_create(c, &m);
	reme_surface_generate(c, m, v);
	// Colorize surface vertices.
	reme_surface_colorize_vertices(c, m, v);
	// Inpaint vertices with invalid colors.
	reme_surface_inpaint_vertices(c, m);
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
	reme_surface_colorize_vertices(c, m, v);
	reme_surface_inpaint_vertices(c, m);
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