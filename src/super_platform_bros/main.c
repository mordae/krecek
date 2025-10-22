#include <sdk.h>

float volume = 0.5f;

void game_start(void)
{
	sdk_set_output_gain_db(volume);
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs.vol_up) {
		volume += 12.0 * dt;
	}

	if (sdk_inputs.vol_down) {
		volume -= 12.0 * dt;
	}

	if (sdk_inputs_delta.select > 0) {
		game_reset();
		return;
	}

	if (sdk_inputs_delta.vol_sw > 0) {
		if (!sdk_inputs.start) {
			if (volume < SDK_GAIN_MIN) {
				volume = 0;
			} else {
				volume = SDK_GAIN_MIN - 1;
			}
		}
	}

	volume = clamp(volume, SDK_GAIN_MIN - 1.0, 6);

	if (sdk_inputs.vol_up || sdk_inputs.vol_down || sdk_inputs.vol_sw) {
		sdk_set_output_gain_db(volume);
	}
}

void game_paint(unsigned dt_usec)
{
 (void)dt_usec;
  tft_fill(0); //reset screen every frame to 0-black                                                                                             
               }                                                                                                
       
int main()                                                                                               
{                                                                                                        
        struct sdk_config config = {                                                                     
                .wait_for_usb = true,                                                                    
                .show_fps = false,                                                                       
                .off_on_select = true,                                                                   
                .fps_color = rgb_to_rgb565(31, 31, 31),                                                  
        };                                                                                               
                                                                                                         
        sdk_main(&config);                                                                               
}
